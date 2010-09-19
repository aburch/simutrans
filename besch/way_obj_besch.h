/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  Modulbeschreibung:
 *      This files describes way objects like a electrifications
 */
#ifndef __WAY_OBJ_BESCH_H
#define __WAY_OBJ_BESCH_H

#include "bildliste_besch.h"
#include "obj_besch_std_name.h"
#include "skin_besch.h"
#include "../dataobj/ribi.h"
#include "../dataobj/way_constraints.h"
#include "../utils/checksum.h"

class werkzeug_t;
class checksum_t;

/**
 * Way type description. Contains all needed values to describe a
 * way type in Simutrans.
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Flache Bilder mit ribis
 *	3   Hangbilder
 *	4   Flache Bilder Diagonalstrecken
 *      5   Hajo: Skin (cursor and icon)
 *
 * @author  Volker Meyer, Hj. Malthaner
 */
class way_obj_besch_t : public obj_besch_std_name_t {
    friend class way_obj_writer_t;
    friend class way_obj_reader_t;

private:
    /**
     * Price per square
     * @author Hj. Malthaner
     */
    uint32 price;

	uint32 scaled_price;

    /**
     * Maintenance cost per square/month
     * @author Hj. Malthaner
     */
    uint32 maintenance;

	uint32 scaled_maintenance;

    /**
     * Max speed
     * @author Hj. Malthaner
     */
    uint32 topspeed;

    /**
     * Introduction date
     * @author Hj. Malthaner
     */
    uint16 intro_date;
    uint16 obsolete_date;

    /**
     * Way type: i.e. road or track
     * @see waytype_t
     * @author Hj. Malthaner
     */
    uint8 wtyp;

    /**
     * set to powerline of overheadwire or ignore
     * @see waytype_t
     * @author Hj. Malthaner
     */
	uint8 own_wtyp;

	/*Way constraints for, e.g., loading gauges, types of electrification, etc.
	* @author: jamespetts*/
	//uint8 way_constraints_permissive;
	//uint8 way_constraints_prohibitive;
	way_constraints_of_way_t way_constraints;

	werkzeug_t *builder;


public:
	sint32 get_preis() const { return scaled_price; }

	sint32 get_base_price() const { return price; }

	sint32 get_wartung() const { return scaled_maintenance; }

	sint32 get_base_maintenance() const { return maintenance; }

	void set_scale(float scale_factor)
	{
		// BG: 29.08.2009: explicit typecasts avoid warnings
		scaled_price = (uint32)(price * scale_factor > 0 ? price * scale_factor : 1);
		scaled_maintenance = (uint32)(maintenance * scale_factor > 0 ? maintenance * scale_factor : 1);
	}

	/**
	 * Determines max speed in km/h allowed on this way
	 * @author Hj. Malthaner
	 */
	uint32 get_topspeed() const { return topspeed; }

	/**
	 * get way type
	 * @see waytype_t
	 * @author Hj. Malthaner
	 */
	waytype_t get_wtyp() const { return (waytype_t)wtyp; }

	/**
	* returns the system type of this way (mostly used with rails)
	* @see weg_t::styp
	* @author DarioK
	*/
	waytype_t get_own_wtyp() const { return (waytype_t)own_wtyp; }

	// way objects can have a front and a backimage, unlike ways ...
	image_id get_front_image_id(ribi_t::ribi ribi) const { return get_child<bildliste_besch_t>(2)->get_bild_nr(ribi); }

	image_id get_back_image_id(ribi_t::ribi ribi) const { return get_child<bildliste_besch_t>(3)->get_bild_nr(ribi); }

	image_id get_front_slope_image_id(hang_t::typ hang) const
	{
#ifndef DOUBLE_GROUNDS
		if(!hang_t::ist_einfach(hang)) {
			return IMG_LEER;
		}
		return get_child<bildliste_besch_t>(4)->get_bild_nr(hang / 3 - 1);
#else
		int nr;
		switch(hang) {
			case 4:
				nr = 0;
				break;
			case 12:
				nr = 1;
				break;
			case 28:
				nr = 2;
				break;
			case 36:
				nr = 3;
				break;
			default:
				return IMG_LEER;
		}
		return get_child<bildliste_besch_t>(4)->get_bild_nr(nr);
#endif
	  }

	image_id get_back_slope_image_id(hang_t::typ hang) const
	{
#ifndef DOUBLE_GROUNDS
		if(!hang_t::ist_einfach(hang)) {
			return IMG_LEER;
		}
		return get_child<bildliste_besch_t>(5)->get_bild_nr(hang / 3 - 1);
#else
		int nr;
		switch(hang) {
			case 4:
				nr = 0;
				break;
			case 12:
				nr = 1;
				break;
			case 28:
				nr = 2;
				break;
			case 36:
				nr = 3;
				break;
			default:
				return IMG_LEER;
		}
		return get_child<bildliste_besch_t>(5)->get_bild_nr(nr);
#endif
	  }

	image_id get_front_diagonal_image_id(ribi_t::ribi ribi) const
	{
		if(!ribi_t::ist_kurve(ribi)) {
			return IMG_LEER;
		}
		return get_child<bildliste_besch_t>(6)->get_bild_nr(ribi / 3 - 1);
	}

	image_id get_back_diagonal_image_id(ribi_t::ribi ribi) const
	{
		if(!ribi_t::ist_kurve(ribi)) {
			return IMG_LEER;
		}
		return get_child<bildliste_besch_t>(7)->get_bild_nr(ribi / 3 - 1);
	}

	bool has_diagonal_bild() const {
		if (get_child<bildliste_besch_t>(4)->get_bild(0)) {
			// has diagonal fontimage
			return true;
		}
		if (get_child<bildliste_besch_t>(5)->get_bild(0)) {
			// or diagonal back image
			return true;
		}
		return false;
	}

	/**
	* @return introduction year
	* @author Hj. Malthaner
	*/
	uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return introduction month
	* @author Hj. Malthaner
	*/
	uint16 get_retire_year_month() const { return obsolete_date; }

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	skin_besch_t const* get_cursor() const { return get_child<skin_besch_t>(8); }

	/* Way constraints: determines whether vehicles
	 * can travel on this way. This method decodes
	 * the byte into bool values. See here for
	 * information on bitwise operations: 
	 * http://www.cprogramming.com/tutorial/bitwise_operators.html
	 * @author: jamespetts
	 * */
	//const bool permissive_way_constraint_set(uint8 i)
	//{
	//	return ((way_constraints_permissive & 1)<<i != 0);
	//}

	//const bool prohibitive_way_constraint_set(uint8 i)
	//{
	//	return ((way_constraints_prohibitive & 1)<<i != 0);
	//}

	//uint8 get_way_constraints_permissive() const { return way_constraints_permissive; }
	//uint8 get_way_constraints_prohibitive() const { return way_constraints_prohibitive; }
	const way_constraints_of_way_t& get_way_constraints() const { return way_constraints; }
	void set_way_constraints(const way_constraints_of_way_t& value) { way_constraints = value; }

	// default tool for building
	werkzeug_t *get_builder() const {
		return builder;
	}
	void set_builder( werkzeug_t *w )  {
		builder = w;
	}

	void calc_checksum(checksum_t *chk) const
	{
		chk->input(price);
		chk->input(maintenance);
		chk->input(topspeed);
		chk->input(intro_date);
		chk->input(obsolete_date);
		chk->input(wtyp);
		chk->input(own_wtyp);

		//Experimental values
		chk->input(way_constraints.get_permissive());
		chk->input(way_constraints.get_prohibitive());
	}
};

#endif
