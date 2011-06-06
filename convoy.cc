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

static const float32e8_t g_accel((uint32)980665L, (uint32)100000L); // gravitational acceleration

static const float32e8_t  half((uint32) 1, (uint32)  2);
static const float32e8_t third((uint32) 1, (uint32)  3);
static const float32e8_t tenth((uint32) 1, (uint32) 10);

static const float32e8_t milli((uint32) 1, (uint32) 1000);
static const float32e8_t kilo((uint32) 1000, (uint32) 1);

// some unit conversions:
static const float32e8_t kmh2ms((uint32) 10, (uint32) 36);
static const float32e8_t ms2kmh((uint32) 36, (uint32) 10);


// make sure, that no macro calculates a or b twice:
inline const float32e8_t double_min(const float32e8_t &a, const float32e8_t &b)
{
	return a < b ? a : b;
}

// helps to calculate roots. pow fails to calculate roots of negative bases.
inline const float32e8_t signed_power(const float32e8_t &base, const float32e8_t &expo)
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
		cf = v.get_besch()->get_air_resistance();
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
	const float32e8_t Frs = g_accel * (adverse.fr * weight.weight_cos + weight.weight_sin);
	if (Frs > get_starting_force()) 
	{
		// this convoy is too heavy to start.
		return 0;
	}
	const float32e8_t p3 =  Frs / (float32e8_t((uint32) 3) * adverse.cf);
	const float32e8_t q2 = get_continuous_power() / (float32e8_t((uint32) 2) * adverse.cf);
	const float32e8_t sd = signed_power(q2 * q2 + p3 * p3 * p3, half);;
	const float32e8_t vmax = signed_power(q2 + sd, third) + signed_power(q2 - sd, third); 
	return min(vehicle.max_speed, (sint32)(vmax * ms2kmh + float32e8_t::one)); // 1.0 to compensate inaccuracy of calculation and make sure this is at least what calc_move() evaluates.
}

sint32 convoy_t::calc_max_weight(sint32 sin_alpha)
{
	if (vehicle.max_speed == 0)
		return 0;
	const float32e8_t v = vehicle.max_speed * kmh2ms; // from km/h to m/s
	const float32e8_t f = get_continuous_power() / v - adverse.cf * v * v;
	if (f <= 0)
	{
		return 0;
	}
	return abs(double_min(get_starting_force(), f) / (g_accel * (adverse.fr + milli * sin_alpha)));
}

sint32 convoy_t::calc_max_starting_weight(sint32 sin_alpha)
{
	return abs(get_starting_force() / (/*1.01* */ g_accel * (adverse.fr + milli * sin_alpha))); // 1.01 to compensate inaccuracy of calculation 
}

float32e8_t convoy_t::calc_speed_holding_force(float32e8_t speed /* in m/s */, float32e8_t Frs /* in N */)
{
	return double_min(adverse.cf * speed * speed, get_force(speed) - Frs); /* in N */
}

// The timeslice to calculate acceleration, speed and covered distance in reasonable small chuncks. 
#define DT_TIME_FACTOR 64
#define DT_SLICE_SECONDS 2
#define DT_SLICE (DT_TIME_FACTOR * DT_SLICE_SECONDS)

void convoy_t::calc_move(long delta_t, const float32e8_t &simtime_factor, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll)
{
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
		sp_soll += (sint32)delta_t * akt_speed;
	}
	else
	{
		float32e8_t dx = 0;
		const float32e8_t Frs = g_accel * (adverse.fr * weight.weight_cos + weight.weight_sin); // msin, mcos are calculated per vehicle due to vehicle specific slope angle.
		const float32e8_t vmax = speed_to_v(akt_speed_soll);
		float32e8_t v = speed_to_v(akt_speed); // v in m/s, akt_speed in simutrans vehicle speed;
		float32e8_t fvmax = 0; // force needed to hold vmax. will be calculated as needed
		float32e8_t speed_ratio = 0; 
		//static uint32 count1 = 0;
		//static sint32 count2 = 0;
		//static sint32 count3 = 0;
		//count1++;
		// iterate the passed time.
		while (delta_t > 0)
		{
			// the driver's part: select accelerating force:
			float32e8_t f;
			bool is_breaking = false; // don't roll backwards, due to breaking
			if (v < float32e8_t((uint32)999, (uint32)1000) * vmax)
			{
				// Below set speed: full acceleration
				// If set speed is far below the convoy max speed as e.g. aircrafts on ground reduce force.
				// If set speed is at most a 10th of convoy's maximum, we reduce force to its 10th.
				f = get_force(v) - Frs;
				if (f > float32e8_t((uint32)1000000)) // reducing force does not apply to 'weak' convoy's, thus we can save a lot of time skipping this code.
				{
					if (speed_ratio == 0) // speed_ratio is a constant within this function. So calculate it once only.
					{
						speed_ratio = ms2kmh * vmax / vehicle.max_speed;
					}
					if (speed_ratio < tenth)
					{
						fvmax = calc_speed_holding_force(vmax, Frs);
						if (f > fvmax)
						{
							f = (f - fvmax) * tenth + fvmax;
						}
					}
				}
			}
			else if (v < float32e8_t((uint32)1001, (uint32)1000) * vmax)
			{
				// at or slightly above set speed: hold this speed
				if (fvmax == 0) // fvmax is a constant within this function. So calculate it once only.
				{
					fvmax = calc_speed_holding_force(vmax, Frs);
				}
				f = fvmax;
			}
			else if (v < float32e8_t((uint32)11, (uint32)10) * vmax)
			{
				// slightly above set speed: coasting 'til back to set speed.
				f = -Frs;
			}
			else if (v < float32e8_t((uint32)15, (uint32)10) * vmax)
			{
				is_breaking = true;
				// running too fast, apply the breaks! 
				// hill-down Frs might become negative and works against the brake.
				f = -(get_starting_force() + Frs);
			}
			else
			{
				is_breaking = true;
				// running much too fast, slam on the brakes! 
				// assuming the brakes are up to 5 times stronger than the start-up force.
				// hill-down Frs might become negative and works against the brake.
				f = -(5 * get_starting_force() + Frs);
			}

			// accelerate: calculate new speed according to acceleration within the passed second(s).
			long dt;
			float32e8_t df = simtime_factor * (f - sgn(v) * adverse.cf * v * v);
			if (delta_t >= DT_SLICE && (sint32)abs(df) > weight.weight / (10 * DT_SLICE_SECONDS))
			{
				// This part is important for acceleration/deceleration phases only.
				// When a small force produces small speed change, we can add it at once in the 'else' section.
				//count2++;
				v += (DT_SLICE_SECONDS * df) / weight.weight; 
				dt = DT_SLICE;
			}
			else
			{
				//count3++;
				v += (float32e8_t((uint32)delta_t) * df) / float32e8_t((sint32)DT_TIME_FACTOR * weight.weight);
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
			dx += v * float32e8_t((sint32)dt);
			delta_t -= dt; // another DT_SLICE_SECONDS passed
		}
		akt_speed = v_to_speed(v); // akt_speed in simutrans vehicle speed, v in m/s
		sp_soll += v_to_speed(dx);
	}
	if (sp_soll > KMH_SPEED_UNLIMITED)
	{
		sp_soll = KMH_SPEED_UNLIMITED;
	}
	akt_speed = min(akt_speed, kmh_to_speed(adverse.max_speed));
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
	return (force * world.get_einstellungen()->get_global_power_factor_percent() + 50 * GEAR_FACTOR) / (100 * GEAR_FACTOR);
}


sint32 potential_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		power += vehicles[i]->get_effective_power_index(speed);
	}
	return (power * world.get_einstellungen()->get_global_power_factor_percent() + 50 * GEAR_FACTOR) / (100 * GEAR_FACTOR);
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
	return (force * convoy.get_welt()->get_einstellungen()->get_global_power_factor_percent() + 50 * GEAR_FACTOR) / (100 * GEAR_FACTOR);
}


sint32 existing_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		power += convoy.get_vehikel(i)->get_besch()->get_effective_power_index(speed);
	}
	return (power * convoy.get_welt()->get_einstellungen()->get_global_power_factor_percent() + 50 * GEAR_FACTOR) / (100 * GEAR_FACTOR);
}

