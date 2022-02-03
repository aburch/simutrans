/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_VEHICLE_DESC_H
#define DESCRIPTOR_VEHICLE_DESC_H


#include "obj_base_desc.h"
#include "goods_desc.h"
#include "image_list.h"
#include "image_array.h"
#include "skin_desc.h"
#include "sound_desc.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../simunits.h"


class checksum_t;

/**
 * Vehicle type description - all attributes of a vehicle type
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   freight
 *  3   smoke
 *  4   empty 1d image list
 *  5   either 1d (freight_image_type==0) or 2d image list
 *  6   required leading vehicle 1
 *  7   required leading vehicle 2
 * ... ...
 *  n+5 required leading vehicle n
 *  n+6 allowed trailing vehicle 1
 *  n+7 allowed trailing vehicle 2
 * ... ...
 *  n+m+5 allowed trailing vehicle m
 *  n+m+6 freight for which special images are defined
 */
class vehicle_desc_t : public obj_desc_transport_related_t {
	friend class vehicle_reader_t;
	friend class vehicle_builder_t;

public:
	/**
	 * Engine type
	 */
	enum engine_t {
		unknown = -1,
		steam   = 0,
		diesel,
		electric,
		bio,
		sail,
		fuel_cell,
		hydrogene,
		battery
	};


private:
	uint16  capacity;
	uint16  loading_time; // time per full loading/unloading
	uint32  weight;
	uint32  power;
	uint16  running_cost;

	uint16  gear;       // engine gear (power multiplier), 64=100

	uint8 len;          // length (=8 is half a tile, the old default)
	sint8 sound;

	uint8  leader_count;  // all defined leading vehicles
	uint8  trailer_count; // all defined trailer

	uint8  engine_type; // diesel, steam, electric (requires electrified ways), fuel_cell, etc.

	sint8 freight_image_type; // number of freight images (displayed for different goods)

public:
	// dummy vehicle for the XREF reader
	static vehicle_desc_t *any_vehicle;

	// since we have a second constructor
	vehicle_desc_t() { }

	// default vehicle (used for way search and similar tasks)
	// since it has no images and not even a name node any calls to this will case a crash
	vehicle_desc_t(uint8 wtype, uint16 speed, engine_t engine) {
		maintenance = freight_image_type = price = capacity = axle_load = running_cost = intro_date = leader_count = trailer_count = 0;
		power = weight = 1;
		loading_time = 1000;
		gear = 64;
		len = 8;
		sound = -1;
		wtyp = wtype;
		engine_type = (uint8)engine;
		topspeed = speed;
	}

	goods_desc_t const* get_freight_type() const { return get_child<goods_desc_t>(2); }

	skin_desc_t const* get_smoke() const { return get_child<skin_desc_t>(3); }

	image_id get_base_image() const { return get_image_id(ribi_t::dir_south, get_freight_type() ); }

	// returns the number of different directions
	uint8 get_dirs() const { return get_child<image_list_t>(4)->get_image(4) ? 8 : 4; }

	// return a matching image
	// beware, there are three classes of vehicles
	// vehicles with and without freight images, and vehicles with different freight images
	// they can have 4 or 8 directions ...
	image_id get_image_id(ribi_t::dir dir, const goods_desc_t *ware) const
	{
		const image_t *image=0;
		const image_list_t *list=0;

		if(freight_image_type>0  &&  ware!=NULL) {
			// more freight images and a freight: find the right one

			sint8 goods_index=0; // freight images: if not found use first freight

			for( sint8 i=0;  i<freight_image_type;  i++  ) {
				if (ware == get_child<goods_desc_t>(6 + trailer_count + leader_count + i)) {
					goods_index = i;
					break;
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			image_array_t const* const list2d = get_child<image_array_t>(5);
			image=list2d->get_image(dir, goods_index);
			if(!image) {
				if(dir>3) {
					image = list2d->get_image(dir - 4, goods_index);
				}
			}
			if (image != NULL) return image->get_id();
		}

		// only try 1d freight image list for old style vehicles
		if(freight_image_type==0  &&  ware!=NULL) {
			list = get_child<image_list_t>(5);
		}

		if(!list) {
			list = get_child<image_list_t>(4);
			if(!list) {
				return IMG_EMPTY;
			}
		}

		image = list->get_image(dir);
		if(!image) {
			if(dir>3) {
				image = list->get_image(dir - 4);
			}
			if(!image) {
				return IMG_EMPTY;
			}
		}
		return image->get_id();
	}

	/**
	 * Returns allowed leader vehicles.
	 * If get_leader(0) == NULL then either all or no leaders are allowed.
	 * To distinguish these cases check get_leader_count().
	 */
	const vehicle_desc_t *get_leader(uint8 i) const
	{
		if(  i >= leader_count  ) {
			return 0;
		}
		return get_child<vehicle_desc_t>(6 + i);
	}

	uint8 get_leader_count() const { return leader_count; }

	/**
	 * Returns vehicles that this vehicle is allowed to pull.
	 * If get_trailer(0) == NULL then either all or no followers are allowed.
	 * To distinguish these cases check get_trailer_count().
	 */
	const vehicle_desc_t *get_trailer(uint8 i) const
	{
		if(  i >= trailer_count  ) {
			return 0;
		}
		return get_child<vehicle_desc_t>(6 + leader_count + i);
	}

	uint8 get_trailer_count() const { return trailer_count; }

	/* returns true, if this veh can be before the next_veh
	 * uses NULL to indicate end of convoi
	 */
	bool can_lead(const vehicle_desc_t *next_veh) const
	{
		if(  trailer_count==0  ) {
			return true;
		}
		for( uint8 i=0;  i<trailer_count;  i++  ) {
			vehicle_desc_t const* const veh = get_child<vehicle_desc_t>(6 + leader_count + i);
			if(  veh==next_veh  ) {
				return true;
			}
			// not leading and "any" => we can follow
			if(  next_veh!=NULL  &&  veh==vehicle_desc_t::any_vehicle  ) {
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
		if(  leader_count==0  ) {
			return true;
		}
		for( uint8 i=0;  i<leader_count;  i++  ) {
			vehicle_desc_t const* const veh = get_child<vehicle_desc_t>(6 + i);
			if(  veh==prev_veh  ) {
				return true;
			}
			// not leading and "any" => we can follow
			if(  prev_veh!=NULL  &&  veh==vehicle_desc_t::any_vehicle  ) {
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	bool can_follow_any() const { return trailer_count==0; }

	uint16 get_capacity() const { return capacity; }
	uint16 get_loading_time() const { return loading_time; } // ms per full loading/unloading
	uint32 get_weight() const { return weight; }
	uint32 get_power() const { return power; }
	uint32 get_running_cost() const { return running_cost; }
	sint32 get_fixed_cost() const { return get_maintenance(); }
	sint8 get_sound() const { return sound; }

	/**
	* 64 = 1.00
	* @return gear value
	*/
	uint16 get_gear() const { return gear; }

	/**
	* @return engine type
	* eletric engines require an electrified way to run
	*/
	uint8 get_engine_type() const { return engine_type; }

	/**
	 * @return the vehicles length in 1/8 of the normal len
	*/
	uint8 get_length() const { return len; }
	uint32 get_length_in_steps() const { return get_length() * VEHICLE_STEPS_PER_CARUNIT; }

	void calc_checksum(checksum_t *chk) const;
};

#endif
