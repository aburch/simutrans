/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */
#ifndef __VEHIKEL_BESCH_H
#define __VEHIKEL_BESCH_H
#include <cstring>

#include "obj_base_desc.h"
#include "goods_desc.h"
#include "image_list.h"
#include "image_array.h"
#include "image_array_3d.h"
#include "skin_desc.h"
#include "sound_desc.h"
#include "../dataobj/ribi.h"
#include "../dataobj/way_constraints.h"
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
class vehicle_desc_t : public obj_desc_transport_related_t {
    friend class vehicle_reader_t;
    friend class vehicle_builder_t;

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
		battery,
		petrol,
		turbine,
		MAX_TRACTION_TYPE
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
		case petrol:
			return "petrol";
		case turbine:
			return "turbine";
		}
	}

	enum veh_basic_constraint_t {
		cannot_be_at_end = 0,
		can_be_head  = 1,
		can_be_tail  = 2,
		unconnectable = 4,
		intermediate_unique = 8,
		unknown_constraint = 128,
		only_at_front =(can_be_head|unconnectable),
		only_at_end = (can_be_tail|unconnectable) // this type always be bidirectional=0
	};

private:
	uint32 upgrade_price;			// Price if this vehicle is bought as an upgrade, not a new vehicle.
	uint32 base_upgrade_price;		// Upgrade price (without scale factor)
	uint16 *capacity;				// Payload (pointer to an array of capacities per class)
	uint16 overcrowded_capacity;	// The capacity of a vehicle if overcrowded (usually expressed as the standing capacity).
	uint32 weight;					// Weight in kg
	uint32 power;					// Power in kW
	uint16 running_cost;			// Per kilometre cost
	uint32 fixed_cost;				// Monthly cost @author: jamespetts, April 2009
	uint32 base_fixed_cost;			// Monthly cost (without scale factor)

	uint16 gear;					// engine gear (power multiplier), 64=100

	uint8 len;						// length (=8 is half a tile, the old default)
	sint16 sound;

	uint8 leader_count;				// all defined leading vehicles
	uint8 trailer_count;			// all defined trailer
	uint8 upgrades;					// The number of vehicles that are upgrades of this vehicle.

	uint8 engine_type;				// diesel, steam, electric (requires electrified ways), fuel_cell, etc.

	uint8 freight_image_type;		// number of freight images (displayed for different goods)
	uint8 livery_image_type;		// Number of different liveries (@author: jamespetts, April 2011)
	
	bool is_tilting;				// Whether it is a tilting train (can take corners at higher speeds). 0 for no, 1 for yes. Anything other than 1 is assumed to be no.
	
	way_constraints_of_vehicle_t way_constraints;

	uint8 catering_level;			// The level of catering. 0 for no catering. Higher numbers for better catering.

	bool bidirectional = false;		// Whether must always travel in one direction
	bool can_lead_from_rear = false;	// Whether vehicle can lead a convoy when it is at the rear.            Ranran: This parameter is obsolete and is now included in basic_constraint_next.
	bool can_be_at_rear = true;		// Whether the vehicle may be at the rear of a convoy (default = true). Ranran: It is used to read the old pak, and the flag takes over to the basic_constraint_next.
	uint8 basic_constraint_prev = can_be_head;
	uint8 basic_constraint_next = can_be_tail;

	uint8 *comfort;					// How comfortable that a vehicle is for passengers. (Pointer to an array of comfort levels per class)

	uint8 classes;					// The number of different classes that this vehicle accommodates

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
	uint32 *geared_power; // @author: Bernd Gabriel, Nov  4, 2009: == power * gear in W
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

	// The maximum number of km between stops (not waypoints)
	uint16 range;

	// The level of wear to which this vehicle subjects a way
	// measured in Standard Axle loads (8t) * 10,000. 
	uint32 way_wear_factor;

	// Whether this vehicle may be allowed to pass under
	// a restricted height bridge.
	//@jamespetts January 2017
	bool is_tall;

	// if true, can not mix another goods in the same car.  @Ranran, July 2019(v14.6)
	bool mixed_load_prohibition;

	// @author: Bernd Gabriel, Dec 12, 2009: called as last action in read_node()
	void loaded();

	int get_add_to_node() const 
	{ 
		if(freight_image_type > 1 && livery_image_type > 1)
		{
			return 6;
		}
		int i = freight_image_type == 255 ? 1 : 0;
		return livery_image_type > 0 ? 5 + i : 6;
	}

	// set basic constraint to the old version data. v14.8, 2020 @Ranran
	void fix_basic_constraint();

public:
	// since we have a second constructor
	vehicle_desc_t() : geared_power(0), geared_force(0) { }

	// default vehicle (used for way search and similar tasks)
	// since it has no images and not even a name node any calls to this will case a crash
	// Also, capacity and comfort are not initialised here
	vehicle_desc_t(uint8 wtype, uint16 speed, engine_t engine, uint16 al = 0, uint32 weight = 1) : geared_power(0), geared_force(0) {
		freight_image_type = livery_image_type = price = upgrade_price = overcrowded_capacity = running_cost = intro_date = leader_count = trailer_count = catering_level = upgrades = 0;
		fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
		classes = power = 1;
		gear = GEAR_FACTOR;
		len = 8;
		sound = -1;
		wtyp = wtype;
		axle_load = al;
		weight = weight;
		engine_type = (uint8)engine;
		topspeed = speed;
		mixed_load_prohibition = false;
		is_tilting = false;
		bidirectional = false;
		can_lead_from_rear = false;
		can_be_at_rear = false;
		available_only_as_upgrade = false;
		basic_constraint_prev = can_be_head;
		basic_constraint_next = can_be_tail;
		// These two lines are necessary for the building of way objects, so that they
		// do not get stuck with constraints. 
		way_constraints.set_permissive(0);
		way_constraints.set_prohibitive(255);
#ifndef NETTOOL
		min_loading_time = max_loading_time = (uint32)seconds_to_ticks(30, 250); 
#endif
		tractive_effort = 0;
		brake_force = BRAKE_FORCE_UNKNOWN;
		minimum_runway_length = 3;
		range = 0;
		comfort = NULL;
		capacity = NULL;
	}

	virtual ~vehicle_desc_t()
	{
		delete[] geared_power;
		delete[] geared_force;
		delete[] comfort;
		delete[] capacity;
	}

	goods_desc_t const* get_freight_type() const { return get_child<goods_desc_t>(2); }

	skin_desc_t const* get_smoke() const { return get_child<skin_desc_t>(3); }

	image_id get_base_image() const { return get_image_id(ribi_t::dir_south, get_freight_type() ); }
	image_id get_base_image(const char* livery) const { return get_image_id(ribi_t::dir_south, get_freight_type(), livery ); }

	// returns the number of different directions
	uint8 get_dirs() const { return get_child<image_list_t>(4)->get_image(4) ? 8 : 4; }

	// return a matching image
	// Vehicles can have single liveries, multiple liveries, 
	// single frieght images, multiple frieght images or no freight images.
	// they can have 4 or 8 directions ...
	image_id get_image_id(ribi_t::dir dir, const goods_desc_t *ware, const char* livery_type = "default") const
	{
		const image_t *image=0;
		const image_list_t *list=0;

		if(capacity == 0 && ware)
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
					if(!strcmp(livery_type, get_child<text_desc_t>(5 + trailer_count + leader_count + upgrades + freight_images + i)->get_text()))
					{
						livery_index = i;
						break;
					}
				}
			}
			// vehicle has multiple liveries - get the appropriate one (if no list then fallback to livery zero)
			image_array_t const* const list2d = get_child<image_array_t>(4);
			image=list2d->get_image(dir, livery_index);
			
			if(!image) 
			{
				if(dir>3)
				{
					image = list2d->get_image(dir - 4, livery_index);
				}
			}
			if (image != NULL) return image->get_id();
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
					if(!strcmp(livery_type, get_child<text_desc_t>(6 + trailer_count + leader_count + upgrades + i)->get_text()))
					{
						livery_index = i;
						break;
					}
				}
			}
			// vehicle has multiple liveries - get the appropriate one (if no list then fallback to livery zero)
			image_array_t const* const list2d = get_child<image_array_t>(5);
			image = list2d->get_image(dir, livery_index);
			
			if(!image) 
			{
				if(dir>3)
				{
					image = list2d->get_image(dir - 4, livery_index);
				}
			}
			if (image != NULL) return image->get_id();
		}

		if(freight_image_type > 0 && freight_image_type < 255 && ware!=NULL && livery_image_type == 0)
		{
			// Multiple freight images, single livery
			// more freight images and a freight: find the right one

			sint8 goods_index = 0; // freight images: if not found use first freight
			
			for( uint8 i=0;  i<freight_image_type;  i++  ) 
			{
				
				if (ware == get_child<goods_desc_t>(6 + trailer_count + leader_count + upgrades + i)) 
				{
					goods_index = i;
					break;
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			image_array_t const* const list2d = get_child<image_array_t>(5);
			image=list2d->get_image(dir, goods_index);
			if(!image) 
			{
				if(dir>3)
				{
					image = list2d->get_image(dir - 4, goods_index);
				}
			}
			if (image != NULL) return image->get_id();
		}

		if(freight_image_type > 0 && freight_image_type < 255 && ware != NULL && livery_image_type > 0)
		{
			// Multiple freight images, multiple liveries

			sint8 goods_index = 0; // freight images: if not found use first freight
			uint8 livery_index = 0;

			for( uint8 i=0;  i<freight_image_type;  i++  ) 
			{
				if (ware == get_child<goods_desc_t>(6 + trailer_count + leader_count + upgrades + i)) 
				{
					goods_index = i;
					break;
				}
			}

			if(strcmp(livery_type, "default"))
			{
				for(uint8 j = 0; j < livery_image_type; j++) 
				{
					if(!strcmp(livery_type, get_child<text_desc_t>(6 + trailer_count + leader_count + freight_image_type + upgrades + j)->get_text()))
					{
						livery_index = j;
						break;
					}
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			image_array_3d_t const* const list3d = get_child<image_array_3d_t>(5);
			if (!list3d)
			{
				dbg->warning("image_id get_image_id()", "Cannot read 3d image list from memory");
				return IMG_EMPTY;
			}
			image = list3d->get_image(dir, livery_index, goods_index);
			if(!image) 
			{
				if(dir>3)
				{
					image = list3d->get_image(dir - 4, livery_index, goods_index);
				}
			}
			if (image != NULL) return image->get_id();
		}

		// only try 1d freight image list for old style vehicles
		if((freight_image_type == 0 || freight_image_type == 255) && ware != NULL && livery_image_type == 0) 
		{
			// Single freight image, single livery
			list = get_child<image_list_t>(5);
		}

		if(!list) 
		{
			list = get_child<image_list_t>(4);
			if(!list)
			{
				return IMG_EMPTY;
			}
		}

		image = list->get_image(dir);
		if(!image) 
		{
			if(dir>3)
			{
				image = list->get_image(dir - 4);
			}
			if(!image) 
			{
				return IMG_EMPTY;
			}
		}
		return image->get_id();
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
				const char* livery_name = get_child<text_desc_t>(5 + trailer_count + leader_count + upgrades + freight_images + i)->get_text();
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
	uint8 get_livery_count() const {
		return livery_image_type;
	}
	uint16 get_available_livery_count(class karte_t *welt) const;

	/**
	 * Returns allowed leader vehicles.
	 * If get_leader(0) == NULL then either all or no leaders are allowed.
	 * To distinguish these cases check get_leader_count().
	 */
	const vehicle_desc_t *get_leader(int i) const
	{
		if(i < 0 || i >= leader_count) {
			return NULL;
		}
		return get_child<vehicle_desc_t>(get_add_to_node() + i);
	}

	/**
	 * Returns vehicles that this vehicle is allowed to pull.
	 * If get_trailer(0) == NULL then either all or no followers are allowed.
	 * To distinguish these cases check get_trailer_count().
	 */
	const vehicle_desc_t *get_trailer(int i) const
	{
		if(i < 0 || i >= trailer_count) {
			return NULL;
		}
		return get_child<vehicle_desc_t>(get_add_to_node() + leader_count + i);
	}

	int get_trailer_count() const { return trailer_count; }

	/* returns true, if this veh can be before the next_veh
	 * uses NULL to indicate end of convoi
	 */
	bool can_lead(const vehicle_desc_t *next_veh) const
	{
		if (basic_constraint_next & unconnectable && next_veh) {
			return false;
		}
		if(trailer_count == 0) 
		{
			if(basic_constraint_next & can_be_tail)
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

		for( int i=0;  i<trailer_count;  i++  ) {
			vehicle_desc_t const* const veh = get_child<vehicle_desc_t>(get_add_to_node() + leader_count + i);
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
	bool can_follow(const vehicle_desc_t *prev_veh) const
	{
		if (basic_constraint_prev & unconnectable && prev_veh) {
			return false;
		}
		if(  leader_count==0  ) {
			if (basic_constraint_prev & can_be_head)
			{
				return true;
			}
			else
			{
				if (prev_veh != NULL)
				{
					return true;
				}
			}
		}
		for( int i=0;  i<leader_count;  i++  ) 
		{
			vehicle_desc_t const* const veh = get_child<vehicle_desc_t>(get_add_to_node() + i);
			if(veh==prev_veh) 
			{
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	int get_leader_count() const { return leader_count; }

	// Returns the vehicle types to which this vehicle type may be upgraded.

	const vehicle_desc_t *get_upgrades(int i) const
	{
		if(i < 0 || i >= upgrades)
		{
			return NULL;
		}
		return get_child<vehicle_desc_t>(get_add_to_node() + trailer_count + leader_count + i);
	}

	int get_upgrades_count() const { return upgrades; }
	// returns this vehicle has available upgrades
	// 1 = near future, 2 = already available          @Ranran
	uint8 has_available_upgrade(const uint16 month_now, bool show_future = true) const;

	bool can_follow_any() const { return trailer_count==0; }

	uint8 get_number_of_classes() const { return classes; }
	uint16 get_capacity(uint32 g_class = 0) const { return classes == 0 ? 0 : g_class >= classes ? capacity[0] : capacity[g_class]; }
	uint16 get_total_capacity() const
	{
		uint16 cap = 0;
		for (uint8 i = 0; i < classes; i++)
		{
			cap += get_capacity(i);
		}
		return cap;
	}
	void fix_number_of_classes();
	uint32 get_weight() const { return weight; }
	uint16 get_running_cost() const { return running_cost; }
	uint16 get_running_cost(const class karte_t *welt) const; //Overloaded method - includes increase for obsolescence.
	uint32 get_fixed_cost() const { return fixed_cost; }
	uint32 get_fixed_cost(class karte_t *welt) const;  //Overloaded method - includes increase for obsolescence.
	uint32 get_adjusted_monthly_fixed_cost(class karte_t *welt) const; // includes increase for obsolescence and adjustment for monthly figures
	sint16 get_sound() const { return sound; }
	bool is_bidirectional() const { return bidirectional; }
	uint8 get_comfort(uint32 g_class = 0) const { return  classes == 0 ? 0: g_class >= classes ? comfort[0] : comfort[g_class]; }
	uint16 get_overcrowded_capacity() const { return overcrowded_capacity; }
	uint32 get_min_loading_time() const { return get_total_capacity() > 0 ? min_loading_time : 0; }
	uint32 get_max_loading_time() const { return get_total_capacity() > 0 ? max_loading_time : 0; }
	uint32 get_upgrade_price() const { return upgrade_price; }
	bool is_available_only_as_upgrade() const { return available_only_as_upgrade; }

	uint8 get_adjusted_comfort(uint8 catering_level, uint8 g_class = 0) const
	{
		if (g_class >= classes)
		{
			g_class = 0;
		}
		uint32 modified_comfort = (uint32)comfort[g_class];
		switch(catering_level)
		{
		case 0:
			modified_comfort = modified_comfort > 0 ? modified_comfort : 1;
			break;

		case 1:
			modified_comfort += 5;
			break;

		case 2:
			modified_comfort += 10;
			break;

		case 3:
			modified_comfort += 16;
			break;

		case 4:
			modified_comfort += 20;
			break;

		case 5:
		default:
			modified_comfort += 25;
			break;
		};
		return min(255, modified_comfort);
	}

	// BG, 15.06.2009: the formula for obsolescence formerly implemented twice in get_running_cost() and get_fixed_cost()
	uint32 calc_running_cost(const class karte_t *welt, uint32 base_cost) const;	

	float32e8_t get_power_force_ratio() const;
	uint32 calc_max_force(const uint32 power) const { 
		return power ? (uint32)(power / get_power_force_ratio() + float32e8_t::half) : 0; 
	}
	uint32 calc_max_power(const uint32 force) const { 
		return force ? (uint32)(force * get_power_force_ratio()) : 0; 
	}
	uint32 get_power() const { 
		return power ? power : calc_max_power(tractive_effort); 
	}
	uint32 get_tractive_effort() const { 
		return tractive_effort ? tractive_effort : calc_max_force(power);
	}

	uint16 get_brake_force() const { return brake_force; }

	uint16 get_minimum_runway_length() const { return minimum_runway_length; }

	uint16 get_range() const { return range; }
	
	// returns bit flags of bidirectional and has power for drawing formation picture
	uint8 get_interactivity() const;

	/**
	* @return introduction year
	* @author Hj. Malthaner
	*/
	uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return time when no longer in production
	* @author prissi
	*/
	uint16 get_retire_year_month() const { return retire_date; }

	/**
	* @return time when the vehicle is obsolete
	* @author: jamespetts
	*/
	uint16 get_obsolete_year_month(const class karte_t *welt) const;

	// Returns 2 in the near future. Use the judgment of 2 only when control the display of the future
	uint8 is_future (const uint16 month_now) const
	{
		return (!month_now || (intro_date - month_now <= 0)) ? 0 : (intro_date - month_now < 12) ? 2 : 1;
	}


	bool will_end_prodection_soon(const uint16 month_now) const
	{
		return month_now && (retire_date - month_now) < 12 && (retire_date - month_now) > 0;
	}

	/**
	* @ Returns true if the vehicle is no longer in production
	* @author: prissi
	*/
	bool is_retired (const uint16 month_now) const
	{
		return month_now  &&  (retire_date <= month_now);
	}

	/**
	* @ Returns true if the vehicle is obsolete
	* @author: 
	*/
	bool is_obsolete (const uint16 month_now, const class karte_t* welt) const
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
	uint16 get_engine_type() const { return engine_type; }

	/* @return the vehicles length in 1/8 of the normal len
	* @author prissi
	*/
	uint8 get_length() const { return len; }

	uint32 get_length_in_steps() const { return get_length() * VEHICLE_STEPS_PER_CARUNIT; }
	
	/*Whether this is a tilting train (and can take coerners faster
	*@author: jamespetts*/
	bool get_tilting() const { return is_tilting;	}

	// Returns whether one side of the vehicle can be "head".
	// Note: Normally the front side is checked, but it is necessary to check the rear side when vehicle is reversed or before reversing convoy. (Ranran)
	bool get_can_be_at_front(bool chk_rear_end) const {
		if (chk_rear_end ? basic_constraint_next & can_be_head : basic_constraint_prev & can_be_head) {
			return true;
		}
		return false;
	}
	// Returns whether one side of the vehicle can be "tail end".
	// Note: Normally the rear side is checked, but it is necessary to check the front side when vehicle is reversed or before reversing convoy. (Ranran)
	bool get_can_be_at_rear(bool chk_front_end) const {
		if (chk_front_end ? basic_constraint_prev & can_be_head || basic_constraint_prev & can_be_tail
			: basic_constraint_next & can_be_head || basic_constraint_next & can_be_tail)
		{
			return true;
		}
		return false;
	}
	// return basic coupling constraint flags
	uint8 get_basic_constraint_next(bool reversed = false) const { return reversed ? basic_constraint_prev : basic_constraint_next; }
	uint8 get_basic_constraint_prev(bool reversed = false) const { return reversed ? basic_constraint_next : basic_constraint_prev; }

	float32e8_t get_air_resistance() const { return air_resistance; }
	float32e8_t get_rolling_resistance() const { return rolling_resistance; }
	
	const way_constraints_of_vehicle_t& get_way_constraints() const { return way_constraints; }
	void set_way_constraints(const way_constraints_of_vehicle_t& value) { way_constraints = value; }
	
	/*The level of catering provided by this vehicle (0 if none)
	*@author: jamespetts*/
	uint8 get_catering_level() const { return catering_level; }

	uint32 get_way_wear_factor() const { return way_wear_factor; }

	bool get_is_tall() const { return is_tall; }
	bool get_mixed_load_prohibition() const { return mixed_load_prohibition; }

	void set_scale(uint16 scale_factor, uint32 way_wear_factor_rail, uint32 way_wear_factor_road, uint16 standard_axle_load)
	{ 
		obj_desc_transport_related_t::set_scale(scale_factor);

		upgrade_price = (uint32) set_scale_generic<sint64>(base_upgrade_price, scale_factor);
		if (base_upgrade_price && !upgrade_price) upgrade_price = 1;

		fixed_cost = set_scale_generic<uint32>(base_fixed_cost, scale_factor);
		if (base_fixed_cost && ! fixed_cost) fixed_cost = 1;
#ifndef NETTOOL
		if(max_loading_time_seconds != 65535)
		{
			max_loading_time = (uint32)seconds_to_ticks(max_loading_time_seconds, scale_factor);
		}
		if(min_loading_time_seconds != 65535)
		{
			min_loading_time = (uint32)seconds_to_ticks(min_loading_time_seconds, scale_factor);
		}
#endif
		if(way_wear_factor == UINT32_MAX_VALUE) 
		{
			// Uninitialised. Set it here, as cannot set it on reading because we need welt, and reading is static.
			uint32 power;

			switch(get_waytype())
			{
			case monorail_wt:
			case track_wt:
			case tram_wt:
			case narrowgauge_wt:
			case air_wt: // Runways should be like roads, but aircraft are so huge that it is unbalancable as such
				power = way_wear_factor_rail;
				break;
			case maglev_wt:
			case water_wt:
				power = 0;
				break;
			case road_wt:
			default:
				power = way_wear_factor_road; 
				break;
			};
			if(power > 0)
			{
				uint32 axles = axle_load ? (weight / axle_load) / 1000 : 1; // Weight is in kg.
				axles = max(axles, 1);
			
				float32e8_t adjusted_standard_axle((uint32)axle_load, (uint32)standard_axle_load);
				const float32e8_t adjusted_standard_axle_original = adjusted_standard_axle;
				float32e8_t adjusted_standard_axle_extra((uint32)weight % (uint32)axles); 
				adjusted_standard_axle_extra /= float32e8_t((uint32)1000, (uint32)1);
				const float32e8_t adjusted_standard_axle_original_extra = adjusted_standard_axle_extra;

				while(--power)
				{
					adjusted_standard_axle *= adjusted_standard_axle_original;
					adjusted_standard_axle_extra *= adjusted_standard_axle_original_extra;
				}

				// Add estimate of hammer blow for steam locomotives
				// See http://www.archive.org/stream/steelrailstheir02sellgoog/steelrailstheir02sellgoog_djvu.txt pp. 70-72 for details of this formula.
				// This assumes a 2 cylinder locomotive.
				if((get_waytype() == track_wt || get_waytype() == narrowgauge_wt) && power > 0 && engine_type == steam)
				{
					if(axle_load < 11)
					{
						if(topspeed < 90)
						{
							adjusted_standard_axle += (adjusted_standard_axle * float32e8_t((uint32)2, (uint32)3));
						}
						else
						{
							adjusted_standard_axle += (adjusted_standard_axle * float32e8_t((uint32)3, (uint32)4));
						}
					}
					else
					{
						adjusted_standard_axle += (adjusted_standard_axle * float32e8_t((uint32)26, (uint32)100));
					}
					adjusted_standard_axle_extra += (adjusted_standard_axle * float32e8_t((uint32)26, (uint32)100));
				}

				adjusted_standard_axle *= axles;
				adjusted_standard_axle += adjusted_standard_axle_extra;

				adjusted_standard_axle *= (uint32)10000; 			

				way_wear_factor = (uint32)adjusted_standard_axle.to_sint32();
			}
			else
			{
				way_wear_factor = 0;
			}
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

			case road_wt:			return 15L; //0.15 when read

			default:				return 15L; //0.15 when read
		};
	}  

	static uint32 get_rolling_default(sint8 waytype)
	{
		// See http://www.engineeringtoolbox.com/rolling-friction-resistance-d_1303.html
		switch(waytype)
		{			
			case track_wt:
			case monorail_wt:		return 15L; //0.0015 when read

			case tram_wt:			return 60L; //0.006 when read						

			case narrowgauge_wt:	return 17L; //0.0017 when read

			case air_wt:
			case water_wt:			return 10L; //0.001 when read
			
			case maglev_wt:			return 13L; //0.0013 when read

			default:
			case road_wt:			return 90L; //0.009 when read
		};
	}
};
#define vehicle_desc_t_defined

#endif
