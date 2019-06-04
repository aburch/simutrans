/*
 * Copyright (c) 2009..2011 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * convoy_t: common collection of properties of a convoi_t and a couple of vehicles which are going to become a convoy_t.
 * While convoi_t is involved in game play, convoy_t is the entity that physically models the convoy.
 */
/*******************************************************************************

Vehicle/Convoy Physics.

We know: delta_v = a * delta_t, where acceleration "a" is nearly constant for very small delta_t only.

At http://de.wikipedia.org/wiki/Fahrwiderstand we find a 
complete explanation of the force equation of a land vehicle.
(Sorry, there seems to be no english pendant at wikipedia).

Force balance: Fm = Ff + Fr + Fs + Fa; 

Fm: machine force in Newton [N] = [kg*m/s^2]

Ff: air resistance, always > 0
    Ff = cw/2 * A * rho * v^2, 
		cw: friction factor: average passenger cars and high speed trains: 0.25 - 0.5, average trucks and trains: 0.7
		A: largest profile: average passenger cars: 3, average trucks: 6, average train: 10 [m^2]
		rho = density of medium (air): 1.2 [kg/m^3]
		v: speed [m/s]

Fr: roll resistance, always > 0 
    Fr = fr * g * m * cos(alpha)
		fr: roll resistance factor: steel wheel on track: 0.0015, car wheel on road: 0.015
		g: gravitation constant: 9,81 [m/s^2]
		m: mass [kg]
		alpha: inclination: 0=flat

Fs: slope force/resistance, downhill: Fs < 0 (force), uphill: Fs > 0 (resistance)
	Fs = g * m * sin(alpha)
		g: gravitation constant: 9.81 [m/s^2]
		m: mass [kg]
		alpha: inclination: 0=flat

Fa: accelerating force
	Fa = m * a
		m: mass [kg]
		a: acceleration

Let Frs = Fr + Fs = g * m * (fr * cos(alpha) + sin(alpha)).
Let cf = cw/2 * A * rho.

Then

Fm = cf * v^2 + Frs + m * a

a = (Fm - Frs - cf * v^2) / m

*******************************************************************************/
#pragma once

#ifndef convoy_h
#define convoy_h

#include <limits>
#include <math.h>

#include "utils/float32e8_t.h"
#include "simunits.h"
#include "tpl/vector_tpl.h"
#include "descriptor/vehicle_desc.h"
#include "simtypes.h"

class vehicle_t;

//// CF_*: constants related to air resistance
//
////#define CF_TRACK 0.7 / 2 * 10 * 1.2
////#define CF_TRACK 4.2
//static const float32e8_t CF_TRACK = float32e8_t((uint32) 13);
//static const float32e8_t CF_MAGLEV = float32e8_t((uint32) 10);
////#define CF_ROAD 0.7 / 2 * 6 * 1.2
//static const float32e8_t CF_ROAD = float32e8_t((uint32) 252, (uint32) 100);
//static const float32e8_t CF_WATER = float32e8_t((uint32) 25);
//static const float32e8_t CF_AIR = float32e8_t((uint32)1);
//
//// FR_*: constants related to rolling resistance
//
////should be 0.0015, but for game balance it is higher 
////#define FR_TRACK 0.0015
//static const float32e8_t FR_MAGLEV = float32e8_t((uint32) 15, (uint32) 10000);
//static const float32e8_t FR_TRACK = float32e8_t((uint32) 51, (uint32) 10000);
//static const float32e8_t FR_ROAD = float32e8_t((uint32) 15, (uint32) 1000);
//static const float32e8_t FR_WATER = float32e8_t((uint32) 1, (uint32) 1000);
//static const float32e8_t FR_AIR = float32e8_t((uint32) 1, (uint32) 1000);

extern const float32e8_t BR_AIR;
extern const float32e8_t BR_WATER;
extern const float32e8_t BR_TRACK;
extern const float32e8_t BR_TRAM;
extern const float32e8_t BR_MAGLEV;
extern const float32e8_t BR_ROAD;
extern const float32e8_t BR_DEFAULT;

/******************************************************************************/

struct vehicle_summary_t
{
	uint32 length;			// sum of vehicles' length in 1/OBJECT_OFFSET_STEPSth of a tile
	uint32 tiles;           // length of convoy in tiles.
	sint32 weight;			// sum of vehicles' own weight without load in kg
	sint32 max_speed;		// minimum of all vehicles' maximum speed in km/h
	sint32 max_sim_speed;	// minimum of all vehicles' maximum speed in simutrans speed

	inline void clear()
	{
		length = 0;
		tiles = 0;
		weight = 0;
		max_speed = KMH_SPEED_UNLIMITED; // if there is no vehicle, there is no speed limit!
	}

	inline void add_vehicle(const vehicle_desc_t &b)
	{
		length += b.get_length();
		weight += b.get_weight();
		max_speed = min(max_speed, b.get_topspeed());
	}

	// call update_summary() after all vehicles have been added.
	inline void update_summary(uint8 length_of_last_vehicle)
	{
		// this correction corresponds to the correction in convoi_t::get_tile_length()
		tiles = (length + (max(CARUNITS_PER_TILE/2, length_of_last_vehicle) - length_of_last_vehicle) + CARUNITS_PER_TILE - 1) / CARUNITS_PER_TILE;
		max_sim_speed = kmh_to_speed(max_speed);
	}
};

/******************************************************************************/

// should have been named: struct environ_summary_t
// but "environ" is the name of a defined macro. 
struct adverse_summary_t
{
	float32e8_t cf;	// air resistance constant: cf = cw/2 * A * rho. Depends on rho, which depends on altitude.
	float32e8_t fr;	// rolling resistance
	float32e8_t br; // brake force factor
	sint32 max_speed;

	inline void clear()
	{
		cf = fr = br = 0;
		max_speed = KMH_SPEED_UNLIMITED; 
	}

	void add_vehicle(const vehicle_t &v);
	void add_vehicle(const vehicle_desc_t &b, bool is_leading);
};

/******************************************************************************/

struct freight_summary_t
{
	// several freight of the same category may weigh different: 
	sint32 min_freight_weight; // in kg
	sint32 max_freight_weight; // in kg

	inline void clear()
	{
		min_freight_weight = max_freight_weight = 0;
	}

	void add_vehicle(const vehicle_desc_t &b);
};

/******************************************************************************/

struct weight_summary_t
{
	sint32 weight;				// vehicle and freight weight in kg. depends on vehicle (weight) and freight (weight)
	float32e8_t weight_cos;		// vehicle and freight weight in kg multiplied by cos(alpha). depends on adverse (way/inclination), vehicle and freight
	float32e8_t weight_sin;		// vehicle and freight weight in kg multiplied by sin(alpha). depends on adverse (way/inclination), vehicle and freight

	weight_summary_t()
	{
	}

	weight_summary_t(sint32 kgs, sint32 sin_alpha)
	{
		clear();
		add_weight(kgs, sin_alpha);
	}

	inline void clear()
	{
		weight_cos = weight_sin = (uint32) 0;
		weight = 0;
	}

	/**
	 * kgs: weight in kilograms
	 * sin_alpha: inclination and friction factor == 1000 * sin(alpha), 
	 *		      e.g.: 50 corresponds to an inclination of 28 per mille.
	 */
	void add_weight(sint32 kgs, sint32 sin_alpha);

	void add_vehicle(const vehicle_t &v);
};

/******************************************************************************/

class convoy_t /*abstract */
{
private:
	/**
	 * Get force in N according to current speed in m/s
	 */
	inline float32e8_t get_force(const float32e8_t &speed) 
	{
		return get_force_summary(abs(speed));
	}

	/**
	 * Get force in N that holds the given speed v or maximum available force, what ever is lesser.
	 * Frs = Fr + Fs
	 * Fr: roll resistance, always > 0 
	 * Fs: slope force/resistance, downhill: Fs < 0 (force), uphill: Fs > 0 (resistance)
	 */
	inline float32e8_t calc_speed_holding_force(const float32e8_t &v /* in m/s */, const float32e8_t &Frs /* in N */)
	{
		return min(get_force(v), adverse.cf * v * v + Frs); /* in N */
	}
protected:
	vehicle_summary_t vehicle_summary;
	adverse_summary_t adverse;

	/**
	 * get brake force in kN according to current speed in m/s
	 */
	virtual float32e8_t get_brake_summary(/*const float32e8_t &speed*/) = 0;

	/**
	 * get engine force in kN according to current speed in m/s
	 */
	virtual float32e8_t get_force_summary(const float32e8_t &speed) = 0;

	/**
	 * get engine power in kW according to current speed in m/s
	 */
	virtual float32e8_t get_power_summary(const float32e8_t &speed) = 0;

	/**
	 * get vehicle summary
	 */
	virtual const vehicle_summary_t &get_vehicle_summary() {
		return vehicle_summary;
	}

	/**
	 * get adverse summary
	 */
	virtual const adverse_summary_t &get_adverse_summary() {
		return adverse;
	}

	/**
	 * convert power index into power in W or convert force index into force in N.
	 * power_index: a value gotten from vehicles (e.g. from get_effective_force_index()/get_effective_power_index()).
	 * power_factor: the global power factor percentage. Must not be 0!.
	 */
	inline float32e8_t power_index_to_power(const sint64 &power_index, sint32 power_factor)
	{
		static const float32e8_t factor = float32e8_t(10) / float32e8_t(GEAR_FACTOR);
		return float32e8_t(power_index * power_factor) * factor;
	}
public:
	/**
	 * get braking force in N at given speed in m/s
	 */
	virtual float32e8_t get_braking_force(/*const float32e8_t &speed*/) 
	{
		return get_brake_summary(/*speed*/);
	}

	/*
	 * get starting force in N
	 */
	virtual float32e8_t get_starting_force() { 
		return get_force_summary(0); 
	}

	/*
	 * get continuous power in W
	 */
	virtual float32e8_t get_continuous_power() 
	{ 
#ifndef NETTOOL
		return get_power_summary(get_vehicle_summary().max_speed * kmh2ms); 

#else
		return get_power_summary(get_vehicle_summary().max_speed);
#endif
	}

	/**
	 * Update adverse.max_speed. If given speed is less than current adverse.max_speed, then speed becomes the new adverse.max_speed.
	 */
	void update_max_speed(const int speed)
	{ 
		if (adverse.max_speed > speed || adverse.max_speed == 0) adverse.max_speed = speed; 
	}

	/**
	 * For calculating max speed at an arbitrary weight apply this result to your weight_summary_t() constructor as param sin_alpha.
	 */
	virtual sint16 get_current_friction() = 0;

	/** 
	 * Get maximum possible speed of convoy in km/h according to weight, power/force, inclination, etc.
	 * Depends on vehicle, adverse and given weight.
	 */
	sint32 calc_max_speed(const weight_summary_t &weight); 

	/** 
	 * Get maximum pullable weight at given inclination of convoy in kg according to maximum force, allowed maximum speed and continuous power.
	 * @param sin_alpha is 1000 times sin(inclination_angle). e.g. 50 == inclination of 2.8 per mille.
	 * Depends on vehicle and adverse.
	 */
	sint32 calc_max_weight(sint32 sin_alpha); 

	sint32 calc_max_starting_weight(sint32 sin_alpha);

	/**
	 * Get the minimum braking distance in m for the convoy with given weight summary at given speed v in m/s.
	 */
	float32e8_t calc_min_braking_distance(const weight_summary_t &weight, const float32e8_t &v);

	/**
	 * Get the minimum braking distance in steps for the convoy with given weight summary at given simutrans speed.
	 */
	sint32 calc_min_braking_distance(const class settings_t &settings, const weight_summary_t &weight, sint32 speed);

	/** 
	 * Calculate the movement within delta_t
	 *
	 * @param simtime_factor the factor for translating simutrans time. Currently (Oct, 23th 2011) this is the length of a tile in meters divided by the standard tile length (1000 meters).
	 * @param weight the current weight summary of the convoy.
	 * @param akt_speed_soll the desired end speed in simutrans speed.
	 * @param next_speed_limit the next speed limit in simutrans speed.
	 * @param steps_til_limit the distance in simutrans steps to the next speed limit.
	 * @param steps_til_brake the distance in simutrans steps to the point where we must start braking to obey the speed limit at steps_til_limit.
	 * @param akt_speed the current speed and returns the new speed after delta_t has gone in simutrans speed.
	 * @param sp_soll the number of simutrans yards still to go and returns the new number of simutrans yards to go.
	 */
	void calc_move(const class settings_t &settings, long delta_t, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 next_speed_limit, sint32 steps_til_limit, sint32 steps_til_brake, sint32 &akt_speed, sint32 &sp_soll, float32e8_t &akt_v);
	virtual ~convoy_t(){}
};

/******************************************************************************/

enum convoy_detail_e
{
	cd_vehicle_summary  = 0x01,
	cd_adverse_summary  = 0x02,
	cd_freight_summary  = 0x04,
	cd_weight_summary   = 0x08,
	cd_starting_force   = 0x10,
	cd_continuous_power = 0x20,
	cd_braking_force    = 0x40,
};

class lazy_convoy_t /*abstract*/ : public convoy_t
{
private:
	freight_summary_t freight;
	float32e8_t starting_force;   // in N, calculated in convoy_t::get_starting_force()
	float32e8_t braking_force;      // in N, calculated in convoy_t::get_brake_force()
	float32e8_t continuous_power; // in W, calculated in convoy_t::get_continuous_power()
protected:
	int is_valid; // OR combined enum convoy_detail_e values.
	// decendents implement the update methods. 
	virtual void update_vehicle_summary(vehicle_summary_t &vehicle) { (void)vehicle; } // = 0;
	virtual void update_adverse_summary(adverse_summary_t &adverse) { (void)adverse; } // = 0;
	virtual void update_freight_summary(freight_summary_t &freight) { (void)freight; } // = 0;
public:

	//-----------------------------------------------------------------------------
	
	// vehicle_summary becomes invalid, when the vehicle list or any vehicle's vehicle_desc_t changes.
	inline void invalidate_vehicle_summary()
	{
		is_valid &= ~(cd_vehicle_summary|cd_adverse_summary|cd_weight_summary|cd_starting_force|cd_continuous_power|cd_braking_force);
	}

	// vehicle_summary is valid if (is_valid & cd_vehicle_summary != 0)
	inline void validate_vehicle_summary() {
		if (!(is_valid & cd_vehicle_summary)) 
		{
			is_valid |= cd_vehicle_summary;
			update_vehicle_summary(vehicle_summary);
		}
	}

	// vehicle_summary needs recaching only, if it is going to be used. 
	virtual const vehicle_summary_t &get_vehicle_summary() {
		validate_vehicle_summary();
		return convoy_t::get_vehicle_summary();
	}

	//-----------------------------------------------------------------------------
	
	// adverse_summary becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_desc_t or any vehicle's location/way changes.
	inline void invalidate_adverse_summary()
	{
		is_valid &= ~(cd_adverse_summary|cd_weight_summary|cd_braking_force);
	}

	// adverse_summary is valid if (is_valid & cd_adverse_summary != 0)
	inline void validate_adverse_summary() {
		if (!(is_valid & cd_adverse_summary)) 
		{
			is_valid |= cd_adverse_summary;
			update_adverse_summary(adverse);
		}
	}

	// adverse_summary needs recaching only, if it is going to be used. 
	virtual const adverse_summary_t &get_adverse_summary() {
		validate_adverse_summary();
		return convoy_t::get_adverse_summary();
	}

	//-----------------------------------------------------------------------------
	
	// freight_summary becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_desc_t.
	inline void invalidate_freight_summary()
	{
		is_valid &= ~(cd_freight_summary);
	}

	// freight_summary is valid if (is_valid & cd_freight_summary != 0)
	inline void validate_freight_summary() {
		if (!(is_valid & cd_freight_summary)) 
		{
			is_valid |= cd_freight_summary;
			update_freight_summary(freight);
		}
	}

	// freight_summary needs recaching only, if it is going to be used. 
	virtual const freight_summary_t &get_freight_summary() {
		validate_freight_summary();
		return freight;
	}

	//-----------------------------------------------------------------------------
	
	// starting_force becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_desc_t.
	inline void invalidate_starting_force()
	{
		is_valid &= ~(cd_starting_force);
	}

	virtual float32e8_t get_starting_force() 
	{
		if (!(is_valid & cd_starting_force)) 
		{
			is_valid |= cd_starting_force;
			starting_force = convoy_t::get_starting_force();
		}
		return starting_force;
	}

	//-----------------------------------------------------------------------------
	
	// brake_force becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_desc_t.
	inline void invalidate_brake_force()
	{
		is_valid &= ~(cd_braking_force);
	}

	virtual float32e8_t get_braking_force() 
	{
		if (!(is_valid & cd_braking_force)) 
		{
			is_valid |= cd_braking_force;
			braking_force = convoy_t::get_braking_force();
		}
		return braking_force;
	}

	//-----------------------------------------------------------------------------
	
	// continuous_power becomes invalid, when vehicle_summary becomes invalid 
	// or any vehicle's vehicle_desc_t.
	inline void invalidate_continuous_power()
	{
		is_valid &= ~(cd_continuous_power);
	}

	virtual float32e8_t get_continuous_power()
	{
		if (!(is_valid & cd_continuous_power)) 
		{
			is_valid |= cd_continuous_power;
			continuous_power = convoy_t::get_continuous_power();
		}
		return continuous_power;
	}

	//-----------------------------------------------------------------------------

	lazy_convoy_t() : convoy_t()
	{
		is_valid = 0;
	}

	sint32 calc_max_speed(const weight_summary_t &weight)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		return convoy_t::calc_max_speed(weight);
	}

	sint32 calc_max_weight(sint32 sin_alpha)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		return convoy_t::calc_max_weight(sin_alpha);
	}

	sint32 calc_max_starting_weight(sint32 sin_alpha)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		return convoy_t::calc_max_starting_weight(sin_alpha);
	}

	void calc_move(const class settings_t &settings, long delta_t, const weight_summary_t &weight, sint32 akt_speed_soll, sint32 next_speed_limit, sint32 steps_til_limit, sint32 steps_til_break, sint32 &akt_speed, sint32 &sp_soll, float32e8_t &akt_v)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
		convoy_t::calc_move(settings, delta_t, weight, akt_speed_soll, next_speed_limit, steps_til_limit, steps_til_break, akt_speed, sp_soll, akt_v);
	}

	//virtual ~lazy_convoy_t(){}
};

/******************************************************************************/

class potential_convoy_t : public lazy_convoy_t
{
private:
	vector_tpl<const vehicle_desc_t *> &vehicles;
protected:
	virtual void update_vehicle_summary(vehicle_summary_t &vehicle);
	virtual void update_adverse_summary(adverse_summary_t &adverse);
	virtual void update_freight_summary(freight_summary_t &freight);
	virtual float32e8_t get_brake_summary(/*const float32e8_t &speed */ /* in m/s */);
	virtual float32e8_t get_force_summary(const float32e8_t &speed /* in m/s */);
	virtual float32e8_t get_power_summary(const float32e8_t &speed /* in m/s */);
public:
	potential_convoy_t(vector_tpl<const vehicle_desc_t *> &desc) : lazy_convoy_t(), vehicles(desc)
	{
	}
	virtual sint16 get_current_friction();
	virtual ~potential_convoy_t(){}
	virtual float32e8_t get_resistance_summary();
};

/******************************************************************************/

class vehicle_as_potential_convoy_t : public potential_convoy_t
{
private:
	vector_tpl<const vehicle_desc_t *> vehicles;
public:
	vehicle_as_potential_convoy_t(const vehicle_desc_t &desc) : potential_convoy_t(vehicles)
	{
		vehicles.append(&desc);
	}
};

/******************************************************************************/

class existing_convoy_t : public lazy_convoy_t
{
private:
	class convoi_t &convoy;
	weight_summary_t weight;
protected:
	virtual void update_vehicle_summary(vehicle_summary_t &vehicle);
	virtual void update_adverse_summary(adverse_summary_t &adverse);
	virtual void update_freight_summary(freight_summary_t &freight);
	virtual void update_weight_summary(weight_summary_t &weight);
	virtual float32e8_t get_brake_summary(/*const float32e8_t &speed*/ /* in m/s */);
	virtual float32e8_t get_force_summary(const float32e8_t &speed /* in m/s */);
	virtual float32e8_t get_power_summary(const float32e8_t &speed /* in m/s */);
public:
	existing_convoy_t(class convoi_t &vehicles) : lazy_convoy_t(), convoy(vehicles)
	{
		validate_vehicle_summary();
		validate_adverse_summary();
	}
	virtual ~existing_convoy_t(){}

	//-----------------------------------------------------------------------------

	virtual sint16 get_current_friction();
	
	// weight_summary becomes invalid, when vehicle_summary or envirion_summary 
	// becomes invalid.
	inline void invalidate_weight_summary()
	{
		is_valid &= ~cd_weight_summary;
	}

	// weight_summary is valid if (is_valid & cd_weight_summary != 0)
	inline void validate_weight_summary() {
		if (!(is_valid & cd_weight_summary)) 
		{
			is_valid |= cd_weight_summary;
			update_weight_summary(weight);
		}
	}

	// weight_summary needs recaching only, if it is going to be used. 
	inline const weight_summary_t &get_weight_summary() {
		validate_weight_summary();
		return weight;
	}

	inline void calc_move(const settings_t &settings, long delta_t, sint32 akt_speed_soll, sint32 next_speed_limit, sint32 steps_til_limit, sint32 steps_til_brake, sint32 &akt_speed, sint32 &sp_soll, float32e8_t &akt_v)
	{
		validate_weight_summary();
		convoy_t::calc_move(settings, delta_t, weight, akt_speed_soll, next_speed_limit, steps_til_limit, steps_til_brake, akt_speed, sp_soll, akt_v);
	}

};

#endif
