/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __WEG_BESCH_H
#define __WEG_BESCH_H

#include "bildliste_besch.h"
#include "obj_besch_std_name.h"
#include "skin_besch.h"
#include "../dataobj/ribi.h"
#include "../dataobj/way_constraints.h"
#include "../utils/checksum.h"

class werkzeug_t;

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
class weg_besch_t : public obj_besch_std_name_t {
	friend class way_writer_t;
	friend class way_reader_t;

public:
	enum { elevated=1, joined=7 /* only tram */, special=255 };

private:
	/**
	 * Price per square
	 * @author Hj. Malthaner
	 */
	sint32 price;
	sint32 scaled_price;

	/**
	 * Maintenance cost per square/month
	 * @author Hj. Malthaner
	 */
	sint32 maintenance;
	sint32 scaled_maintenance;

	/**
	 * Max speed
	 * @author Hj. Malthaner
	 */
	uint32 topspeed;

	/**
	 * Max weight
	 * @author Hj. Malthaner
	 */
	uint32 max_weight;

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
	 * Way system type: i.e. for wtyp == track this
	 * can be used to select track system type (tramlike=7, elevated=1, ignore=255)
	 * @author Hj. Malthaner
	 */
	uint8 styp;

	/* true, if a tile with this way should be always drawn as a thing
	*/
	uint8 draw_as_ding;

	/* number of seasons (0 = none, 1 = no snow/snow)
	*/
	sint8 number_seasons;

	/*Way constraints for, e.g., loading gauges, types of electrification, etc.
	* @author: jamespetts*/
	//uint8 way_constraints_permissive;
	//uint8 way_constraints_prohibitive;
	way_constraints_of_way_t way_constraints;
	// this is the defualt tools for building this way ...
	werkzeug_t *builder;

public:
	sint32 get_preis() const { return scaled_price; }

	sint32 get_base_price() const { return price; }

	sint32 get_wartung() const { return scaled_maintenance; }

	sint32 get_base_maintenance() const { return  maintenance; }

	void set_scale(float scale_factor) 
	{
		// BG: 29.08.2009: explicit typecasts avoid warnings
		scaled_price = (sint32)(price * scale_factor < 1 ? (price > 0 ? 1 : 0) : price * scale_factor);
		scaled_maintenance = (sint32)(maintenance * scale_factor < (maintenance > 0 ? 1 : 0) ? 1: maintenance * scale_factor);
	}

	/**
	 * Determines max speed in km/h allowed on this way
	 * @author Hj. Malthaner
	 */
	uint32 get_topspeed() const { return topspeed; }

	//Returns maximum weight
	uint32 get_max_weight() const { return (max_weight < 9999 && max_weight > 0) ? max_weight : 999; }

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
	uint8 get_styp() const { return styp; }

	image_id get_bild_nr(ribi_t::ribi ribi, uint8 season) const
	{
		int const n = season && number_seasons == 1 ? 6 : 2;
		return get_child<bildliste_besch_t>(n)->get_bild_nr(ribi);
	}

	image_id get_bild_nr_switch(ribi_t::ribi ribi, uint8 season, bool nw) const
	{
		uint8 listen_nr = (season && number_seasons == 1) ? 6 : 2;
		bildliste_besch_t const* const bl = get_child<bildliste_besch_t>(listen_nr);
		// only do this if extended switches are there
		if(  bl->get_anzahl()>16  ) {
			static uint8 ribi_to_extra[16] = {
				255, 255, 255, 255, 255, 255, 255, 0,
				255, 255, 255, 1, 255, 2, 3, 4
			};
			return bl->get_bild_nr( ribi_to_extra[ribi]+16+(nw*5) );
		}
		// else return standard values
		return bl->get_bild_nr( ribi );
	}

	image_id get_hang_bild_nr(hang_t::typ hang, uint8 season) const
	{
#ifndef DOUBLE_GROUNDS
		if(!hang_t::ist_einfach(hang)) {
			return IMG_LEER;
		}
		int const n = season && number_seasons == 1 ? 7 : 3;
		return get_child<bildliste_besch_t>(n)->get_bild_nr(hang / 3 - 1);
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
		int const n = season && number_seasons == 1 ? 7 : 3;
		return get_child<bildliste_besch_t>(n)->get_bild_nr(nr);
#endif
	}

	image_id get_diagonal_bild_nr(ribi_t::ribi ribi, uint8 season) const
	{
		int const n = season && number_seasons == 1 ? 8 : 4;
		return get_child<bildliste_besch_t>(n)->get_bild_nr(ribi / 3 - 1);
	}

	bool has_diagonal_bild() const {
		return get_child<bildliste_besch_t>(4)->get_bild_nr(0) != IMG_LEER;
	}

	bool has_switch_bild() const {
		return get_child<bildliste_besch_t>(2)->get_anzahl() > 16;
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

	/* true, if this tile is to be drawn as lie a normal thing */
	bool is_draw_as_ding() const { return draw_as_ding; }

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	const skin_besch_t * get_cursor() const
	{
		return get_child<skin_besch_t>(5);
	}

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
		chk->input(max_weight);
		chk->input(intro_date);
		chk->input(obsolete_date);
		chk->input(wtyp);
		chk->input(styp);

		//Experimental values
		chk->input(way_constraints.get_permissive());
		chk->input(way_constraints.get_prohibitive());
	}
};

#endif
