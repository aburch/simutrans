/*
 * Copyright (c) 2009..2011 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * convoy_t: common collection of properties of a convoi_t and a couple of vehicles which are going to become a convoy_t.
 * While convoi_t is involved in game play, convoy_t is the entity that physically models the convoy.
 */

#include <math.h>
#include "convoy.h"
#include "bauer/goods_manager.h"
#include "vehicle/simvehicle.h"
#include "simconvoi.h"

static karte_ptr_t welt;

static const float32e8_t g_accel((uint32) 980665, (uint32) 100000); // gravitational acceleration

static const float32e8_t _101_percent((uint32) 101, (uint32) 100);
static const float32e8_t _110_percent((uint32) 110, (uint32) 100);

static const float32e8_t million((uint32) 1000000);

const float32e8_t BR_AIR = float32e8_t(2, 1);
const float32e8_t BR_WATER = float32e8_t(1, 10);
const float32e8_t BR_TRACK = float32e8_t(1, 2);
const float32e8_t BR_TRAM = float32e8_t(1, 1);
const float32e8_t BR_MAGLEV = float32e8_t(12, 10);
const float32e8_t BR_ROAD = float32e8_t(4, 1);
const float32e8_t BR_DEFAULT = float32e8_t(1, 1);

// helps to calculate roots. pow fails to calculate roots of negative bases.
inline const float32e8_t signed_power(const float32e8_t &base, const float32e8_t &expo)
{
	if (base >= float32e8_t::zero)
		return pow(base, expo);
	return -pow(-base, expo);
}

static void get_possible_freight_weight(uint8 catg_index, sint32 &min_weight, sint32 &max_weight)
{
	max_weight = 0;
	min_weight = WEIGHT_UNLIMITED;
	for (uint16 j=0; j<goods_manager_t::get_count(); j++) 
	{
		const goods_desc_t &ware = *goods_manager_t::get_info(j);
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

void adverse_summary_t::add_vehicle(const vehicle_t &v)
{
	add_vehicle(*v.get_desc(), v.is_leading());

	const waytype_t waytype = v.get_waytype();
	if (waytype != air_wt || ((const air_vehicle_t &)v).get_flyingheight() <= 0)
	{
		weg_t *way = v.get_weg();
		if (way)
		{
			max_speed = min(max_speed, way->get_max_speed());
		}
	}

	// The vehicle may be limited by braking approaching a station
	// Or airplane circling for landing, or airplane height,
	// Or cornering, or other odd cases
	// These are carried in vehicle_t unlike other speed limits 
	if (v.get_speed_limit() != vehicle_t::speed_unlimited()) 
	{
		max_speed = min(max_speed, speed_to_kmh(v.get_speed_limit()));
	}
}


void adverse_summary_t::add_vehicle(const vehicle_desc_t &b, bool is_leading)
{
	if (br.is_zero())
	{
		switch (b.get_waytype())
		{
			case air_wt:
				br = BR_AIR;
				break;

			case water_wt:
				br = BR_WATER;
				break;
		
			case track_wt:
			case narrowgauge_wt:
			case overheadlines_wt: 
				br = BR_TRACK;
				break;

			case tram_wt:
			case monorail_wt:      
				br = BR_TRAM;
				break;
			
			case maglev_wt:
				br = BR_MAGLEV;
				break;

			case road_wt:
				br = BR_ROAD;
				break;

			default:
				br = BR_DEFAULT;
				break;
		}
	}

	if (is_leading) 
	{
		cf = b.get_air_resistance();
	}

	fr += b.get_rolling_resistance();
}

/******************************************************************************/

void freight_summary_t::add_vehicle(const vehicle_desc_t &b)
{
	const sint32 payload = b.get_total_capacity();
	if (payload > 0)
	{
		sint32 min_weight, max_weight;
		get_possible_freight_weight(b.get_freight_type()->get_catg_index(), min_weight, max_weight);
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

void weight_summary_t::add_vehicle(const vehicle_t &v)
{
	// v.get_frictionfactor() between about -14 (downhill) and 50 (uphill). 
	// Including the factor 1000 for tons to kg conversion, 50 corresponds to an inclination of 28 per mille.
	add_weight(v.get_total_weight(), v.get_frictionfactor());
}

/******************************************************************************/

sint32 convoy_t::calc_max_speed(const weight_summary_t &weight) 
{ 
	const float32e8_t Frs = g_accel * (get_adverse_summary().fr * weight.weight_cos + weight.weight_sin);
	if (Frs > get_starting_force()) 
	{
		// this convoy is too heavy to start.
		return 0;
	}
	
	// this iterative version is a bit more precise as it uses the correct speed.
	/*
	float32e8_t v = vehicle_summary.max_speed * kmh2ms;
	float32e8_t F = get_force(v);
	float32e8_t Ff = adverse.cf * v * v;
	if (Frs + Ff > F)
	{
		// this convoy is too weak for its maximum allowed speed.
		float32e8_t vmax = v;
		float32e8_t vmin = 0;
		while (sgn(vmax - vmin, float32e8_t::milli))
		{
			v = (vmax + vmin) * float32e8_t::half;
			F = get_force(v);
			Ff = adverse.cf * v * v;
			float32e8_t Frsf = Frs + Ff;
			int sign = sgn(Frsf - F, float32e8_t::milli);
			switch (sign)
			{
				case 1:
					// too weak for this speed
					vmax = v;
					continue;

				case -1:
					// has force for more speed
					vmin = v;
					continue;
			}
			break;
		}
	}
	return (v * ms2kmh + float32e8_t::half).to_sint32();
	*/
	
	// this version approximates the result. It ignores that get_continuous_power() depends on vehicle.max_speed,
	// which results in less power, than actually is present at lower speed.
	const float32e8_t p3 = Frs / (float32e8_t::three * adverse.cf);
	const float32e8_t q2 = get_continuous_power() / (float32e8_t::two * adverse.cf);
	const float32e8_t sd = signed_power(q2 * q2 + p3 * p3 * p3, float32e8_t::half);
	const float32e8_t vmax = signed_power(q2 + sd, float32e8_t::third) + signed_power(q2 - sd, float32e8_t::third); 
	return min(vehicle_summary.max_speed, (sint32)(vmax * ms2kmh + float32e8_t::one)); // 1.0 to compensate inaccuracy of calculation and make sure this is at least what calc_move() evaluates.
}

sint32 convoy_t::calc_max_weight(sint32 sin_alpha)
{
	if (vehicle_summary.max_speed == 0)
		return 0;
	const float32e8_t v = vehicle_summary.max_speed * kmh2ms; // from km/h to m/s
	const float32e8_t f = get_continuous_power() / v - adverse.cf * v * v;
	if (f <= 0)
	{
		return 0;
	}
	return abs(min(f, float32e8_t(get_starting_force())) / (_101_percent * g_accel * (adverse.fr + float32e8_t::milli * sin_alpha)));
}

sint32 convoy_t::calc_max_starting_weight(sint32 sin_alpha)
{
	return abs(get_starting_force() / (_101_percent * g_accel * (adverse.fr + float32e8_t::milli * sin_alpha))); // 1.01 to compensate inaccuracy of calculation 
}

// The timeslice to calculate acceleration, speed and covered distance in reasonable small chuncks. 
#define DT_SLICE_SECONDS 2
#define DT_SLICE (DT_TIME_FACTOR * DT_SLICE_SECONDS)
//static const float32e8_t fl_time_factor(DT_TIME_FACTOR, 1);
//static const float32e8_t fl_time_divisor(1, DT_TIME_FACTOR);
static const float32e8_t fl_slice_seconds(DT_SLICE_SECONDS);
static const float32e8_t fl_max_seconds_til_vsoll(1800);

float32e8_t convoy_t::calc_min_braking_distance(const weight_summary_t &weight, const float32e8_t &v)
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
	const float32e8_t F = get_braking_force(/*v*/); // f in N
	return (weight.weight / 2) * (vv / (F + Frs /*+ Ff*/)); // min braking distance in m
}

sint32 convoy_t::calc_min_braking_distance(const settings_t &settings, const weight_summary_t &weight, sint32 speed)
{
	const float32e8_t x = calc_min_braking_distance(weight, speed_to_v(speed)) * _110_percent;
	return settings.meters_to_steps(x);
}

double convoy_t::calc_acceleration_time(const weight_summary_t &weight, sint32 speed)
{
	if (!weight.weight || !speed) { return 0.0; }
	double total_sec = 0;
	for (int i = 1; i < speed; i++) {
		const uint32 a = (get_force_summary(i * kmh2ms) - calc_speed_holding_force(i * kmh2ms, get_adverse_summary().fr)).to_sint32() * 1000 / 9.80665 / 30.9 / weight.weight * 100;
		if (!a) { return 0.0; /* given speed error */ }
		const double delta_t = (double)100.0 / a;
		total_sec += delta_t;
	}
	return total_sec;
}

uint32 convoy_t::calc_acceleration_distance(const weight_summary_t &weight, sint32 speed)
{
	if (!weight.weight || !speed) { return 0.0; }
	uint64 travel_distance = 0;
	for (int i = 1; i < speed; i++) {
		const uint32 a = (get_force_summary(i * kmh2ms) - calc_speed_holding_force(i * kmh2ms, get_adverse_summary().fr)).to_sint32() * 1000 / 9.80665 / 30.9 / weight.weight * 100;
		if (!a) { return 0; /* given speed error */ }
		const double delta_t = (double)100.0 / a;
		travel_distance += delta_t * (i - 0.5) * 1000 / 3600 * 100; // [cm]
	}
	return travel_distance/100; // in meter
}

inline float32e8_t _calc_move(const float32e8_t &a, const float32e8_t &t, const float32e8_t &v0)
{
	return (float32e8_t::half * a * t + v0) * t;
}

void convoy_t::calc_move(const settings_t &settings, long delta_t, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 next_speed_limit, sint32 steps_til_limit, sint32 steps_til_brake, sint32 &akt_speed, sint32 &sp_soll, float32e8_t &akt_v)
{
	float32e8_t delta_s = settings.ticks_to_seconds(delta_t);
	if (delta_s >= fl_max_seconds_til_vsoll)
	{
		// After fl_max_seconds_til_vsoll any vehicle has reached its akt_speed_soll. 
		// Shorten the process. 
		akt_speed = min(max(akt_speed_soll, akt_speed), kmh_to_speed(calc_max_speed(weight)));
		akt_v = speed_to_v(akt_speed);
		sp_soll += (sint32)(settings.meters_to_steps(delta_s * akt_v) * steps2yards); // sp_soll in simutrans yards, dx in m
	}
	else
	{
		const float32e8_t fweight = weight.weight; // convoy's weight in kg
		const float32e8_t Frs = g_accel * (get_adverse_summary().fr * weight.weight_cos + weight.weight_sin); // Frs in N, weight.weight_cos and weight.weight_sin are calculated per vehicle due to vehicle specific slope angle.
		const float32e8_t vlim = speed_to_v(next_speed_limit); // vlim in m/s, next_speed_limit in simutrans vehicle speed.
		const float32e8_t xlim = settings.steps_to_meters(steps_til_limit); // xbrk in m, steps_til_limit in simutrans steps
		const float32e8_t xbrk = settings.steps_to_meters(steps_til_brake); // xbrk in m, steps_til_brake in simutrans steps
		//float32e8_t vsoll = min(speed_to_v(akt_speed_soll), kmh2ms * min(adverse.max_speed, get_vehicle_summary().max_speed)); // vsoll in m/s, akt_speed_soll in simutrans vehicle speed. "Soll" translates to "Should", so this is the speed limit.
		const float32e8_t check_vsoll_1 = speed_to_v(akt_speed_soll);
		const float32e8_t check_vsoll_2 = kmh2ms * min(adverse.max_speed, get_vehicle_summary().max_speed);
		float32e8_t vsoll = check_vsoll_1 < check_vsoll_2 ? check_vsoll_1 : check_vsoll_2; 
		float32e8_t fvsoll = float32e8_t::zero; // force in N needed to hold vsoll. calculated when needed.
		float32e8_t speed_ratio = float32e8_t::zero; // requested speed / convoy's max speed. calculated when needed.
		float32e8_t dx = float32e8_t::zero; // covered distance in m
		float32e8_t v = akt_v; // v and akt_v in m/s
		float32e8_t bf = float32e8_t::zero; // braking force in N
		// iterate the passed time.
		while (delta_s > float32e8_t::zero)
		{
			// 1) The driver's part: select the force:
			bool is_braking = v >= _110_percent * vsoll;
			if (dx >= xbrk)
			{
				vsoll = vlim;
				is_braking = true;
			}

			float32e8_t f;
			if (is_braking)
			{
				// running too fast, slam on the brakes! 
				// hill-down Frs might become negative and works against the brake.
				// hill-up Frs helps braking, but don't brake too hard (with respect to health of passengers and freight)
				if (bf == float32e8_t::zero) // bf is a constant within this function. So calculate it once only.
				{
					bf = -get_braking_force(/*v*/) + max(float32e8_t::zero, g_accel * weight.weight_sin);
				}
				f = bf;
			}
			else if (v < vsoll)
			{
				// Below set speed: full acceleration
				// If set speed is far below the convoy max speed as e.g. aircrafts on ground reduce force.
				// If set speed is at most a 10th of convoy's maximum, we reduce force to its 10th.
				f = get_force(v);
				if (f > million) // reducing force does not apply to 'weak' convoy's, thus we can save a lot of time skipping this code.
				{
					if (speed_ratio == float32e8_t::zero) // speed_ratio is a constant within this function. So calculate it once only.
					{
						speed_ratio = ms2kmh * vsoll / vehicle_summary.max_speed;
					}
					if (speed_ratio < float32e8_t::tenth)
					{
						if (fvsoll == float32e8_t::zero) // fvsoll is a constant within this function. So calculate it once only.
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
			else 
			{
				if (fvsoll == float32e8_t::zero) // fvsoll is a constant within this function. So calculate it once only.
				{
					fvsoll = calc_speed_holding_force(vsoll, Frs);
				}
				f = fvsoll;
			}
			const float32e8_t Ff = sgn(v) * adverse.cf * v * v;
			f -= Ff + Frs;

			// 2) The "differential equation" part: calculate new speed:
			float32e8_t dt_s;
			if (delta_s >= fl_slice_seconds && abs(f.to_sint32()) > weight.weight / (10 * DT_SLICE_SECONDS))
			{
				// This part is important for acceleration/deceleration phases only. 
				// If the force to weight ratio exceeds a certain level, then we must calculate speed iterative,
				// as it depends on previous speed.
				dt_s = fl_slice_seconds;
			}
			else
			{
				// If a small force produces a small speed change, we can add the difference at once in the 'else' section
				// with a disregardable inaccuracy.
				dt_s = delta_s;
			}
			float32e8_t a = f / fweight;
			float32e8_t v0 = v;
			float32e8_t x;
			v += a * dt_s;
			if (is_braking)
			{
				if (v < vsoll)
				{
					// don't brake too much
					v = vsoll;
					a = v / dt_s;
				}
				x = dx + _calc_move(a, dt_s, v0);
				if (x > xlim && v < V_MIN)
				{
					// don't stop before arrival.
					v = V_MIN;
					a = v / dt_s;
					x = dx + _calc_move(a, dt_s, v0);
				}
			}
			else 
			{
				if (v > vsoll)	
				{
					// don't accelerate too much
					v = vsoll;
				}
				else if (v < V_MIN)
				{
					v = V_MIN;
				}
				x = dx + _calc_move(a, dt_s, v0);
				if (x > xbrk)
				{
					// don't run beyond xbrk, where we must start braking.
					x = xbrk;
					if (xbrk > dx && abs(a) > float32e8_t::milli)
					{
						// turn back time to when we reached xbrk:
						dt_s = (sqrt(v0 * v0 + float32e8_t::two * a * (xbrk - dx)) - v0) / a;
					}
				}
			}
			dx = x;
			delta_s -= dt_s; // another time slice passed
		}
		akt_v = v;
		akt_speed = v_to_speed(v); // akt_speed in simutrans vehicle speed, v in m/s
		sp_soll += (sint32)(settings.meters_to_steps(dx) * steps2yards); // sp_soll in simutrans yards, dx in m
	}
}

//float32e8_t convoy_t::power_index_to_power(const sint64 &power_index, sint32 power_factor)
//{
//	return float32e8_t(power_index * (power_factor * 10), (sint64) GEAR_FACTOR);
//}

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
	uint32 count = vehicles.get_count();
	for (uint32 i = count; i-- > 0; )
	{
		adverse.add_vehicle(*vehicles[i], i == 0);
	}
	if (count > 0)
	{
		adverse.fr /= count;
	}
}


void potential_convoy_t::update_freight_summary(freight_summary_t &freight)
{		
	freight.clear();
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		const vehicle_desc_t &b = *vehicles[i];
		freight.add_vehicle(b);
	}
}

float32e8_t potential_convoy_t::get_brake_summary(/*const float32e8_t &speed*/ /* in m/s */)
{
	float32e8_t force = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		const vehicle_desc_t &b = *vehicles[i];
		uint16 bf = b.get_brake_force();
		if (bf != BRAKE_FORCE_UNKNOWN)
		{
			force += bf * (uint32) 1000;
		}
		else
		{
			// Usual brake deceleration is about -0.5 .. -1.5 m/s² depending on vehicle and ground. 
			// With F=ma, a = F/m follows that brake force in N is ~= 1/2 weight in kg
			force += get_adverse_summary().br * b.get_weight();
		}
	}
	return force;
}


float32e8_t potential_convoy_t::get_resistance_summary()
{
	float32e8_t rolling_resistance = 0;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		rolling_resistance += vehicles[i]->get_rolling_resistance();
	}
	return rolling_resistance;
}

float32e8_t potential_convoy_t::get_force_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 force = 0;
	sint32 v = speed;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		force += vehicles[i]->get_effective_force_index(v);
	}
	return power_index_to_power(force, welt->get_settings().get_global_force_factor_percent());
}


float32e8_t potential_convoy_t::get_power_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 power = 0;
	sint32 v = speed;
	for (uint32 i = vehicles.get_count(); i-- > 0; )
	{
		power += vehicles[i]->get_effective_power_index(v);
	}
	return power_index_to_power(power, welt->get_settings().get_global_power_factor_percent());
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
	return convoy.get_vehicle_count() > 0 ? get_friction_of_waytype(convoy.get_vehicle(0)->get_waytype()) : 0;
}

void existing_convoy_t::update_vehicle_summary(vehicle_summary_t &vehicle)
{
	vehicle.clear();
	uint32 count = convoy.get_vehicle_count();
	for (uint32 i = count; i-- > 0; )
	{
		vehicle.add_vehicle(*convoy.get_vehicle(i)->get_desc());
	}
	if (count > 0)
	{
		vehicle.update_summary(convoy.get_vehicle(count-1)->get_desc()->get_length());
	}
}


void existing_convoy_t::update_adverse_summary(adverse_summary_t &adverse)
{
	adverse.clear();
	uint16 count = convoy.get_vehicle_count();
	for (uint16 i = count; i-- > 0; )
	{
		vehicle_t &v = *convoy.get_vehicle(i);
		adverse.add_vehicle(v);
	}
	if (count > 0)
	{
		adverse.fr /= count;
	}
}


void existing_convoy_t::update_freight_summary(freight_summary_t &freight)
{		
	freight.clear();
	for (uint16 i = convoy.get_vehicle_count(); i-- > 0; )
	{
		const vehicle_desc_t &b = *convoy.get_vehicle(i)->get_desc();
		freight.add_vehicle(b);
	}
}


void existing_convoy_t::update_weight_summary(weight_summary_t &weight)
{
	weight.clear();
	for (uint16 i = convoy.get_vehicle_count(); i-- > 0; )
	{
		vehicle_t &v = *convoy.get_vehicle(i);
		weight.add_vehicle(v);
	}
}


float32e8_t existing_convoy_t::get_brake_summary(/*const float32e8_t &speed*/ /* in m/s */)
{
	float32e8_t force = 0;
	for (uint16 i = convoy.get_vehicle_count(); i-- > 0; )
	{
		vehicle_t &v = *convoy.get_vehicle(i);
		const uint16 bf = v.get_desc()->get_brake_force();
		if (bf != BRAKE_FORCE_UNKNOWN)
		{
			force += bf * (uint32) 1000;
		}
		else
		{
			// Usual brake deceleration is about -0.5 .. -1.5 m/s² depending on vehicle and ground. 
			// With F=ma, a = F/m follows that brake force in N is ~= 1/2 weight in kg
			force += get_adverse_summary().br * v.get_total_weight();
		}
	}
	return force;
}
 

float32e8_t existing_convoy_t::get_force_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 force = 0;
	sint32 v = speed;
	for (uint16 i = convoy.get_vehicle_count(); i-- > 0; )
	{
		force += convoy.get_vehicle(i)->get_desc()->get_effective_force_index(v);
	}
	return power_index_to_power(force, welt->get_settings().get_global_force_factor_percent());
}


float32e8_t existing_convoy_t::get_power_summary(const float32e8_t &speed /* in m/s */)
{
	sint64 power = 0;
	sint32 v = speed;
	for (uint16 i = convoy.get_vehicle_count(); i-- > 0; )
	{
		power += convoy.get_vehicle(i)->get_desc()->get_effective_power_index(v);
	}
	return power_index_to_power(power, welt->get_settings().get_global_power_factor_percent());
}
