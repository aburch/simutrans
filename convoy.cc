/*
 * Copyright (c) 2009 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * convoy_t: common collection of properties of a convoi_t and a couple of vehicles which are going to become a convoy_t.
 * While convoi_t is involved in game play, convoy_t is the entity that physically models the convoy.
 */

#include <math.h>
#include "convoy.h"
#include "bauer/warenbauer.h"
#include "vehicle/simvehikel.h"


// make sure, that no macro calculates a or b twice:
//inline double double_min(double a, double b)
//{
//	return a < b ? a : b;
//}

// helps to calculate roots. pow fails to calculate roots of negative bases.
//inline double signed_power(double base, double expo)
//{
//	if (base >= 0)
//		return pow(base, expo);
//	return -pow(-base, expo);
//}

static void get_possible_freight_weight(uint8 catg_index, sint32 &min_weight, sint32 &max_weight)
{
	max_weight = 0;
	min_weight = WEIGHT_UNLIMITED;
	for (uint16 j=0; j<warenbauer_t::get_waren_anzahl(); j++) 
	{
		const ware_besch_t &ware = *warenbauer_t::get_info(j);
		if (ware.get_catg_index() == catg_index) 
		{
			sint32 weight = ware.get_weight_per_unit();
			if (max_weight < weight) 
			{
				max_weight = weight;
			}
			if (min_weight > weight) 
			{
				min_weight = weight;
			}
		}
	}
	// No freight of given category found? Then there is no min weight!
	if (min_weight == WEIGHT_UNLIMITED) 
	{
		min_weight = 0;
	}
}

/******************************************************************************/

void adverse_summary_t::add_vehicle(const vehikel_t &v)
{
	const waytype_t waytype = v.get_waytype();
	if (max_speed == KMH_SPEED_UNLIMITED)
	{
		// all vehicles have the same waytype, thus setting once is enough
		set_by_waytype(waytype);
	}
	if (waytype != air_wt || ((const aircraft_t &)v).get_flyingheight() <= 0)
	{
		const grund_t *gr = v.get_welt()->lookup(v.get_pos());
		if (gr)
		{
			weg_t *way = gr->get_weg(waytype);
			if (way)
			{
				sint32 limit = way->get_max_speed();
				if (max_speed > limit)
				{
					max_speed = limit;
				}
			}
		}
	}
	if (v.is_first()) {
		cf = fraction32_t(v.get_besch()->get_air_resistance(), 100);
	}
	// The vehicle may be limited by braking approaching a station
	// Or airplane circling for landing, or airplane height,
	// Or cornering, or other odd cases
	// These are carried in vehikel_t unlike other speed limits 
	if (v.get_speed_limit() == vehikel_t::speed_unlimited() ) {
		max_speed = KMH_SPEED_UNLIMITED;
	}
	else
	{
		sint32 limit = speed_to_kmh(v.get_speed_limit());
		if (max_speed > limit)
		{
			max_speed = limit;
		}
	}
}

/******************************************************************************/

void freight_summary_t::add_vehicle(const vehikel_besch_t &b)
{
	const sint32 payload = b.get_zuladung();
	if (payload > 0)
	{
		sint32 min_weight, max_weight;
		get_possible_freight_weight(b.get_ware()->get_catg_index(), min_weight, max_weight);
		min_freight_weight += min_weight * payload;
		max_freight_weight += max_weight * payload;
	}
}

/******************************************************************************/

void weight_summary_t::add_weight(sint32 kgs, sint32 sin_alpha)
{
	//sint32 kgs = tons > 0 ? tons * 1000 : 100; // give a minimum weight
	weight += kgs; 
	// sin_alpha <-- v.get_frictionfactor() between about -14 (downhill) and 50 (uphill). 
	// Including the factor 1000 for tons to kg conversion, 50 corresponds to an inclination of 28 per mille.
	// sin_alpha == 1000 would be 90 degrees == vertically up, -1000 vertically down.
	if (sin_alpha != 0)
	{
		weight_sin += (kgs * sin_alpha + 500) / 1000;
	}
	//if (sin_alpha > 200 || sin_alpha < -200)
	//{
	//	// Below sin_alpha = 200 the deviation from correct result is less than 2%! Noone will ever see the difference,
	//	// but for the time being, we always save a lot of time skipping this evaluation.
	//	weight_cos += tons * sqrt(1000000.0 - sin_alpha * sin_alpha); // Remember: sin(alpha)^2 + cos(alpha)^2 = 1
	//}
	//else
	{			 
		weight_cos += kgs;
	}
}

/******************************************************************************/

sint32 convoy_t::calc_max_speed(const weight_summary_t &weight) 
{ 
	
	const fraction32_t nine_point_eight_one(981, 10);

	const fraction32_t Frs = nine_point_eight_one * (adverse.fr * weight.weight_cos + weight.weight_sin);
	if (Frs > get_starting_force()) 
	{
		// this convoy is too heavy to start.
		return 0;
	}
	
	const fraction32_t one = 1;
	const fraction32_t two = 2;
	const fraction32_t three = 3;
	const fraction32_t three_point_six(36, 10);
	
	const fraction32_t p3 =  Frs / (three * adverse.cf);
	const fraction32_t q2 = fraction32_t(get_continuous_power()) / (two * adverse.cf);
	const fraction32_t sd = pow(q2 * q2 + p3 * p3 * p3, one/two);
	const fraction32_t vmax = (pow(q2 + sd, one/three) + pow(q2 - sd, one/three)); 
	const fraction32_t comparator = vmax * three_point_six + one; // 1.0 to compensate inaccuracy of calculation and make sure this is at least what calc_move() evaluates.
	const fraction32_t return_value = min(vehicle.max_speed, comparator.integer());  // This is the same clipping as simply casting a double to an int.
	return return_value.integer(); // This is the same clipping as simply casting a double to an int.

}

sint32 convoy_t::calc_max_weight(sint32 sin_alpha)
{
	const fraction32_t one = 1;
	const fraction32_t three_point_six(36, 10);
	
	if (vehicle.max_speed == 0)
		return 0;
	const fraction32_t v = (fraction32_t(vehicle.max_speed)) * (one/three_point_six); // from km/h to m/s
	const fraction32_t f = fraction32_t(get_continuous_power()) / v - adverse.cf * v * v;
	if (f <= 0)
	{
		return 0;
	}
	const fraction32_t comparator = fraction32_t(981, 10) * (adverse.fr + fraction32_t(1, 1000) * sin_alpha);
	const sint32 comp_int = comparator.integer();
	return abs(min(get_starting_force(), f.integer()) / (comp_int == 0 ? 1 : comp_int));
}

sint32 convoy_t::calc_max_starting_weight(sint32 sin_alpha)
{
	const fraction32_t denominator = (fraction32_t(101, 100) * fraction32_t(981, 100) * (adverse.fr + fraction32_t(1, 1000) * sin_alpha));
	const sint32 d_int = denominator.integer();
	return abs(get_starting_force() / (d_int == 0 ? 1 : d_int)); // 1.01 to compensate inaccuracy of calculation 
}

fraction32_t convoy_t::calc_speed_holding_force(fraction32_t speed /* in m/s */, fraction32_t Frs /* in N */)
{
	const fraction32_t first_comparator = adverse.cf * speed * speed;
	const fraction32_t second_comparator = fraction32_t(get_force(speed), 1) - Frs;
	return min(first_comparator.integer(), second_comparator.integer()); /* in N */
}

//sint32 convoy_t::new_calc_speed_holding_force_100(sint32 speed /* in m/s x 100 */, sint32 Frs /* in N x 100*/)
//{
//	return min((adverse.cf * speed * speed) / 100, get_force(speed) - Frs); /* in N x 100 */
//	//return min((adverse.cf * speed * speed) / 10000, get_force(speed) - new_frs_100); // USE THIS WHEN CF IS 100 x
//}

// The timeslice to calculate acceleration, speed and covered distance in reasonable small chuncks. 
#define DT_TIME_FACTOR 64
#define DT_SLICE_SECONDS 2
#define DT_SLICE (DT_TIME_FACTOR * DT_SLICE_SECONDS)

void convoy_t::calc_move(long delta_t, uint16 simtime_factor_integer, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll)
{
	const fraction32_t simtime_factor(simtime_factor_integer, 100);
	fraction32_t dx = 0;
	/*sint32 new_dx_100 = 0;*/
	if (adverse.max_speed < KMH_SPEED_UNLIMITED)
	{
		const sint32 speed_limit = kmh_to_speed(adverse.max_speed);
		if (akt_speed_soll > speed_limit)
		{
			akt_speed_soll = speed_limit;
		}
	}
	if (delta_t > 1800 * DT_TIME_FACTOR)
	{
		// After 30 minutes any vehicle has reached its akt_speed_soll. 
		// Shorten the process. 
		akt_speed = min(akt_speed_soll, kmh_to_speed(calc_max_speed(weight)));
		dx = delta_t * akt_speed;
	}
	else
	{
		const fraction32_t Frs = fraction32_t(981, 100) * (adverse.fr * weight.weight_cos + weight.weight_sin); // msin, mcos are calculated per vehicle due to vehicle specific slope angle.
		/*const sint32 new_frs_100 = 981 * (adverse.fr * weight.weight_cos + weight.weight_sin);*/
		const fraction32_t vmax = speed_to_v(akt_speed_soll);
		/*const uint32 new_vmax_100 = speed_to_v(akt_speed_soll)  * 100;*/
		fraction32_t v = speed_to_v(akt_speed); // v in m/s, akt_speed in simutrans vehicle speed;
		//sint32 new_v_10000 = speed_to_v(akt_speed) * 10000; // v in m/s, akt_speed in simutrans vehicle speed;
		fraction32_t fvmax = 0; // force needed to hold vmax. will be calculated as needed
		/*sint32 new_fvmax_100 = 0;*/
		fraction32_t speed_ratio = 0; 
		/*sint32 new_speed_ratio_100 = 0;*/
		//static uint32 count1 = 0;
		//static sint32 count2 = 0;
		//static sint32 count3 = 0;
		//count1++;
		// iterate the passed time.
		while (delta_t > 0)
		{
			// the driver's part: select accelerating force:
			fraction32_t f;
			/*uint32 new_f_100;*/
			bool is_breaking = false; // don't roll backwards, due to breaking

			if (v < fraction32_t(999, 100) * vmax)
			{
				//assert(new_v_10000 < (999 * new_vmax_100) / 10);
				// Below set speed: full acceleration
				// If set speed is far below the convoy max speed as e.g. aircrafts on ground reduce force.
				// If set speed is at most a 10th of convoy's maximum, we reduce force to its 10th.
				f = fraction32_t(get_force(v)) - Frs;
				/*new_f_100 = (get_force(new_v_10000) / 100) - new_frs_100;*/
				if (f > 1000000) // reducing force does not apply to 'weak' convoy's, thus we can save a lot of time skipping this code.
				{
					//assert(new_f_100 > 100000000);
					if (speed_ratio == 0) // speed_ratio is a constant within this function. So calculate it once only.
					{
						speed_ratio = fraction32_t(36, 10) * vmax / vehicle.max_speed;
						/*new_speed_ratio_100 = 360 * new_vmax_100 / (vehicle.max_speed * 100);*/
					}
					if (speed_ratio < fraction32_t(1, 10))
					{
						//assert(new_speed_ratio_100 < 10);
						fvmax = calc_speed_holding_force(vmax, Frs);
						//new_fvmax_100 = fvmax * 100; /*TEMPORARY - need to re-do method called */
						if (f > fvmax)
						{
							//assert(new_f_100 > new_fvmax_100);

							f = (f - fvmax) * fraction32_t(1, 10) + fvmax;

							/*new_f_100 = (new_f_100 - new_fvmax_100) * 10 + new_fvmax_100;*/
							//assert(new_f_100 == ((uint32)f * 100));
						}
					}
				}
			}
			else if (v < fraction32_t(1001, 1000) * vmax)
			{
				//assert(new_v_10000 < (1001 * new_vmax_100) / 10);
				// at or slightly above set speed: hold this speed
				if (fvmax == 0) // fvmax is a constant within this function. So calculate it once only.
				{
					fvmax = calc_speed_holding_force(vmax, Frs);
					/*new_fvmax_100 = new_calc_speed_holding_force_100(new_vmax_100, new_frs_100);*/
					///assert((sint32)(fvmax * 100) == new_fvmax_100);
				}
				f = fvmax;
				/*new_f_100 = new_fvmax_100;*/

			}
			else if (v < fraction32_t(11, 10) * vmax)
			{
				//assert(new_v_10000 < 110 * new_vmax_100);
				// slightly above set speed: coasting 'til back to set speed.
				f = fraction32_t(-Frs.n, Frs.d);
				/*new_f_100 = -new_frs_100;*/
			}
			else if (v < fraction32_t(15, 10) * vmax)
			{
				///assert(new_v_10000 <  150 * new_vmax_100);
				is_breaking = true;
				// running too fast, apply the breaks! 
				// hill-down Frs might become negative and works against the brake.
				const fraction32_t tmp = fraction32_t(get_starting_force()) + Frs;
				f = fraction32_t(-tmp.n, tmp.d);
				/*new_f_100 = -(get_starting_force() * 100 + new_frs_100);*/
			}
			else
			{
				is_breaking = true;
				// running much too fast, slam on the brakes! 
				// assuming the brakes are up to 5 times stronger than the start-up force.
				// hill-down Frs might become negative and works against the brake.
				const fraction32_t tmp = (fraction32_t(5 * get_starting_force()) + Frs);
				f = fraction32_t(-tmp.n, tmp.d);
					
				/*new_f_100 = -(500 * get_starting_force() + new_frs_100);*/
			}

			// accelerate: calculate new speed according to acceleration within the passed second(s).
			long dt;
			fraction32_t df = (simtime_factor * (f - fraction32_t(sgn(v.n), v.d)) * adverse.cf * v * v) / fraction32_t(100);
			/*const sint32 new_v_100 = new_v_10000 / 100;*/
			//sint32 new_df_100 = (simtime_factor * (new_f_100 - sgn<sint32>(new_v_10000) * adverse.cf * new_v_100 * new_v_100)) / 1000000; /* Will need to be / 100000000 when cf is *100*/
			if (delta_t >= DT_SLICE && (sint32)abs(df.integer()) > weight.weight / (10 * DT_SLICE_SECONDS))
			{
				//assert(delta_t >= DT_SLICE && abs(new_df_100) > weight.weight * 10 / (DT_SLICE_SECONDS));
				// This part is important for acceleration/deceleration phases only.
				// When a small force produces small speed change, we can add it at once in the 'else' section.
				//count2++;
				v += (fraction32_t(DT_SLICE_SECONDS) * df) / weight.weight; 
				/*new_v_10000 += (DT_SLICE_SECONDS * 100 * new_df_100) / weight.weight; */
				//assert(new_v_10000 ==  v * 10000);
				dt = DT_SLICE;
			}
			else
			{
				//count3++;
				v += (fraction32_t(delta_t) * df) / (DT_TIME_FACTOR * weight.weight); 
				/*new_v_10000 += (delta_t * new_df_100 * 100) / (DT_TIME_FACTOR * weight.weight);*/ 
				dt = delta_t;
			}
			if (is_breaking)
			{
				if (v < vmax)
				{
					//assert(new_v_10000 < new_vmax_100 * 100);
					v = vmax;
					/*new_v_10000 = new_vmax_100 * 100;*/
				}
			}
			else if (/* is_breaking */ f < 0 && v < 1)
			{
				//assert(new_f_100 < 0 && new_v_10000 < 10000);
				v = 1;
				/*new_v_10000 = 10000;*/
			}
			dx += fraction32_t(dt) * v;
			/*new_dx_100 += (dt * new_v_10000) / 100;*/
			delta_t -= dt; // another DT_SLICE_SECONDS passed
		}
		akt_speed = v_to_speed(v); // akt_speed in simutrans vehicle speed, v in m/s
		/*const sint32 shadow_akt_speed = v_to_speed(new_v_10000) / 10000;*/
		//assert(akt_speed >= shadow_akt_speed - 50 && akt_speed <= shadow_akt_speed + 50);
		dx = x_to_steps(dx);
		/*new_dx_100 = x_to_steps(new_dx_100);*/
	}
	if (dx < KMH_SPEED_UNLIMITED - sp_soll)
	{
		sp_soll += dx.integer();
		/*sp_soll += dx / 100; */
	}	
	else
	{
		sp_soll = KMH_SPEED_UNLIMITED;
	}
}

/******************************************************************************/

void potential_convoy_t::update_vehicle_summary(vehicle_summary_t &vehicle)
{		
	vehicle.clear();
	sint32 count = vehicles.get_count();
	for (uint32 i = count; i-- > 0; )
	{
		vehicle.add_vehicle(*vehicles[i]);
	}
	if (count > 0)
	{
		vehicle.update_summary(vehicles[count - 1]->get_length());
	}
}


void potential_convoy_t::update_adverse_summary(adverse_summary_t &adverse)
{
	adverse.clear();
	uint32 i = vehicles.get_count();
	if (i > 0)
	{
		const vehikel_besch_t &b = *vehicles[0];
		adverse.set_by_waytype(b.get_waytype());
	}		
}


void potential_convoy_t::update_freight_summary(freight_summary_t &freight)
{		
	freight.clear();
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		const vehikel_besch_t &b = *vehicles[i];
		freight.add_vehicle(b);
	}
}


sint32 potential_convoy_t::get_force_summary(sint32 speed /* in m/s */)
{
	sint32 force = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		force += vehicles[i]->get_effective_force_index(speed);
	}
	return (force * world.get_einstellungen()->get_global_power_factor_percent() * (100 / GEAR_FACTOR) + 50) / 10000;
}


sint32 potential_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		power += vehicles[i]->get_effective_power_index(speed);
	}
	return (power * world.get_einstellungen()->get_global_power_factor_percent() * (100 / GEAR_FACTOR) + 50) / 10000;
}

// Bernd Gabriel, Dec, 25 2009
sint16 potential_convoy_t::get_current_friction()
{
	return vehicles.get_count() > 0 ? get_friction_of_waytype(vehicles[0]->get_waytype()) : 0;
}

/******************************************************************************/

// Bernd Gabriel, Dec, 25 2009
sint16 existing_convoy_t::get_current_friction()
{
	return convoy.get_vehikel_anzahl() > 0 ? get_friction_of_waytype(convoy.get_vehikel(0)->get_waytype()) : 0;
}

void existing_convoy_t::update_vehicle_summary(vehicle_summary_t &vehicle)
{
	vehicle.clear();
	uint32 count = convoy.get_vehikel_anzahl();
	for (uint32 i = count; i-- > 0; )
	{
		vehicle.add_vehicle(*convoy.get_vehikel(i)->get_besch());
	}
	if (count > 0)
	{
		vehicle.update_summary(convoy.get_vehikel(count-1)->get_besch()->get_length());
	}
	//both get_length() and get_tile_length() iterate through the list of vehicles, thus most likely it's faster to iterate once as done above.
	//vehicle.length = convoy.get_length();
	//vehicle.tiles = convoy.get_tile_length();
	//vehicle.max_speed = speed_to_kmh(convoy.get_min_top_speed());
	//vehicle.weight = convoy.get_sum_gewicht() * 1000;
}


void existing_convoy_t::update_adverse_summary(adverse_summary_t &adverse)
{
	adverse.clear();
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		vehikel_t &v = *convoy.get_vehikel(i);
		adverse.add_vehicle(v);
	}
}


void existing_convoy_t::update_freight_summary(freight_summary_t &freight)
{		
	freight.clear();
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		const vehikel_besch_t &b = *convoy.get_vehikel(i)->get_besch();
		freight.add_vehicle(b);
	}
}


void existing_convoy_t::update_weight_summary(weight_summary_t &weight)
{
	weight.clear();
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		vehikel_t &v = *convoy.get_vehikel(i);
		weight.add_vehicle(v);
	}
}


sint32 existing_convoy_t::get_force_summary(sint32 speed /* in m/s */)
{
	sint32 force = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		force += convoy.get_vehikel(i)->get_besch()->get_effective_force_index(speed);
	}
	return (force * convoy.get_welt()->get_einstellungen()->get_global_power_factor_percent() * (100 / GEAR_FACTOR) + 50) / 10000;
}


sint32 existing_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		power += convoy.get_vehikel(i)->get_besch()->get_effective_power_index(speed);
	}
	return (power * convoy.get_welt()->get_einstellungen()->get_global_power_factor_percent() * (100 / GEAR_FACTOR) + 50) / 10000;
}

