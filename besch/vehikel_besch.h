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
#include "../simtypes.h"
#include "../simunits.h"


class checksum_t;

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
class vehikel_besch_t : public obj_besch_transport_related_t {
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
	uint16  zuladung;
	uint16  loading_time;	// time per full loading/unloading
	uint32  gewicht;
	uint32  leistung;
	uint16  running_cost;
	uint16  fixed_cost;

	uint16  gear;       // engine gear (power multiplier), 64=100

	uint8 len;			// length (=8 is half a tile, the old default)
	sint8 sound;

	uint8  vorgaenger;	// all defined leading vehicles
	uint8  nachfolger;	// all defined trailer

	uint8  engine_type; // diesel, steam, electric (requires electrified ways), fuel_cell, etc.

	sint8 freight_image_type;	// number of freight images (displayed for different goods)


public:
	// since we have a second constructor
	vehikel_besch_t() { }

	// default vehicle (used for way seach and similar tasks)
	// since it has no images and not even a name knot any calls to this will case a crash
	vehikel_besch_t(uint8 wtyp, uint16 speed, engine_t engine) {
		freight_image_type = cost = zuladung = axle_load = running_cost = fixed_cost = intro_date = vorgaenger = nachfolger = 0;
		leistung = gewicht = 1;
		loading_time = 1000;
		gear = 64;
		len = 8;
		sound = -1;
		wt = wtyp;
		engine_type = (uint8)engine;
		topspeed = speed;
	}

	ware_besch_t const* get_ware() const { return get_child<ware_besch_t>(2); }

	skin_besch_t const* get_rauch() const { return get_child<skin_besch_t>(3); }

	image_id get_basis_bild() const { return get_bild_nr(ribi_t::dir_sued, get_ware() ); }

	// returns the number of different directions
	uint8 get_dirs() const { return get_child<bildliste_besch_t>(4)->get_bild(4) ? 8 : 4; }

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
				if (ware == get_child<ware_besch_t>(6 + nachfolger + vorgaenger + i)) {
					ware_index = i;
					break;
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			bildliste2d_besch_t const* const liste2d = get_child<bildliste2d_besch_t>(5);
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
			liste = get_child<bildliste_besch_t>(5);
		}

		if(!liste) {
			liste = get_child<bildliste_besch_t>(4);
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
	const vehikel_besch_t *get_vorgaenger(int i) const
	{
		if(i < 0 || i >= vorgaenger) {
			return 0;
		}
		return get_child<vehikel_besch_t>(6 + i);
	}

	int get_vorgaenger_count() const { return vorgaenger; }

	// Liefert die erlaubten Nachfolger.
	// liefert get_nachfolger(0) == NULL, so bedeutet das entweder alle
	// Nachfolger sind erlaubt oder keine. Um das zu unterscheiden, sollte
	// man vorher hat_nachfolger() befragen
	const vehikel_besch_t *get_nachfolger(int i) const
	{
		if(i < 0 || i >= nachfolger) {
			return 0;
		}
		return get_child<vehikel_besch_t>(6 + vorgaenger + i);
	}

	int get_nachfolger_count() const { return nachfolger; }

	/* returns true, if this veh can be before the next_veh
	 * uses NULL to indicate end of convoi
	 */
	bool can_lead(const vehikel_besch_t *next_veh) const
	{
		if(  nachfolger==0  ) {
			return true;
		}
		for( int i=0;  i<nachfolger;  i++  ) {
			vehikel_besch_t const* const veh = get_child<vehikel_besch_t>(6 + vorgaenger + i);
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
		for( int i=0;  i<vorgaenger;  i++  ) {
			vehikel_besch_t const* const veh = get_child<vehikel_besch_t>(6 + i);
			if(veh==prev_veh) {
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	bool can_follow_any() const { return nachfolger==0; }

	uint16 get_zuladung() const { return zuladung; }
	uint16 get_loading_time() const { return loading_time; } // ms per full loading/unloading
	uint32 get_gewicht() const { return gewicht; }
	uint32 get_leistung() const { return leistung; }
	uint16 get_betriebskosten() const { return running_cost; }
	uint16 get_maintenance() const { return fixed_cost; }
	sint8 get_sound() const { return sound; }

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

	void calc_checksum(checksum_t *chk) const;
};

#endif
