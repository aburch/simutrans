/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __VEHIKEL_BESCH_H
#define __VEHIKEL_BESCH_H
#include <cstring>

#include "obj_besch_std_name.h"
#include "ware_besch.h"
#include "bildliste_besch.h"
#include "bildliste2d_besch.h"
#include "bildliste3d_besch.h"
#include "skin_besch.h"
#include "sound_besch.h"
#include "../dataobj/ribi.h"
#include "../dataobj/way_constraints.h"
#include "../simworld.h"
#include "../simtypes.h"
#include "../utils/float32e8_t.h"
#include "../simunits.h"

// GEAR_FACTOR: a gear of 1.0 is stored as 64
#define GEAR_FACTOR 64

// BRAKE_FORCE_UNDEFINED
#define BRAKE_FORCE_UNKNOWN 65535

const uint32 DEFAULT_FIXED_VEHICLE_MAINTENANCE = 0;
class checksum_t;

/**
 * Vehicle type description - all attributes of a vehicle type
 *
 *  child nodes:
 *	0   Name
 *	1   Copyright
 *	2   freight
 *	3   smoke
 *	4   empty 1d image list (or 2d list if there are multiple liveries)
 *	5   either 1d (freight_image_type==0), 2d image list or 3d image list (if multiple liveries)
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

	static const char* get_engine_type(engine_t e) 
	{
		switch(e)
		{
		case unknown:
		default:
			return "unknown";
		case steam:
			return "steam";
		case diesel:
			return "diesel";
		case electric:
			return "electric";
		case bio:
			return "bio";
		case sail:
			return "sail";
		case fuel_cell:
			return "fuel_cell";
		case hydrogene:
			return "hydrogene";
		case battery:
			return "battery";
		}
	}


private:
	uint32 preis;				// Price
	uint32 base_price;			// Price (without scale factor)
	uint32 upgrade_price;		// Price if this vehicle is bought as an upgrade, not a new vehicle.
	uint32 base_upgrade_price;  // Upgrade price (without scale factor)
	uint16 zuladung;			// Payload
	uint16 overcrowded_capacity; // The capacity of a vehicle if overcrowded (usually expressed as the standing capacity).
	uint16 geschw;				// Speed in km/h
	uint32 gewicht;				// Weight in tonnes
	uint16 axle_load;			// New for Standard, not used yet.
	uint32 leistung;			// Power in kW
	uint16 running_cost;		// Per kilometre cost
	uint32 fixed_cost;			// Monthly cost @author: jamespetts, April 2009
	uint32 base_fixed_cost;		// Monthly cost (without scale factor)

	uint16 intro_date;			// introduction date
	uint16 obsolete_date;		// phase out at
	uint16 gear;				// engine gear (power multiplier), 64=100

	sint8 typ;         			// see weg_t for allowed types
	uint8 len;					// length (=8 is half a tile, the old default)
	sint8 sound;

	uint8 vorgaenger;			// all defined leading vehicles
	uint8 nachfolger;			// all defined trailer
	uint8 upgrades;				// The vehicles types to which this type may be upgraded.

	uint8 engine_type;			// diesel, steam, electric (requires electrified ways), fuel_cell, etc.

	uint8 freight_image_type;	// number of freight images (displayed for different goods)
	uint8 livery_image_type;	// Number of different liveries (@author: jamespetts, April 2011)
	
	bool is_tilting;			 //Whether it is a tilting train (can take corners at higher speeds). 0 for no, 1 for yes. Anything other than 1 is assumed to be no.
	
	way_constraints_of_vehicle_t way_constraints;

	uint8 catering_level;		 //The level of catering. 0 for no catering. Higher numbers for better catering.

	bool bidirectional;			//Whether must always travel in one direction
	bool can_lead_from_rear;	//Whether vehicle can lead a convoy when it is at the rear.
	bool can_be_at_rear;		//Whether the vehicle may be at the rear of a convoy (default = true).

	uint8 comfort;				// How comfortable that a vehicle is for passengers.

	/** The time that the vehicle takes to load
	  * in ticks. Min: if no passengers/goods
	  * board/alight; Max: if all passengers/goods
	   * board/alight at once. Scaled linear
	  * beween the two. Was just "loading_time"
	  * before 10.0. 
	  * @author: jamespetts
	  */
	uint32 max_loading_time;
	uint32 min_loading_time; 

	/**
	 * The raw values in seconds are
	 * read from the .pak files in 
	 * vehicle_reader.cc, but the
	 * scale is not available there, 
	 * so they are stored here for
	 * use in the set_scale() method
	 * in simworld.cc
	 * @author: jamespetts
	 */
	uint16 max_loading_time_seconds;
	uint16 min_loading_time_seconds;

	bool available_only_as_upgrade; // If true, can not be bought as new: only upgraded.
	
	uint16 tractive_effort; // tractive effort / force in kN
	uint16 brake_force;		// The brake force in kN 
							// (that is, vehicle brake force, not the force of the brakes on the wheels;
							// this latter measure is commonly cited for deisel railway locomotives on
							// Wikipedia, but is no use here).

	float32e8_t air_resistance; // The "cf" value in physics calculations.
	float32e8_t rolling_resistance; // The "fr" value in physics calculations.

	// these values are not stored and therefore calculated in loaded():
	// they are arrays having one element per speed in m/s:
	uint32 max_speed;     // @author: Bernd Gabriel, May 27, 2012: length of the geared_* arrays (== maximum speed in m/s)
	uint32 *geared_power; // @author: Bernd Gabriel, Nov  4, 2009: == leistung * gear in W
	uint32 *geared_force; // @author: Bernd Gabriel, Dec 12, 2009: == tractive_effort * gear in N
	/**
	 * force threshold speed in km/h.
	 * Below this threshold the engine works as constant force engine.
	 * Above this threshold the engine works as constant power engine.
	 * @author Bernd Gabriel, Nov 4, 2009
	 */
	uint32 force_threshold_speed; // @author: Bernd Gabriel, Nov 4, 2009: in m/s

	// Obsolescence settings
	// @author: jamespetts
	uint16 increase_maintenance_after_years;
	uint16 increase_maintenance_by_percent;
	uint8 years_before_maintenance_max_reached;

	//@author: jamespetts; 28th of July 2012
	uint16 minimum_runway_length;

	// @author: Bernd Gabriel, Dec 12, 2009: called as last action in read_node()
	void loaded();

	int get_add_to_node() const 
	{ 
		return livery_image_type > 0 ? 5 : 6;
	}

public:
	// since we have a second constructor
	vehikel_besch_t() : geared_power(0), geared_force(0) { }

	// default vehicle (used for way seach and similar tasks)
	// since it has no images and not even a name knot any calls to this will case a crash
	vehikel_besch_t(uint8 wtyp, uint16 speed, engine_t engine) : geared_power(0), geared_force(0) {
		freight_image_type = livery_image_type = preis = upgrade_price = zuladung = overcrowded_capacity = running_cost = intro_date = vorgaenger = nachfolger = catering_level = upgrades = 0;
		fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
		leistung = gewicht = comfort = 1;
		gear = GEAR_FACTOR;
		//geared_power = GEAR_FACTOR;
		len = 8;
		sound = -1;
		typ = wtyp;
		engine_type = (uint8)engine;
		geschw = speed;
		is_tilting = bidirectional = can_lead_from_rear = available_only_as_upgrade = false;
		// These two lines are necessary for the building of way objects, so that they
		// do not get stuck with constraints. 
		way_constraints.set_permissive(0);
		way_constraints.set_prohibitive(255);
		min_loading_time = max_loading_time = (uint32)seconds_to_ticks(30, 250); 
		tractive_effort = 0;
		brake_force = BRAKE_FORCE_UNKNOWN;
		minimum_runway_length = 0;
	}

	virtual ~vehikel_besch_t()
	{
		delete [] geared_power;
		delete [] geared_force;
	}

	ware_besch_t const* get_ware() const { return get_child<ware_besch_t>(2); }

	skin_besch_t const* get_rauch() const { return get_child<skin_besch_t>(3); }

	image_id get_basis_bild() const { return get_bild_nr(ribi_t::dir_sued, get_ware() ); }
	image_id get_basis_bild(const char* livery) const { return get_bild_nr(ribi_t::dir_sued, get_ware(), livery ); }

	// returns the number of different directions
	uint8 get_dirs() const { return get_child<bildliste_besch_t>(4)->get_bild(4) ? 8 : 4; }

	// return a matching image
	// Vehicles can have single liveries, multiple liveries, 
	// single frieght images, multiple frieght images or no freight images.
	// they can have 4 or 8 directions ...
	image_id get_bild_nr(ribi_t::dir dir, const ware_besch_t *ware, const char* livery_type = "default") const
	{
		const bild_besch_t *bild=0;
		const bildliste_besch_t *liste=0;

		if(zuladung == 0 && ware)
		{
			ware = NULL;
		}

		if(livery_image_type > 0 && (ware == NULL || freight_image_type == 0))
		{
			// Multiple liveries, empty images
			uint8 livery_index = 0;
			if(strcmp(livery_type, "default"))
			{
				const uint8 freight_images = freight_image_type == 255 ? 1 : freight_image_type;
				for(uint8 i = 0; i < livery_image_type; i++) 
				{
					if(!strcmp(livery_type, get_child<text_besch_t>(5 + nachfolger + vorgaenger + upgrades + freight_images + i)->get_text()))
					{
						livery_index = i;
						break;
					}
				}
			}
			// vehicle has multiple liveries - get the appropriate one (if no list then fallback to livery zero)
			bildliste2d_besch_t const* const liste2d = get_child<bildliste2d_besch_t>(4);
			bild=liste2d->get_bild(dir, livery_index);
			
			if(!bild) 
			{
				if(dir>3)
				{
					bild = liste2d->get_bild(dir - 4, livery_index);
				}
			}
			if (bild != NULL) return bild->get_nummer();
		}

		if(livery_image_type > 0 && freight_image_type == 255 && ware != NULL)
		{
			// Multiple liveries, single freight image
			// freight_image_type == 255 means that there is a single freight image and multiple liveries.
			uint8 livery_index = 0;
			if(strcmp(livery_type, "default"))
			{
				// With the "default" livery, always select livery index 0
				for(uint8 i = 0; i < livery_image_type; i++) 
				{
					if(!strcmp(livery_type, get_child<text_besch_t>(6 + nachfolger + vorgaenger + upgrades + i)->get_text()))
					{
						livery_index = i;
						break;
					}
				}
			}
			// vehicle has multiple liveries - get the appropriate one (if no list then fallback to livery zero)
			bildliste2d_besch_t const* const liste2d = get_child<bildliste2d_besch_t>(5);
			bild = liste2d->get_bild(dir, livery_index);
			
			if(!bild) 
			{
				if(dir>3)
				{
					bild = liste2d->get_bild(dir - 4, livery_index);
				}
			}
			if (bild != NULL) return bild->get_nummer();
		}

		if(freight_image_type > 0 && freight_image_type < 255 && ware!=NULL && livery_image_type == 0)
		{
			// Multiple freight images, single livery
			// more freight images and a freight: find the right one

			sint8 ware_index = 0; // freight images: if not found use first freight
			
			for( uint8 i=0;  i<freight_image_type;  i++  ) 
			{
				
				if (ware == get_child<ware_besch_t>(6 + nachfolger + vorgaenger + upgrades + i)) 
				{
					ware_index = i;
					break;
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			bildliste2d_besch_t const* const liste2d = get_child<bildliste2d_besch_t>(5);
			bild=liste2d->get_bild(dir, ware_index);
			if(!bild) 
			{
				if(dir>3)
				{
					bild = liste2d->get_bild(dir - 4, ware_index);
				}
			}
			if (bild != NULL) return bild->get_nummer();
		}

		if(freight_image_type > 0 && freight_image_type < 255 && ware != NULL && livery_image_type > 0)
		{
			// Multiple freight images, multiple liveries

			sint8 ware_index = 0; // freight images: if not found use first freight
			uint8 livery_index = 0;

			for( uint8 i=0;  i<freight_image_type;  i++  ) 
			{
				if (ware == get_child<ware_besch_t>(6 + nachfolger + vorgaenger + upgrades + i)) 
				{
					ware_index = i;
					break;
				}
			}

			if(strcmp(livery_type, "default"))
			{
				for(uint8 j = 0; j < livery_image_type; j++) 
				{
					if(!strcmp(livery_type, get_child<text_besch_t>(6 + nachfolger + vorgaenger + freight_image_type + upgrades + j)->get_text()))
					{
						livery_index = j;
						break;
					}
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			bildliste3d_besch_t const* const liste3d = get_child<bildliste3d_besch_t>(5);
			bild = liste3d->get_bild(dir, livery_index, ware_index);
			if(!bild) 
			{
				if(dir>3)
				{
					bild = liste3d->get_bild(dir - 4, livery_index, ware_index);
				}
			}
			if (bild != NULL) return bild->get_nummer();
		}

		// only try 1d freight image list for old style vehicles
		if((freight_image_type == 0 || freight_image_type == 255) && ware != NULL && livery_image_type == 0) 
		{
			// Single freight image, single livery
			liste = get_child<bildliste_besch_t>(5);
		}

		if(!liste) 
		{
			liste = get_child<bildliste_besch_t>(4);
			if(!liste)
			{
				return IMG_LEER;
			}
		}

		bild = liste->get_bild(dir);
		if(!bild) 
		{
			if(dir>3)
			{
				bild = liste->get_bild(dir - 4);
			}
			if(!bild) 
			{
				return IMG_LEER;
			}
		}
		return bild->get_nummer();
	}

	bool check_livery(const char* name) const
	{
		// Note: this only checks empty images. The assumption is
		// that a livery defined for empty images will also be 
		// defined for freight images. If that assumption is false,
		// the default livery will be used for freight images.
		if(livery_image_type > 0)
		{
			for(sint8 i = 0; i < livery_image_type; i++) 
			{
				const uint8 freight_images = freight_image_type == 255 ? 1 : freight_image_type;
				const char* livery_name = get_child<text_besch_t>(5 + nachfolger + vorgaenger + upgrades + freight_images + i)->get_text();
				if(!strcmp(name, livery_name))
				{
					return true;
				}
			}
			return false;
		}
		else
		{
			return false;
		}
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
		return get_child<vehikel_besch_t>(get_add_to_node() + i);
	}

	// Liefert die erlaubten Nachfolger.
	// liefert get_nachfolger(0) == NULL, so bedeutet das entweder alle
	// Nachfolger sind erlaubt oder keine. Um das zu unterscheiden, sollte
	// man vorher hat_nachfolger() befragen
	const vehikel_besch_t *get_nachfolger(int i) const
	{
		if(i < 0 || i >= nachfolger) {
			return NULL;
		}
		return get_child<vehikel_besch_t>(get_add_to_node() + vorgaenger + i);
	}

	int get_nachfolger_count() const { return nachfolger; }

	/* returns true, if this veh can be before the next_veh
	 * uses NULL to indicate end of convoi
	 */
	bool can_lead(const vehikel_besch_t *next_veh) const
	{
		if(nachfolger == 0) 
		{
			if(can_be_at_rear)
			{
				return true;
			}
			else
			{
				if(next_veh != NULL)
				{
					return true;
				}
			}
		}

		for( int i=0;  i<nachfolger;  i++  ) {
			vehikel_besch_t const* const veh = get_child<vehikel_besch_t>(get_add_to_node() + vorgaenger + i);
			if(veh==next_veh) {
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	/* returns true, if this veh can be after the prev_veh
	 * uses NULL to indicate front of convoi
	 */
	bool can_follow(const vehikel_besch_t *prev_veh) const
	{
		if(  vorgaenger==0  ) {
			return true;
		}
		for( int i=0;  i<vorgaenger;  i++  ) 
		{
			vehikel_besch_t const* const veh = get_child<vehikel_besch_t>(get_add_to_node() + i);
			if(veh==prev_veh) 
			{
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	int get_vorgaenger_count() const { return vorgaenger; }

	// Returns the vehicle types to which this vehicle type may be upgraded.

	const vehikel_besch_t *get_upgrades(int i) const
	{
		if(i < 0 || i >= upgrades)
		{
			return NULL;
		}
		return get_child<vehikel_besch_t>(get_add_to_node() + nachfolger + vorgaenger + i);
	}

	int get_upgrades_count() const { return upgrades; }

	bool can_follow_any() const { return nachfolger==0; }

	waytype_t get_waytype() const { return static_cast<waytype_t>(typ); }
	uint16 get_zuladung() const { return zuladung; }
	uint32 get_preis() const { return preis; }
	sint32 get_geschw() const { return geschw; }
	uint16 get_gewicht() const { return gewicht; }
	uint16 get_running_cost() const { return running_cost; }
	uint16 get_running_cost(karte_t *welt) const; //Overloaded method - includes increase for obsolescence.
	uint32 get_fixed_cost() const { return fixed_cost; }
	uint32 get_fixed_cost(karte_t *welt) const;  //Overloaded method - includes increase for obsolescence.
	uint32 get_adjusted_monthly_fixed_cost(karte_t *welt) const; // includes increase for obsolescence and adjustment for monthly figures
	uint16 get_axle_load() const { return axle_load; } /* New Standard - not implemented yet */
	//uint16 get_maintenance() const { return fixed_cost; } /* New Standard - not implemented yet */
	sint8 get_sound() const { return sound; }
	bool is_bidirectional() const { return bidirectional; }
	bool get_can_lead_from_rear() const { return can_lead_from_rear; }
	uint8 get_comfort() const { return comfort; }
	uint16 get_overcrowded_capacity() const { return overcrowded_capacity; }
	uint32 get_min_loading_time() const { return zuladung > 0 ? min_loading_time : 0; }
	uint32 get_max_loading_time() const { return zuladung > 0 ? max_loading_time : 0; }
	uint32 get_upgrade_price() const { return upgrade_price; }
	bool is_available_only_as_upgrade() const { return available_only_as_upgrade; }

	// BG, 15.06.2009: the formula for obsolescence formerly implemented twice in get_running_cost() and get_fixed_cost()
	uint32 calc_running_cost(const karte_t *welt, uint32 base_cost) const;	

	float32e8_t get_power_force_ratio() const;
	uint32 calc_max_force(const uint32 power) const { 
		return power ? (uint32)(power / get_power_force_ratio() + float32e8_t::half) : 0; 
	}
	uint32 calc_max_power(const uint32 force) const { 
		return force ? (uint32)(force * get_power_force_ratio()) : 0; 
	}
	uint32 get_leistung() const { 
		return leistung ? leistung : calc_max_power(tractive_effort); 
	}
	uint32 get_tractive_effort() const { 
		return tractive_effort ? tractive_effort : calc_max_force(leistung);
	}

	uint16 get_brake_force() const { return brake_force; }

	uint16 get_minimum_runway_length() const { return minimum_runway_length; }
	
	/**
	* @return introduction year
	* @author Hj. Malthaner
	*/
	uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return time when no longer in production
	* @author prissi
	*/
	uint16 get_retire_year_month() const { return obsolete_date; }

	/**
	* @return time when the vehicle is obsolete
	* @author: jamespetts
	*/
	uint16 get_obsolete_year_month(const karte_t *welt) const;

	// true if future
	bool is_future (const uint16 month_now) const
	{
		return month_now  &&  (intro_date > month_now);
	}

	/**
	* @ Returns true if the vehicle is no longer in production
	* @author: prissi
	*/
	bool is_retired (const uint16 month_now) const
	{
		return month_now  &&  (obsolete_date <= month_now);
	}

	/**
	* @ Returns true if the vehicle is obsolete
	* @author: 
	*/
	bool is_obsolete (const uint16 month_now, const karte_t* welt) const
	{
		return month_now  &&  (get_obsolete_year_month(welt) <= month_now);
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

	uint32 get_length_in_steps() const { return get_length() * VEHICLE_STEPS_PER_CARUNIT; }
	
	/*Whether this is a tilting train (and can take coerners faster
	*@author: jamespetts*/
	bool get_tilting() const { return is_tilting;	}

	bool get_can_be_at_rear() const { return can_be_at_rear; }

	float32e8_t get_air_resistance() const { return air_resistance; }
	float32e8_t get_rolling_resistance() const { return rolling_resistance; }
	
	const way_constraints_of_vehicle_t& get_way_constraints() const { return way_constraints; }
	void set_way_constraints(const way_constraints_of_vehicle_t& value) { way_constraints = value; }
	
	/*The level of catering provided by this vehicle (0 if none)
	*@author: jamespetts*/
	uint8 get_catering_level() const { return catering_level; }

	void set_scale(uint16 scale_factor)
	{ 
		const uint32 scaled_price = (uint32) set_scale_generic<sint64>(base_price, scale_factor);
		const uint32 scaled_upgrade_price = (uint32) set_scale_generic<sint64>(base_upgrade_price, scale_factor);
		const uint32 scaled_maintenance = set_scale_generic<uint32>(base_fixed_cost, scale_factor);

		preis = (base_price == 0 ? 0 : (scaled_price >= 1 ? scaled_price : 1));
		upgrade_price = (base_upgrade_price == 0 ? 0 : (scaled_upgrade_price >= 1 ? scaled_upgrade_price : 1));
		fixed_cost = (uint32)(base_fixed_cost == 0 ? 0 :(scaled_maintenance >= 1 ? scaled_maintenance : 1));

		if(max_loading_time_seconds != 65535)
		{
			max_loading_time = (uint32)seconds_to_ticks(max_loading_time_seconds, scale_factor);
		}
		if(min_loading_time_seconds != 65535)
		{
			min_loading_time = (uint32)seconds_to_ticks(min_loading_time_seconds, scale_factor);
		}
	}

	/**
	 * Get effective force index. 
	 * Steam engine's force depend on its speed.
	 * Effective force in N: force_index *welt->get_settings().get_global_power_factor() / GEAR_FACTOR
	 * @author Bernd Gabriel
	 */
	uint32 get_effective_force_index(sint32 speed /* in m/s */ ) const;

	/**
	 * Get effective power index. 
	 * Steam engine's power depend on its speed.
	 * Effective power in W: power_index *welt->get_settings().get_global_power_factor() / GEAR_FACTOR
	 * @author Bernd Gabriel
	 */
	uint32 get_effective_power_index(sint32 speed /* in m/s */ ) const;

	void calc_checksum(checksum_t *chk) const;

	static uint32 get_air_default(sint8 waytype)
	{
		switch(waytype)
		{
			case track_wt:
			case tram_wt:
			case monorail_wt:		return 160L; //1.6 when read
			
			case narrowgauge_wt:	return 120L; //1.2 when read

			case water_wt:			return 2500L; //25 when read

			case maglev_wt:			return 145L; //1.45 when read

			case air_wt:			return 100L; //1 when read

			case road_wt:			
			default:				return 15L; //0.15 when read
		};
	}  

	static uint32 get_rolling_default(sint8 waytype)
	{
		switch(waytype)
		{			
			case track_wt:
			case tram_wt:
			case monorail_wt:		return 15L; //0.0015 when read

			case narrowgauge_wt:	return 17L; //0.0017 when read

			case air_wt:
			case water_wt:			return 10L; //0.001 when read
			
			case maglev_wt:			return 13L; //0.0013 when read

			default:
			case road_wt:			return 50L; //0.005 when read
		};
	}
};

#endif
