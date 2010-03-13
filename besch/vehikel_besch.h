/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __VEHIKEL_BESCH_H
#define __VEHIKEL_BESCH_H

#include "obj_besch_std_name.h"
#include "ware_besch.h"
#include "bildliste_besch.h"
#include "bildliste2d_besch.h"
#include "skin_besch.h"
#include "sound_besch.h"
#include "../dataobj/ribi.h"
#include "../simworld.h"
#include "../simtypes.h"

// GEAR_FACTOR: a gear of 1.0 is stored as 64
#define GEAR_FACTOR 64

const uint32 DEFAULT_FIXED_VEHICLE_MAINTENANCE = 0;

/**
 * Vehicle type description - all attributes of a vehicle type
 *
 *  child nodes:
 *	0   Name
 *	1   Copyright
 *	2   freight
 *	3   smoke
 *	4   empty 1d image list
 *	5   either 1d (freight_image_type==0) or 2d image list
 *	6   required leading vehicle 1
 *	7   required leading vehicle 2
 *	... ...
 *	n+5 required leading vehicle n
 *	n+6 allowed trailing vehicle 1
 *	n+7 allowed trailing vehicle 2
 *	... ...
 *	n+m+5 allowed trailing vehicle m
 *  n+m+6 freight for which special images are defined
 *
 * @author Volker Meyer, Hj. Malthaner, kierongreen
 */
class vehikel_besch_t : public obj_besch_std_name_t {
    friend class vehicle_writer_t;
    friend class vehicle_reader_t;
    friend class vehikelbauer_t;

public:
	/**
	 * Engine type
	 * @author Hj. Malthaner
	 */
	enum engine_t {
		 unknown=-1,
		steam=0,
		diesel,
		electric,
		bio,
		sail,
		fuel_cell,
		hydrogene,
		battery
	};


private:
	uint32 preis;  //Price
	uint32 upgrade_price; //Price if this vehicle is bought as an upgrade, not a new vehicle.
	uint16 zuladung; //Payload
	uint16 overcrowded_capacity; // The capacity of a vehicle if overcrowded (usually expressed as the standing capacity).
	uint16 geschw; //Speed in km/h
	uint16 gewicht; //Weight in tons
	uint32 leistung; //Power in kW
	uint16 betriebskosten;  //Running costs
	uint16 scaled_running_costs; //@author: jamespetts
	uint32 fixed_maintenance; //@author: jamespetts, April 2009

	uint16 intro_date; // introduction date
	uint16 obsolete_date; //phase out at
	uint16 gear;       // engine gear (power multiplier), 64=100

	sint8 typ;         	// see weg_t for allowed types
	uint8 len;			// length (=8 is half a tile, the old default)
	sint8 sound;

	uint8 vorgaenger;	// all defined leading vehicles
	uint8 nachfolger;	// all defined trailer
	uint8 upgrades;		// The vehicles types to which this type may be upgraded.

	uint8 engine_type; // diesel, steam, electric (requires electrified ways), fuel_cell, etc.

	sint8 freight_image_type;	// number of freight images (displayed for different goods)

	bool is_tilting; //Whether it is a tilting train (can take corners at higher speeds). 0 for no, 1 for yes. Anything other than 1 is assumed to be no.
	
	uint8 way_constraints_permissive; //Way constraints. Actually, 8 boolean values. Bitwise operations necessary
	uint8 way_constraints_prohibitive; //to uncompress this (but if value is 0, are no constraints).

	uint8 catering_level; //The level of catering. 0 for no catering. Higher numbers for better catering.

	bool bidirectional; //Whether must always travel in one direction
	bool can_lead_from_rear; //Whether vehicle can lead a convoy when it is at the rear.

	uint8 comfort; // How comfortable that a vehicle is for passengers.

	uint16 loading_time; //Time in MS (at speed 1.0) to load/unload.

	bool available_only_as_upgrade; //If yes, can not be bought as new: only upgraded.
	
	uint16 tractive_effort; // tractive effort / force in kN

	// these values are not stored and therefore calculated in loaded():
	uint32 geared_power; // @author: Bernd Gabriel, Nov  4, 2009: == leistung * gear in kW
	uint32 geared_force; // @author: Bernd Gabriel, Dec 12, 2009: == tractive_effort * gear in kN
	/**
	 * force threshold speed in km/h.
	 * Below this threshold the engine works as constant force engine.
	 * Above this threshold the engine works as constant power engine.
	 * @author Bernd Gabriel, Nov 4, 2009
	 */
	uint16 force_threshold_speed; // @author: Bernd Gabriel, Nov 4, 2009: in m/s

	// @author: Bernd Gabriel, Dec 12, 2009: called as last action in read_node()
	void loaded();
public:
	// since we have a second constructor
	vehikel_besch_t() { }

	// default vehicle (used for way seach and similar tasks)
	// since it has no images and not even a name knot any calls to this will case a crash
	vehikel_besch_t(uint8 wtyp, uint16 speed, engine_t engine) {
		freight_image_type = preis = upgrade_price = zuladung = overcrowded_capacity = betriebskosten = intro_date = vorgaenger = nachfolger = catering_level = upgrades = 0;
		fixed_maintenance = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
		leistung = gewicht = comfort = 1;
		gear = GEAR_FACTOR;
		geared_power = GEAR_FACTOR;
		len = 8;
		sound = -1;
		typ = wtyp;
		engine_type = (uint8)engine;
		geschw = speed;
		is_tilting = bidirectional = can_lead_from_rear = available_only_as_upgrade = false;
		way_constraints_prohibitive = 255;
		way_constraints_permissive = 0;
		loading_time = 2000;
		tractive_effort = 0;
	}

	const ware_besch_t *get_ware() const { return static_cast<const ware_besch_t *>(get_child(2)); }

	const skin_besch_t *get_rauch() const { return static_cast<const skin_besch_t *>(get_child(3)); }

	image_id get_basis_bild() const { return get_bild_nr(ribi_t::dir_sued, get_ware() ); }

	// returns the number of different directions
	uint8 get_dirs() const { return (static_cast<const bildliste_besch_t *>(get_child(4)))->get_bild(4) ? 8 : 4; }

	// return a matching image
	// beware, there are three class of vehicles
	// vehicles with and without freight images, and vehicles with different freight images
	// they can have 4 or 8 directions ...
	image_id get_bild_nr(ribi_t::dir dir, const ware_besch_t *ware) const
	{
		const bild_besch_t *bild=0;
		const bildliste_besch_t *liste=0;

		if(freight_image_type>0  &&  ware!=NULL) {
			// more freight images and a freight: find the right one

			sint8 ware_index=0; // freight images: if not found use first freight

			for( sint8 i=0;  i<freight_image_type;  i++  ) {
				if(ware->get_index()==static_cast<const ware_besch_t *>(get_child(6 + nachfolger + vorgaenger + i))->get_index()) {
					ware_index = i;
					break;
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			const bildliste2d_besch_t *liste2d = static_cast<const bildliste2d_besch_t *>(get_child(5));
			bild=liste2d->get_bild(dir, ware_index);
			if(!bild) {
				if(dir>3) {
					bild = liste2d->get_bild(dir - 4, ware_index);
				}
			}
			if (bild != NULL) return bild->get_nummer();
		}

		// only try 1d freight image list for old style vehicles
		if(freight_image_type==0  &&  ware!=NULL) {
			liste = static_cast<const bildliste_besch_t *>(get_child(5));
		}
		else {
			liste = static_cast<const bildliste_besch_t *>(get_child(4));
		}

		if(!liste) {
			liste = static_cast<const bildliste_besch_t *>(get_child(4));
			if(!liste) {
				return IMG_LEER;
			}
		}

		bild = liste->get_bild(dir);
		if(!bild) {
			if(dir>3) {
				bild = liste->get_bild(dir - 4);
			}
			if(!bild) {
				return IMG_LEER;
			}
		}
		return bild->get_nummer();
	}

	// Liefert die erlaubten Vorgaenger.
	// liefert get_vorgaenger(0) == NULL, so bedeutet das entweder alle
	// Vorgänger sind erlaubt oder keine. Um das zu unterscheiden, sollte man
	// vorher hat_vorgaenger() befragen

	// Returns allowed predecessor.
	// provides get_vorgaenger (0) == NULL, it means that either all 
	// predecessors are allowed or not. To distinguish, one should 
	// predict hat_vorgaenger () question (Google)
	const vehikel_besch_t *get_vorgaenger(int i) const
	{
		if(i < 0 || i >= vorgaenger) {
			return NULL;
		}
		return static_cast<const vehikel_besch_t *>(get_child(6 + i));
	}

	/* returns true, if this veh can be before the next_veh */
	bool can_lead(const vehikel_besch_t *next_veh) const
	{
		if(  nachfolger==0  ) {
			return next_veh != 0;
		}
		for( int i=0;  i<nachfolger;  i++  ) {
			const vehikel_besch_t *veh = (vehikel_besch_t *)get_child(6 + vorgaenger + i);
			if(veh==next_veh) {
				return true;
			}
		}
		// only here if not allowed
		return false;
	}
	/* returns true, if this veh can be after the prev_veh */
	bool can_follow(const vehikel_besch_t *prev_veh) const
	{
		if(  vorgaenger==0  ) {
			return prev_veh != 0;
		}
		for( int i=0;  i<vorgaenger;  i++  ) {
			const vehikel_besch_t *veh = (vehikel_besch_t *)get_child(6 + i);
			if(veh==prev_veh) {
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	int get_vorgaenger_count() const { return vorgaenger; }

	// Liefert die erlaubten Nachfolger.
	// liefert get_nachfolger(0) == NULL, so bedeutet das entweder alle
	// Nachfolger sind erlaubt oder keine. Um das zu unterscheiden, sollte
	// man vorher hat_nachfolger() befragen

	// Returns the lawful successor.
	// provides get_nachfolger (0) == NULL, it means that either all
	// succeed or none are allowed. To distinguish, one should 
	// predict hat_nachfolger () question (Google)
	const vehikel_besch_t *get_nachfolger(int i) const
	{
		if(i < 0 || i >= nachfolger) {
			return NULL;
		}
		return static_cast<const vehikel_besch_t *>(get_child(6 + vorgaenger + i));
	}

	int get_nachfolger_count() const { return nachfolger; }

	// Returns the vehicle types to which this vehicle type may be upgraded.

	const vehikel_besch_t * get_upgrades(int i) const
	{
		if(i < 0 || i >= upgrades)
		{
			return NULL;
		}
		return static_cast<const vehikel_besch_t *>(get_child(6 + nachfolger + i));
	}

	int get_upgrades_count() const { return upgrades; }

	waytype_t get_waytype() const { return static_cast<waytype_t>(typ); }
	uint16 get_zuladung() const { return zuladung; }
	uint32 get_preis() const { return preis; }
	uint16 get_geschw() const { return geschw; }
	uint16 get_gewicht() const { return gewicht; }
	uint16 get_betriebskosten() const { return scaled_running_costs; }
	uint16 get_base_running_costs() const { return betriebskosten; }
	uint16 get_base_running_costs(karte_t *welt) const; //Overloaded method - includes increase for obsolescence.
	
	uint16 get_betriebskosten(karte_t *welt) const; //Overloaded method - includes increase for obsolescence.
	uint32 get_fixed_maintenance() const { return fixed_maintenance; }
	uint32 get_fixed_maintenance(karte_t *welt) const;  //Overloaded method - includes increase for obsolescence.
	uint32 get_adjusted_monthly_fixed_maintenance(karte_t *welt) const; // includes increase for obsolescence and adjustment for monthly figures
	sint8 get_sound() const { return sound; }
	bool is_bidirectional() const { return bidirectional; }
	bool get_can_lead_from_rear() const { return can_lead_from_rear; }
	uint8 get_comfort() const { return comfort; }
	uint16 get_overcrowded_capacity() const { return overcrowded_capacity; }
	uint16 get_loading_time() const { return zuladung > 0 ? loading_time : 0; }
	uint32 get_upgrade_price() const { return upgrade_price; }
	bool is_available_only_as_upgrade() const { return available_only_as_upgrade; }

	// BG, 15.06.2009: the formula for obsolescence formerly implemented twice in get_betriebskosten() and get_fixed_maintenance()
	uint32 calc_running_cost(const karte_t *welt, uint32 base_cost) const;	
	float get_power_force_ratio() const;
	uint32 calc_max_force(const uint32 power) const { 
		return power ? (uint32)(power / get_power_force_ratio() + 0.5f) : 0; 
	}
	uint32 calc_max_power(const uint32 force) const { 
		return force ? (uint32)(force * get_power_force_ratio() + 0.5f) : 0; 
	}
	uint32 get_leistung() const { 
		return leistung ? leistung : calc_max_power(tractive_effort); 
	}
	uint32 get_tractive_effort() const { 
		return tractive_effort ? tractive_effort : calc_max_force(leistung);
	}

	/**
	* @return introduction year
	* @author Hj. Malthaner
	*/
	uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return time when obsolete
	* @author prissi
	*/
	uint16 get_retire_year_month() const { return obsolete_date; }

	// true if future
	bool is_future (const uint16 month_now) const
	{
		return month_now  &&  (intro_date > month_now);
	}

	// true if obsolete
	bool is_retired (const uint16 month_now) const
	{
		return month_now  &&  (obsolete_date <= month_now);
	}

	/**
	* 64 = 1.00
	* @return gear value
	* @author Hj. Malthaner
	*/
	uint16 get_gear() const { return gear; }

	/**
	* @return engine type
	* eletric engines require an electrified way to run
	* @author Hj. Malthaner
	*/
	uint8 get_engine_type() const { return engine_type; }

	/* @return the vehicles length in 1/8 of the normal len
	* @author prissi
	*/
	uint8 get_length() const { return len; }
	
	/*Whether this is a tilting train (and can take coerners faster
	*@author: jamespetts*/
	bool get_tilting() const { return (is_tilting);	}
	
	/*Bitwise encoded way constraints (permissive)
	*@author: jamespetts*/
	uint8 get_permissive_constraints() const { return way_constraints_permissive; }
	
	/*Bitwise encoded way constraints (prohibitive)
	*@author: jamespetts*/
	uint8 get_prohibitive_constraints() const { return way_constraints_prohibitive; }

	bool permissive_way_constraint_set(uint8 i) const
	{
		return (((way_constraints_permissive >> i) & 1) != 0);
	}

	bool prohibitive_way_constraint_set(uint8 i) const
	{
		return (((way_constraints_prohibitive >> i) & 1) != 0);
	}
	
	/*The level of catering provided by this vehicle (0 if none)
	*@author: jamespetts*/
	uint8 get_catering_level() const { return catering_level; }


	/* test, if a certain vehicle can lead a convoi *
	 * used by vehikel_search
	 * @author prissi
	 */
	bool can_lead() const {
		if(vorgaenger==0) {
			return true;
		}
		for( int i=0;  i<vorgaenger;  i++  ) {
			if(get_child(6 + i)==NULL) {
				return true;
			}
		}
		// cannot lead
		return false;
	}

	bool can_follow_any() const { return nachfolger==0; }

	void set_scale(float scale_factor) 
	{ 
		// BG: 29.08.2009: explicit typecasts avoid warnings
		scaled_running_costs = (uint16)(betriebskosten == 0 ? 0 : betriebskosten * scale_factor > 0 ? betriebskosten * scale_factor : 1); 
		preis = (uint32)(preis == 0 ? 0 : preis * scale_factor > 0 ? preis * scale_factor : 1);
		fixed_maintenance = (uint32)(fixed_maintenance == 0 ? 0 : fixed_maintenance * scale_factor > 0 ? fixed_maintenance * scale_factor : 1);
	}

	/**
	 * Get effective force index. 
	 * Steam engine's force depend on its speed.
	 * Effective force in kN: force_index * welt->get_einstellungen()->get_global_power_factor() / GEAR_FACTOR
	 * @author Bernd Gabriel
	 */
	uint32 get_effective_force_index(uint16 speed /* in m/s */ ) const;

	/**
	 * Get effective power index. 
	 * Steam engine's power depend on its speed.
	 * Effective power in kW: power_index * welt->get_einstellungen()->get_global_power_factor() / GEAR_FACTOR
	 * @author Bernd Gabriel
	 */
	uint32 get_effective_power_index(uint16 speed /* in m/s */ ) const;
};

#endif
