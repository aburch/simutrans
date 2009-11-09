/*
 * Copyright (c) 2009 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * convoy_metrics_t: Originally extracted from gui_convoy_assembler_t::zeichnen() and gui_convoy_label_t::zeichnen()
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

Let F = Fm - Fr - Fs.
Let cf = cw/2 * A * rho.
Let Frs = Fr + Fs = g * (fr * m * cos(alpha) + m * sin(alpha))

Then

cf * v^2 + m * a - F = 0

a = (F - cf * v^2 - Frs) / m

*******************************************************************************/

#include <math.h>
#include "convoy_metrics.h"
#include "vehicle/simvehikel.h"
#include "bauer/warenbauer.h"

#ifndef MAXUINT32
#define MAXUINT32 ((uint32)~((uint32)0))
#endif

#ifndef MAXUINT16
#define MAXUINT16 ((uint16)~((uint16)0))
#endif

// CF_*: constants related to air resistance

//#define CF_TRACK 0.7 / 2 * 10 * 1.2
//#define CF_TRACK 4.2
#define CF_TRACK 5

//#define CF_ROAD 0.7 / 2 * 6 * 1.2
#define CF_ROAD 2.52

// FR_*: constants related to roll resistance

#define FR_TRACK 0.0015
#define FR_ROAD  0.015
#define FR_WATER 0.015

void get_possible_freight_weight(uint8 catg_index, uint32 &min_weight, uint32 &max_weight)
{
	max_weight = 0;
	min_weight = MAXUINT32;
	for (uint16 j=0; j<warenbauer_t::get_waren_anzahl(); j++) {
		const ware_besch_t &ware = *warenbauer_t::get_info(j);
		if (ware.get_catg_index() == catg_index) {
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
	if (min_weight == MAXUINT32) 
	{
		min_weight = 0;
	}
}

void convoy_metrics_t::reset()
{
	power = 0;
	length = 0;
	vehicle_weight = 0;
	min_freight_weight = 0;
	max_freight_weight = 0;
	max_top_speed = MAXUINT32;
	cf = CF_ROAD;
	fr = FR_ROAD;
	mcos = 0; // m * cos(alpha)
	msin = 0; // m * sin(alpha)
}

void convoy_metrics_t::add_friction(int friction, int weight)
{
	// Friction is interpreted as 1000 * sin(alpha).
	// Including the 1000 for tons to kg conversion a friction of 50 corresponds to an inclination of about 28 per mille.
	if (friction)
	{
		msin += weight * friction;
		mcos += weight * sqrt(1000000.0 - friction * friction); // Remember: sin(alpha)^2 + cos(alpha)^2 = 1
	}
	else
	{			 
		mcos += weight * 1000;
	}
}

void convoy_metrics_t::add_vehicle_besch(const vehikel_besch_t &b)
{
	length += b.get_length();
	vehicle_weight += b.get_gewicht();
	uint32 payload = b.get_zuladung();
	if (payload > 0)
	{
		uint32 min_weight, max_weight;
		get_possible_freight_weight(b.get_ware()->get_catg_index(), min_weight, max_weight);
		min_freight_weight += (min_weight * payload + 499) / 1000;
		max_freight_weight += (max_weight * payload + 499) / 1000;
	}
	if (max_top_speed > b.get_geschw())
	{
		max_top_speed = b.get_geschw();
	}

	switch (b.get_waytype())
	{
		case water_wt:
			fr = FR_WATER;
			break;
	
		case track_wt:
		case overheadlines_wt: 
		case monorail_wt:      
		case maglev_wt:
		case tram_wt:
		case narrowgauge_wt:
			fr = FR_TRACK;
			cf = CF_TRACK;
			break;
	}
}

void convoy_metrics_t::add_vehicle(const vehikel_besch_t &b)
{
	add_vehicle_besch(b);
}

void convoy_metrics_t::add_vehicle(const vehikel_t &v)
{
	add_vehicle_besch(*v.get_besch());
	int weight = v.get_gesamtgewicht(); // vehicle weight in tons
	current_weight += weight; 
}

void convoy_metrics_t::calc()
{
	
}

void convoy_metrics_t::calc(convoi_t &cnv)
{
	reset();
	int count = cnv.get_vehikel_anzahl();
	if (count > 0)
	{
		for(unsigned i = count;  i-- > 0; ) {
			add_vehicle(*cnv.get_vehikel(i));
		}
		add_friction(get_friction_of_waytype(cnv.get_vehikel(0)->get_waytype()), 1);
		//BG, 30.08.2009: cannot use cnv.calc_adjusted_power() here.
		/* 
		 * Convoy_metrics are used to display them in the depot or convoy frame only.
		 * They show the top speeds for given power and full resp. empty convoy.
		 * Nominal power and top speed given in vehikel_besch_t both are max values and this power is valid for this top speed.
		 * Thus we need the max power here instead of the actual speed controlled power.
		 *
		 * It looked a bit strange while playing: 2 identical steam trains (8ft Stirling with 1 KBay-Mail and 6 KBay-Pax)
		 * The convoy frame of the standing train said, convoy was able to run 53..59 km/h, 
		 * while frame of the running train said, convoy could run 85 km/h.
		 * The user (at least me) expects always the same value: the top speed at top speed power, which is the 85.
		 *
		 * cnv.calc_adjusted_power() returns the power at current speed. 
		 * Thus do the same calculation with top speed (using get_effective_power_index(max_top_speed)).
		 *
		 * At first glance the effective power calculation might have been skipped at all, but the convoy top speed 
		 * can differ from the vehicle's top speed and thus steam engine might still be in the reduced power range.
		 */
		power = cnv.get_effective_power(max_top_speed); 
	}
	calc();
}

void convoy_metrics_t::calc(karte_t &world, vector_tpl<const vehikel_besch_t *> &vehicles)
{
	reset();
	int count = vehicles.get_count();
	if (count > 0)
	{
		for(unsigned i = count;  i-- > 0; ) {
			add_vehicle(*vehicles[i]);
		}
		// Evaluate msin and mcos for a single ton. 
		// On a hypothetical flat way we can multiply msin and mcos with the min resp. max weight
		// to calculate the max resp. min speed of the convoy in spe.
		add_friction(get_friction_of_waytype(vehicles[0]->get_waytype()), 1);
		//BG, 30.08.2009: power calculation was missing:
		uint32 power_index = 0;
		for(unsigned i = count;  i-- > 0; ) {
			power_index += vehicles[i]->get_effective_power_index(max_top_speed);
		}
		power = (power_index * world.get_einstellungen()->get_global_power_factor()) / 64;
	}
	calc();
}

// Calculate the maximum speed in kmh the given power in kWh can move the given weight in t.
#define max_kmh(power, weight) (sqrt(((power)/(weight))-1) * 50)

uint32 convoy_metrics_t::get_speed(uint32 weight) 
{ 
	// TODO: power depends on speed a simple max_kmh(power, weight) doesn't work!
	
	// assuming (max) power is available:
	double p3 = weight * (fr * mcos + msin) / ((3/9.81) * cf);
	double q2 = power / (0.002 * cf);
	double sd = sqrt(q2 * q2 + p3 * p3 * p3);
	double vold = max_kmh(power, weight);
	double vmax = (pow(q2 + sd, 1.0/3.0) - pow(sd - q2, 1.0/3.0)) * 3.6; // 3.6 converts to km/h
	return min(max_top_speed, (uint32) vmax); 

	// This was correct for the old physics formulae, but not now. Use maximum theoretical
	// speed, not maximum actual speed until a new formula can be found.

	//return min(max_top_speed, (uint32) max_kmh(power, weight)); 
	//return max_top_speed;
}

enum convoy_detail_e
{
	cd_cached_vehicle_data = 0x01,
	cd_cached_freight_data = 0x02
};

class convoy_t
{
private:
	int is_valid;
	/**
	* The vehicles of this convoi
	*
	* @author Hj. Malthaner
	*/
	array_tpl<vehikel_t*> fahr;

	/**
	* Number of vehicles in this convoi.
	* @author Hj. Malthaner
	*/
	uint8 anz_vehikel;

	// cached_vehicle_data becomes invalid, when the vehicle list or any vehicle's vehicle_besch_t changes.
	// it needs recaching only, if it is going to be used. 
	// it is valid if (is_valid & cd_cached_vehicle_data != 0)
	struct convoy_vehicle_data_t
	{
		/* sum_gewicht: leergewichte aller vehicles *
		* Werden nicht gespeichert, sondern aus den Einzelgewichten
		* errechnet beim beladen/fahren.
		* @author Hj. Malthaner, prissi
		*/
		sint32 weight; // empty weight in tons // sum_gewicht;

		/**
		* Gesamtleistung. Wird nicht gespeichert, sondern aus den Einzelleistungen
		* errechnet.
		* @author Hj. Malthaner
		*/
		sint32 power; // nominal/maximal power in kW // sum_leistung;

		/**
		* Gesamtleistung mit Gear. Wird nicht gespeichert, sondern aus den Einzelleistungen
		* errechnet.
		* @author prissi
		*/
		//sint32 sum_gear_und_leistung;
	}
	cached_vehicle_data;

	// cached_freight_data becomes invalid, when cached_vehicle_data becomes invalid or any vehicle's freight changes.
	// it needs recaching only, if it is going to be used. 
	// it is valid if (is_valid & cd_cached_freight_data != 0)
	struct convoy_freight_data_t
	{
		/* sum_gesamtgewicht: gesamtgewichte aller vehicles *
		* Werden nicht gespeichert, sondern aus den Einzelgewichten
		* errechnet beim beladen/fahren.
		* @author Hj. Malthaner, prissi
		*/
		sint32 weight; // sum_gesamtgewicht - sum_gewicht;
	}
	cached_freight_data;

	/**
	* Lowest top speed of all vehicles. Doesn't get saved, but calculated
	* from the vehicles data
	* @author Hj. Malthaner
	*/
	sint32 min_top_speed;
	
//public:
	
};
