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

static const float32e8_t g_accel((uint32) 980665, (uint32) 100000); // gravitational acceleration

static const float32e8_t  _999_permille((uint32) 999, (uint32) 1000);
static const float32e8_t _1001_permille((uint32)1001, (uint32) 1000);

static const float32e8_t  _90_percent((uint32)  90, (uint32) 100);
static const float32e8_t  _99_percent((uint32)  99, (uint32) 100);
static const float32e8_t _101_percent((uint32) 101, (uint32) 100);
static const float32e8_t _110_percent((uint32) 110, (uint32) 100);

static const float32e8_t milli((uint32) 1, (uint32) 1000);
//static const float32e8_t thousand((uint32) 1000, (uint32) 1);
static const float32e8_t million((uint32) 1000000);

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
	if (v.is_first()) 
	{
		cf = v.get_besch()->get_air_resistance();
	}

	fr = v.get_besch()->get_rolling_resistance();

	// The vehicle may be limited by braking approaching a station
	// Or airplane circling for landing, or airplane height,
	// Or cornering, or other odd cases
	// These are carried in vehikel_t unlike other speed limits 
	if (v.get_speed_limit() == vehikel_t::speed_unlimited() ) 
	{
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
	//	weight_cos += kgs * sqrt(1000000.0 - sin_alpha * sin_alpha); // Remember: sin(alpha)^2 + cos(alpha)^2 = 1
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
	const float32e8_t p3 =  Frs / (float32e8_t::three * adverse.cf);
	const float32e8_t q2 = get_continuous_power() / (float32e8_t::two * adverse.cf);
	const float32e8_t sd = signed_power(q2 * q2 + p3 * p3 * p3, float32e8_t::half);
	const float32e8_t vmax = signed_power(q2 + sd, float32e8_t::third) + signed_power(q2 - sd, float32e8_t::third); 
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
	return abs(min(f, float32e8_t(get_starting_force())) / (_101_percent * g_accel * (adverse.fr + milli * sin_alpha)));
}

sint32 convoy_t::calc_max_starting_weight(sint32 sin_alpha)
{
	return abs(get_starting_force() / (_101_percent * g_accel * (adverse.fr + milli * sin_alpha))); // 1.01 to compensate inaccuracy of calculation 
}

// The timeslice to calculate acceleration, speed and covered distance in reasonable small chuncks. 
#define DT_TIME_FACTOR 64
#define DT_SLICE_SECONDS 2
#define DT_SLICE (DT_TIME_FACTOR * DT_SLICE_SECONDS)
const float32e8_t fl_time_factor = float32e8_t(1, DT_TIME_FACTOR); 
const float32e8_t fl_slice_seconds = float32e8_t(DT_SLICE_SECONDS, 1);

sint32 convoy_t::calc_min_braking_distance(const weight_summary_t &weight, const float32e8_t &v)
{
	// breaking distance: x = 1/2 at². 
	// with a = v/t, v = at, and v² = a²t² --> x = 1/2 v²/a.
	// with F = ma, a = F/m --> x = 1/2 v²m/F.
	// This equation is a rough estimation:
	// - it does not take into account, that Ff depends on v (getting a differential equation).
	// - Frs depends on the inclination of the way. The above Frs is asnapshot of the current position only.

	// Therefore and because the actual braking lasts longer than this estimation predicts anyway, we ignore Frs and Ff here:
	const float32e8_t vv = v * v; // v in m/s
	const float32e8_t Frs = g_accel * (adverse.fr * weight.weight_cos + weight.weight_sin); // Frs in N
	//const float32e8_t Ff = sgn(v) * adverse.cf * vv; // Frs in N
	const float32e8_t F = get_braking_force(v.to_sint32()); // f in N
	return (weight.weight / 2) * (vv / (F + Frs /*+ Ff*/)); // min braking distance in m
}

sint32 convoy_t::calc_min_braking_distance(const float32e8_t &simtime_factor, const weight_summary_t &weight, sint32 speed)
{
	const float32e8_t x = calc_min_braking_distance(weight, speed_to_v(speed)) * _110_percent;
	const sint32 yards = v_to_speed(x / simtime_factor) * DT_TIME_FACTOR;
	return yards >> YARDS_PER_VEHICLE_STEP_SHIFT;
}

void convoy_t::calc_move(long delta_t, const float32e8_t &simtime_factor, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 &akt_speed, sint32 &sp_soll)
{
	sint32 max_speed = min(adverse.max_speed, vehicle.max_speed);
	max_speed = kmh_to_speed(max_speed);
	if (akt_speed_soll > max_speed)
	{
		akt_speed_soll = max_speed;
	}

	if (delta_t > 1800 * DT_TIME_FACTOR)
	{
		// After 30 minutes any vehicle has reached its akt_speed_soll. 
		// Shorten the process. 
		sint32 new_speed = min(akt_speed_soll, kmh_to_speed(calc_max_speed(weight)));
		if (new_speed > max_speed)
		{
			new_speed = max_speed;
		}
		sp_soll += (sint32)delta_t * (akt_speed + new_speed) / 2; // assume average speed. Otherwize in case of braking (akt_speed_soll == 0) the convoy won't move at all.
		akt_speed = new_speed;
	}
	else
	{
		const float32e8_t fweight = weight.weight; // convoy's weight in kg
		const float32e8_t Frs = g_accel * (adverse.fr * weight.weight_cos + weight.weight_sin); // Frs in N, weight.weight_cos and weight.weight_sin are calculated per vehicle due to vehicle specific slope angle.
		const float32e8_t vsoll = speed_to_v(akt_speed_soll); // vsoll in m/s, akt_speed_soll in simutrans vehicle speed.
		float32e8_t fvsoll = 0; // force in N needed to hold vsoll. calculated when needed.
		float32e8_t speed_ratio = 0; // requested speed / convoy's max speed. calculated when needed.
		float32e8_t dx = 0; // covered distance in m
		float32e8_t v = speed_to_v(akt_speed); // v in m/s, akt_speed in simutrans vehicle speed.
		// iterate the passed time.
		while (delta_t > 0)
		{
			const float32e8_t Ff = sgn(v) * adverse.cf * v * v;

			// 1) The driver's part: select the force:
			float32e8_t f;
			bool is_braking = akt_speed_soll <= SPEED_MIN; // don't roll backwards, due to braking
			if (is_braking)
			{
				f = -get_braking_force(v.to_sint32()) - Frs;
			}
			else if (v < _90_percent * vsoll)
			{
				// Below set speed: full acceleration
				// If set speed is far below the convoy max speed as e.g. aircrafts on ground reduce force.
				// If set speed is at most a 10th of convoy's maximum, we reduce force to its 10th.
				f = get_force(v) - Frs;
				if (f > million) // reducing force does not apply to 'weak' convoy's, thus we can save a lot of time skipping this code.
				{
					if (speed_ratio == 0) // speed_ratio is a constant within this function. So calculate it once only.
					{
						speed_ratio = ms2kmh * vsoll / vehicle.max_speed;
					}
					if (speed_ratio < float32e8_t::tenth)
					{
						if (fvsoll == 0) // fvsoll is a constant within this function. So calculate it once only.
						{
							fvsoll = calc_speed_holding_force(vsoll, Frs);
						}
						if (f > fvsoll)
						{
							f = (f - fvsoll) * float32e8_t::tenth + fvsoll;
						}
					}
				}
			}
			else if (v <= vsoll)
			{
				f = get_force(v) - Frs;
				if (fvsoll == 0) // fvsoll is a constant within this function. So calculate it once only.
				{
					fvsoll = calc_speed_holding_force(vsoll, Frs);
				}
				if (f > fvsoll)
				{
					f = (f - fvsoll) * float32e8_t::half + fvsoll;
				}
			}
			//else if (v < _101_percent * vsoll)
			//{
			//	// at or slightly above set speed: hold this speed
			//	if (fvsoll == 0) // fvsoll is a constant within this function. So calculate it once only.
			//	{
			//		fvsoll = calc_speed_holding_force(vsoll, Frs);
			//	}
			//	f = fvsoll;
			//}
			else if (v < _110_percent * vsoll)
			{
				// slightly above set speed: coasting 'til back to set speed.
				f = -Frs;
			}
			else
			{
				is_braking = true;
				// running too fast, slam on the brakes! 
				// hill-down Frs might become negative and works against the brake.
				f = -get_braking_force(v.to_sint32()) - Frs;
			}
			f -= Ff;
			f *= simtime_factor;

			// 2) The "differential equation" part: calculate new speed:
			uint32 dt;
			float32e8_t dt_s;
			if (delta_t >= DT_SLICE && abs(f.to_sint32()) > weight.weight / (10 * DT_SLICE_SECONDS))
			{
				// This part is important for acceleration/deceleration phases only. 
				// If the force to weight ratio exceeds a certain level, then we must calculate speed iterative,
				// as it depends on previous speed.
				dt = DT_SLICE;
				dt_s = fl_slice_seconds;
			}
			else
			{
				// If a small force produces a small speed change, we can add the difference at once in the 'else' section
				// with a disregardable inaccuracy.
				dt = delta_t;
				dt_s = fl_time_factor * dt;
			}
			float32e8_t v0 = v;
			v += (dt_s * f) / weight.weight;
			if (is_braking)
			{
				if (v < vsoll)
				{
					v = vsoll;
				}
			}
			else if (/* is_braking */ f < 0 && v < 1)
			{
				v = 1;
			}
			else if (v > vsoll)	
			{
				v = vsoll;
			}
			dx += v * dt_s;
			delta_t -= dt; // another time slice passed
		}
		akt_speed = v_to_speed(v); // akt_speed in simutrans vehicle speed, v in m/s
		sp_soll += v_to_speed(dx * DT_TIME_FACTOR); // sp_soll in simutrans steps, dx in m
	}
	// BG, 26.09.2011: NOTICE: sp_soll is not a speed, but the number of steps to move. 
	//  Reducing it to KMH_SPEED_UNLIMITED may have solved a problem once upon a time, but looks weird.
	//if (sp_soll > KMH_SPEED_UNLIMITED)
	//{
	//	sp_soll = KMH_SPEED_UNLIMITED;
	//}
	//akt_speed = min(akt_speed, kmh_to_speed(adverse.max_speed));
}

sint32 convoy_t::power_index_to_power(sint32 power_index, sint32 power_factor)
{
	// avoid integer overflow for large powers
	if (power_index >= ((sint32)2147483647L) / power_factor)
	{
		return ((power_index / 100) * power_factor + GEAR_FACTOR / 2) / GEAR_FACTOR;
	}
	return (power_index * power_factor + 50 * GEAR_FACTOR) / (100 * GEAR_FACTOR);
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

sint32 potential_convoy_t::get_brake_summary(const sint32 speed /* in m/s */)
{
	sint32 force = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		const vehikel_besch_t &b = *vehicles[i];
		uint16 bf = b.get_brake_force();
		if (bf != BRAKE_FORCE_UNKNOWN)
		{
			force += bf;
		}
		else
		{
			// Usual brake deceleration is about -0.5 .. -1.5 m/s² depending on vehicle and ground. 
			// With F=ma, a = F/m follows that brake force in N is ~= 1/2 weight in kg
			force += get_adverse_summary().br * (uint32) b.get_gewicht();
		}
	}
	return force;
}


sint32 potential_convoy_t::get_force_summary(sint32 speed /* in m/s */)
{
	sint32 force = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		force += vehicles[i]->get_effective_force_index(speed);
	}
	return power_index_to_power(force, world.get_settings().get_global_power_factor_percent());
}


sint32 potential_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		power += vehicles[i]->get_effective_power_index(speed);
	}
	return power_index_to_power(power, world.get_settings().get_global_power_factor_percent());
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


sint32 existing_convoy_t::get_brake_summary(const sint32 speed /* in m/s */)
{
	sint32 force = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		vehikel_t &v = *convoy.get_vehikel(i);
		uint16 bf = v.get_besch()->get_brake_force();
		if (bf != BRAKE_FORCE_UNKNOWN)
		{
			force += bf;
		}
		else
		{
			// Usual brake deceleration is about -0.5 .. -1.5 m/s² depending on vehicle and ground. 
			// With F=ma, a = F/m follows that brake force in N is ~= 1/2 weight in kg
			force += get_adverse_summary().br * float32e8_t(v.get_gesamtgewicht(), 1000);
		}
	}
	return force;
}


sint32 existing_convoy_t::get_force_summary(sint32 speed /* in m/s */)
{
	sint32 force = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		force += convoy.get_vehikel(i)->get_besch()->get_effective_force_index(speed);
	}
	return power_index_to_power(force, convoy.get_welt()->get_settings().get_global_power_factor_percent());
}


sint32 existing_convoy_t::get_power_summary(sint32 speed /* in m/s */)
{
	sint32 power = 0;
	for (uint16 i = convoy.get_vehikel_anzahl(); i-- > 0; )
	{
		power += convoy.get_vehikel(i)->get_besch()->get_effective_power_index(speed);
	}
	return power_index_to_power(power, convoy.get_welt()->get_settings().get_global_power_factor_percent());
}