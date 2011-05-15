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
inline double signed_power(double base, double expo)
{
	if (base >= 0)
		return pow(base, expo);
	return -pow(-base, expo);
}

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
		cf = fraction_t(v.get_besch()->get_air_resistance(), 100);
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
	// BG, 14.05.2011: back to 'double' calculation, as log() for large fractions 
	// does not work sufficient and this code is not network relevant. It is only used in dialogs. 
	const double Frs = 9.81 * (adverse.fr * weight.weight_cos + weight.weight_sin).to_double();
	if (Frs > get_starting_force()) 
	{
		// this convoy is too heavy to start.
		return 0;
	}
	const double p3 =  Frs / (3.0 * adverse.cf.to_double());
	const double q2 = get_continuous_power() / (2.0 * adverse.cf.to_double());
	const double sd = signed_power(q2 * q2 + p3 * p3 * p3, 1.0/2.0);
	const double vmax = signed_power(q2 + sd, 1.0/3.0) + signed_power(q2 - sd, 1.0/3.0);
	return min(vehicle.max_speed, (sint32)(vmax * 3.6 + 1.0)); // 1.0 to compensate inaccuracy of calculation and make sure this is at least what calc_move() evaluates.

	//const fraction_t Frs = fraction_t(981, 100) * (adverse.fr * weight.weight_cos + weight.weight_sin);
	//if (Frs > get_starting_force()) 
	//{
	//	// this convoy is too heavy to start.
	//	return 0;
	//}
	//const fraction_t p3 =  Frs / (fraction_t(3) * adverse.cf);
	//const fraction_t q2 = fraction_t(get_continuous_power()) / (fraction_t(2) * adverse.cf);
	//const fraction_t sd = pow(q2 * q2 + p3 * p3 * p3, fraction_t(1,2));
	//const fraction_t vmax = (pow(q2 + sd, fraction_t(1, 3)) + pow(q2 - sd,fraction_t(1, 3))); 
	//const fraction_t comparator = vmax * fraction_t(18, 5) + 1; // 1.0 to compensate inaccuracy of calculation and make sure this is at least what calc_move() evaluates.
	//return min(vehicle.max_speed, comparator.integer()); // This is the same clipping as simply casting a double to an int.
}

sint32 convoy_t::calc_max_weight(sint32 sin_alpha)
{
	if (vehicle.max_speed == 0)
		return 0;
	const fraction_t v = (fraction_t(vehicle.max_speed)) * fraction_t(5, 18); // from km/h to m/s
	const fraction_t f = fraction_t(get_continuous_power()) / v - adverse.cf * v * v;
	if (f <= 0)
	{
		return 0;
	}
	const fraction_t comparator = fraction_t(981, 100) * (adverse.fr + fraction_t(1, 1000) * sin_alpha);
	const sint32 comp_int = comparator.integer();
	return abs(min(get_starting_force(), f.integer()) / (comp_int == 0 ? 1 : comp_int));
}

sint32 convoy_t::calc_max_starting_weight(sint32 sin_alpha)
{
	const fraction_t denominator = (fraction_t(101, 100) * fraction_t(981, 100) * (adverse.fr + fraction_t(1, 1000) * sin_alpha));
	const sint32 d_int = denominator.integer();
	return abs(get_starting_force() / (d_int == 0 ? 1 : d_int)); // 1.01 to compensate inaccuracy of calculation 
}

sint32 convoy_t::calc_speed_holding_force(const fraction_t &speed /* in m/s */, sint32 Frs /* in N */)
{
	return min((adverse.cf * speed * speed).integer(), get_force(speed.integer()) - Frs); /* in N */
}

// The timeslice to calculate acceleration, speed and covered distance in reasonable small chuncks. 
#define DT_TIME_FACTOR 64
#define DT_SLICE_SECONDS 2
#define DT_SLICE (DT_TIME_FACTOR * DT_SLICE_SECONDS)

void convoy_t::calc_move(long delta_t, uint16 simtime_factor_integer, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll)
{
	const fraction_t simtime_factor(simtime_factor_integer, 100);
	fraction_t dx = 0;
	//double d_dx = 0.0;
	if (adverse.max_speed < KMH_SPEED_UNLIMITED)
	{
		const sint32 speed_limit = kmh_to_speed(adverse.max_speed);
		if (akt_speed_soll > speed_limit)
		{
			akt_speed_soll = speed_limit;
		}
	}

	//if (delta_t > 1800 * DT_TIME_FACTOR)
	//{
	//	// After 30 minutes any vehicle has reached its akt_speed_soll. 
	//	// Shorten the process. 
	//	akt_speed = min(akt_speed_soll, kmh_to_speed(calc_max_speed(weight)));
	//	dx = delta_t * akt_speed;
	//}
	//else
	{
		//const double d_Frs = 9.81 * (adverse.fr.to_double() * weight.weight_cos.to_double() + weight.weight_sin.to_double()); // msin, mcos are calculated per vehicle due to vehicle specific slope angle.
		//const double d_vmax = d_speed_to_v(akt_speed_soll);
		//double d_v = d_speed_to_v(akt_speed); // v in m/s, akt_speed in simutrans vehicle speed;
		//double d_fvmax = 0; // force needed to hold vmax. will be calculated as needed
		//double d_speed_ratio = 0; 

		const sint32 Frs = (fraction_t(981, 100) * (adverse.fr * weight.weight_cos + weight.weight_sin)).integer(); // msin, mcos are calculated per vehicle due to vehicle specific slope angle.
		const fraction_t vmax = speed_to_v(akt_speed_soll);
		fraction_t v = speed_to_v(akt_speed); // v in m/s, akt_speed in simutrans vehicle speed;
		sint32 fvmax = 0; // force needed to hold vmax. will be calculated as needed
		sint32 speed_ratio = 0;

		//static uint32 count1 = 0;
		//static sint32 count2 = 0;
		//static sint32 count3 = 0;
		//count1++;
		// iterate the passed time.
		while (delta_t > 0)
		{
			// the driver's part: select accelerating force:
			//double d_f;
			sint32 f;
			bool is_breaking = false; // don't roll backwards, due to breaking

			if (1000 * v < vmax * 999) // same as: v < 0.999 * vmax
			{
				// Below set speed: full acceleration
				// If set speed is far below the convoy max speed as e.g. aircrafts on ground reduce force.
				// If set speed is at most a 10th of convoy's maximum, we reduce force to its 10th.
				//d_f = get_force((sint32)(d_v + 0.5)) - d_Frs;
				f = get_force(v.integer()) - Frs;
				if (f > 1000000) // reducing force does not apply to 'weak' convoy's, thus we can save a lot of time skipping this code.
				{
					//if (d_speed_ratio == 0) // speed_ratio is a constant within this function. So calculate it once only.
					//{
					//	// vehicle.max_speed is in km/h, vmax in m/s
					//	d_speed_ratio = (sint32)d_vmax != 0 ? 3.6 * d_vmax / vehicle.max_speed : 0.01 /* any value < 0.1 */;
					//	d_speed_ratio = 1.0 / d_speed_ratio;
					//}
					if (speed_ratio == 0) // speed_ratio is a constant within this function. So calculate it once only.
					{
						// vehicle.max_speed is in km/h, vmax in m/s
						speed_ratio = vmax.integer() != 0 ? 10 * vehicle.max_speed / 36 * vmax.integer() : 1000000 /* any value > 10 */;
					}
					if (speed_ratio > 10)
					{
						// set speed is less than a 10th of the vehicles maximum speed.
						//d_fvmax = d_calc_speed_holding_force(d_vmax, d_Frs);
						//if (d_f > d_fvmax)
						//{
						//	d_f = (d_f - d_fvmax) * 0.1 + d_fvmax;
						//}

						fvmax = calc_speed_holding_force(vmax, Frs);
						if (f > fvmax)
						{
							f = (f - fvmax) / 10 + fvmax;
						}
					}
				}
			}
			else if (1000 * v < 1001 * vmax) // same as v < 1.001 * vmax
			{
				// at or slightly above set speed: hold this speed
				//if (d_fvmax == 0) // fvmax is a constant within this function. So calculate it once only.
				//{
				//	d_fvmax = d_calc_speed_holding_force(d_vmax, d_Frs);
				//}
				//d_f = d_fvmax;

				if (fvmax == 0) // fvmax is a constant within this function. So calculate it once only.
				{
					fvmax = calc_speed_holding_force(vmax.integer(), Frs);
				}
				f = fvmax;
			}
			else if (10 * v < 11 * vmax)
			{
				// slightly above set speed: coasting 'til back to set speed.
				//d_f = -d_Frs;
				f = -Frs;
			}
			else if (10 * v < 15 * vmax)
			{
				// running too fast, apply the breaks!
				is_breaking = true;
				// hill-down Frs might become negative and works against the brake.
				//d_f = -(get_starting_force() + d_Frs);
				f = -(get_starting_force() + Frs);
			}
			else
			{
				// running much too fast, slam on the brakes!
				is_breaking = true;
				// assuming the brakes are up to 5 times stronger than the start-up force.
				// hill-down Frs might become negative and works against the brake.
				//d_f = -(5 * get_starting_force() + d_Frs);
				f = -(5 * get_starting_force() + Frs);
			}

			// accelerate: calculate new speed according to acceleration within the passed second(s).

			//long d_dt;
			//double d_df = simtime_factor.to_double() * (d_f - sgn(d_v) * adverse.cf.to_double() * d_v * d_v);
			//if (delta_t >= DT_SLICE && (uint32)abs(d_df) > weight.weight / (10 * DT_SLICE_SECONDS))
			//{
			//	// This part is important for acceleration/deceleration phases only.
			//	// When a small force produces small speed change, we can add it at once in the 'else' section.
			//	//count2++;
			//	d_v += (DT_SLICE_SECONDS * d_df) / weight.weight; 
			//	d_dt = DT_SLICE;
			//}
			//else
			//{
			//	//count3++;
			//	d_v += (delta_t * d_df) / (DT_TIME_FACTOR * weight.weight); 
			//	d_dt = delta_t;
			//}
			//if (is_breaking)
			//{
			//	if (d_v < d_vmax)
			//	{
			//		d_v = d_vmax;
			//	}
			//}
			//else if (/* is_breaking */ d_f < 0 && d_v < 1)
			//{
			//	d_v = 1;
			//}
			//d_dx += d_dt * d_v;

			long dt;
			fraction_t df = simtime_factor * (f - adverse.cf * (v * v * sgn<sint32>(v.integer())));
			if (delta_t >= DT_SLICE && (sint32)abs(df.integer()) > weight.weight / (10 * DT_SLICE_SECONDS))
			{
				// This part is important for acceleration/deceleration phases only.
				// When a small force produces a small speed change, we can add it at once in the 'else' section.
				//count2++;
				v += df * fraction_t(DT_SLICE_SECONDS, weight.weight);
				dt = DT_SLICE;
			}
			else
			{
				//count3++;
				v += df * fraction_t(delta_t, DT_TIME_FACTOR * weight.weight);
				dt = delta_t;
			}
			if (is_breaking)
			{
				if (v < vmax)
				{
					v = vmax;
				}
			}
			else if (/* is_breaking */ f < 0 && v < 1)
			{
				v = 1;
			}
			dx += dt * v;
			delta_t -= dt; // another DT_SLICE_SECONDS passed
		}
		akt_speed = v_to_speed(v).integer(); // akt_speed in simutrans vehicle speed, v in m/s
		dx = x_to_steps(dx); // dx in simutrans vehicle speed, v in m/s
	}
	sint32 ds = dx.integer();
	if (ds < KMH_SPEED_UNLIMITED - sp_soll)
	{
		sp_soll += ds;
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
	return ((force * world.get_einstellungen()->get_global_power_factor_percent()) / GEAR_FACTOR + 50) / 100;
}


sint32 potential_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		power += vehicles[i]->get_effective_power_index(speed);
	}
	return ((power * world.get_einstellungen()->get_global_power_factor_percent()) / GEAR_FACTOR + 50) / 100;
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
	return ((force * convoy.get_welt()->get_einstellungen()->get_global_power_factor_percent()) / GEAR_FACTOR + 50) / 100;
}


sint32 existing_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		power += convoy.get_vehikel(i)->get_besch()->get_effective_power_index(speed);
	}
	return ((power * convoy.get_welt()->get_einstellungen()->get_global_power_factor_percent()) / GEAR_FACTOR + 50) / 100;
}

