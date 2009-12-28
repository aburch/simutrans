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
inline double double_min(double a, double b)
{
	return a < b ? a : b;
}

// helps to calculate roots. pow fails to calculate roots of negative bases.
inline double signed_power(double base, double expo)
{
	if (base >= 0)
		return pow(base, expo);
	return -pow(-base, expo);
}


static void get_possible_freight_weight(uint8 catg_index, uint32 &min_weight, uint32 &max_weight)
{
	max_weight = 0;
	min_weight = INT_MAX;
	for (uint16 j=0; j<warenbauer_t::get_waren_anzahl(); j++) 
	{
		const ware_besch_t &ware = *warenbauer_t::get_info(j);
		if (ware.get_catg_index() == catg_index) 
		{
			uint32 weight = ware.get_weight_per_unit();
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
	if (min_weight == INT_MAX) 
	{
		min_weight = 0;
	}
}

/******************************************************************************/

void adverse_summary_t::add_vehicle(const vehikel_t &v)
{
	const waytype_t waytype = v.get_waytype();
	if (max_speed == INT_MAX)
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
				sint32 limit = (uint32) way->get_max_speed();
				if (max_speed > limit)
				{
					max_speed = limit;
				}
			}
		}
	}
}

/******************************************************************************/

void freight_summary_t::add_vehicle(const vehikel_besch_t &b)
{
	const uint32 payload = b.get_zuladung();
	if (payload > 0)
	{
		uint32 min_weight, max_weight;
		get_possible_freight_weight(b.get_ware()->get_catg_index(), min_weight, max_weight);
		min_freight_weight += min_weight * payload;
		max_freight_weight += max_weight * payload;
	}
}

/******************************************************************************/

void weight_summary_t::add_weight(uint32 tons, sint32 sin_alpha)
{
	sint32 kgs = tons * 1000;
	weight += kgs; 
	// v.get_frictionfactor() between about -14 (downhill) and 50 (uphill). 
	// Including the factor 1000 for tons to kg conversion, 50 corresponds to an inclination of 28 per mille.
	// sin_alpha == 1000 would be 90 degrees == vertically up, -1000 vertically down.
	if (sin_alpha != 0)
	{
		weight_sin += (sint32)tons * sin_alpha;
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
	const double Frs = 9.81 * (adverse.fr * weight.weight_cos + weight.weight_sin);
	if (Frs > get_force(0)) 
	{
		// this convoy is too heavy to start.
		return 0;
	}
	const double p3 =  Frs / (3.0 * adverse.cf);
	const double q2 = vehicle.power / (0.002 * adverse.cf);
	const double sd = signed_power(q2 * q2 + p3 * p3 * p3, 1.0/2.0);
	const double vmax = (signed_power(q2 + sd, 1.0/3.0) + signed_power(q2 - sd, 1.0/3.0)); 
	return min(vehicle.max_speed, (sint32)(vmax * 3.6 + 1.0)); // 1.0 to compensate inaccuracy of calculation and make sure this is at least what calc_move() evaluates.
}

uint32 convoy_t::calc_max_weight()
{
	if (vehicle.max_speed == 0)
		return 0;
	const double v = vehicle.max_speed * (1.0/3.6); // from km/h to m/s
	return (uint32)((vehicle.power * 1000 - adverse.cf * v * v * v) / (adverse.fr * 9.81 * v));
}

double convoy_t::calc_speed_holding_force(double speed /* in m/s */, double Frs /* in N */)
{
	return double_min(adverse.cf * speed * speed, get_force(speed) - Frs); /* in N */
}

// The timeslice to calculate acceleration, speed and covered distance in reasonable small chuncks. 
#define DT_TIME_FACTOR 64
#define DT_SLICE_SECONDS 2
#define DT_SLICE (DT_TIME_FACTOR * DT_SLICE_SECONDS)

void convoy_t::calc_move(long delta_t, float simtime_factor, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll)
{
	double dx = 0;
	if (adverse.max_speed < INT_MAX)
	{
		sint32 speed_limit = kmh_to_speed(adverse.max_speed);
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
		const double Frs = 9.81 * (adverse.fr * weight.weight_cos + weight.weight_sin); // msin, mcos are calculated per vehicle due to vehicle specific slope angle.
		const double vmax = speed_to_v(akt_speed_soll);
		double v = speed_to_v(akt_speed); // v in m/s, akt_speed in simutrans vehicle speed;
		double fvmax = 0; // force needed to hold vmax. will be calculated as needed
		double f0 = 0; // maximum force at speed 0. will be calculated as needed
		double speed_ratio = 0; 
		//static uint32 count1 = 0;
		//static uint32 count2 = 0;
		//static uint32 count3 = 0;
		//count1++;
		// iterate the passed time.
		while (delta_t > 0)
		{
			// the driver's part: select accelerating force:
			double f;
			bool is_breaking = false; // don't roll backwards, due to breaking
			if (v < 0.999 * vmax)
			{
				// Below set speed: full acceleration
				// If set speed is far below the convoy max speed as e.g. aircrafts on ground reduce force.
				// If set speed is at most a 10th of convoy's maximum, we reduce force to its 10th.
				f = get_force(v) - Frs;
				if (f > 1000000.0) // reducing force does not apply to 'weak' convoy's, thus we can save a lot of time skipping this code.
				{
					if (speed_ratio == 0) // speed_ratio is a constant within this function. So calculate it once only.
					{
						speed_ratio = 3.6 * vmax / vehicle.max_speed;
					}
					if (speed_ratio < 0.1)
					{
						fvmax = calc_speed_holding_force(vmax, Frs);
						if (f > fvmax)
						{
							f = (f - fvmax) * 0.1 + fvmax;
						}
					}
				}
			}
			else if (v < 1.001 * vmax)
			{
				// at or slightly above set speed: hold this speed
				if (fvmax == 0) // fvmax is a constant within this function. So calculate it once only.
				{
					fvmax = calc_speed_holding_force(vmax, Frs);
				}
				f = fvmax;
			}
			else if (v < 1.1 * vmax)
			{
				// slightly above set speed: coasting 'til back to set speed.
				f = - Frs;
			}
			else if (v < 1.5 * vmax)
			{
				is_breaking = true;
				// running too fast, apply the breaks! 
				// hill-down Frs might become negative and works against the brake.
				if (f0 == 0) // f0 is a constant within this function. So calculate it once only.
				{
					f0 = get_force(0);
				}
				f = - f0 - Frs;
			}
			else
			{
				is_breaking = true;
				// running much too fast, slam on the brakes! 
				// assuming the brakes are up to 5 times stronger than the start-up force.
				// hill-down Frs might become negative and works against the brake.
				if (f0 == 0) // f0 is a constant within this function. So calculate it once only.
				{
					f0 = get_force(0);
				}
				f = -5 * f0 - Frs;
			}

			// accelerate: calculate new speed according to acceleration within the passed second(s).
			long dt;
			double df = simtime_factor * (f - sgn(v) * adverse.cf * v * v);
			if (delta_t >= DT_SLICE && (uint32)abs(df) > weight.weight / (10 * DT_SLICE_SECONDS))
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
				v += (delta_t * df) / (DT_TIME_FACTOR * weight.weight); 
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
		akt_speed = v_to_speed(v); // akt_speed in simutrans vehicle speed, v in m/s
		dx = x_to_steps(dx);
	}
	if (dx < INT_MAX - sp_soll)
		sp_soll += (sint32) dx;
	else
		sp_soll = INT_MAX;
}

/******************************************************************************/

void potential_convoy_t::update_vehicle_summary(vehicle_summary_t &vehicle)
{		
	vehicle.clear();
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		const vehikel_besch_t &b = *vehicles[i];
		vehicle.length += b.get_length();
		vehicle.max_speed = min(vehicle.max_speed, (uint32) b.get_geschw());
		vehicle.power += b.get_leistung() * b.get_gear();
		vehicle.weight += b.get_gewicht();
	}
	vehicle.power = (uint32)(vehicle.power * world.get_einstellungen()->get_global_power_factor() + (GEAR_FACTOR/2)) / GEAR_FACTOR;
	vehicle.weight *= 1000;
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


uint32 potential_convoy_t::get_force_summary(uint16 speed /* in m/s */)
{
	uint32 force = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		force += vehicles[i]->get_effective_force_index(speed);
	}
	return (uint32)(force * world.get_einstellungen()->get_global_power_factor() * (1.0f/GEAR_FACTOR) + 0.5f);
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
	vehicle.length = convoy.get_length();
	vehicle.max_speed = speed_to_kmh(convoy.get_min_top_speed());
	vehicle.power = (convoy.get_power_index() + (GEAR_FACTOR/2)) / GEAR_FACTOR;
	vehicle.weight = convoy.get_sum_gewicht() * 1000;
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

uint32 existing_convoy_t::get_force_summary(uint16 speed /* in m/s */)
{
	uint32 force = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		force += convoy.get_vehikel(i)->get_besch()->get_effective_force_index(speed);
	}
	return (uint32)(force * convoy.get_welt()->get_einstellungen()->get_global_power_factor()  * (1.0f/GEAR_FACTOR) + 0.5f);
}

