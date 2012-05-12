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
#include "../utils/checksum.h"


class werkzeug_t;

/**
 * Way type description. Contains all needed values to describe a
 * way type in Simutrans.
 *
 * Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Images for flat ways (indexed by ribi)
 *	3   Images for slopes
 *	4   Images for straight diagonal ways
 *	5   Hajo: Skin (cursor and icon)
 * if number_seasons == 0  (no winter images)
 *	6-8  front images of image lists 2-4
 * else
 *	6-8  winter images of image lists 2-4
 *	9-11 front images of image lists 2-4
 *	12-14 front winter images of image lists 2-4
 *
 * @author  Volker Meyer, Hj. Malthaner
 */
class weg_besch_t : public obj_besch_std_name_t {
	friend class way_reader_t;

public:
	enum { elevated=1, joined=7 /* only tram */, special=255 };

private:
	/**
	 * Price per square
	 * @author Hj. Malthaner
	 */
	uint32 price;

	/**
	 * Maintenance cost per square/month
	 * @author Hj. Malthaner
	 */
	uint32 maintenance;

	/**
	 * Max speed
	 * @author Hj. Malthaner
	 */
	sint32 topspeed;

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

	/* number of seasons (0 = none, 1 = no snow/snow
	*/
	sint8 number_seasons;

	/// if true front_images lists exists as nodes
	bool front_images;

	// this is the default tool for building this way ...
	werkzeug_t *builder;

	/**
	 * calculates index of image list for flat ways
	 * for winter and/or front images
	 * add +1 and +2 to get slope and straight diagonal images, respectively
	 */
	int image_list_base_index(bool snow, bool front) const
	{
		if (number_seasons == 0  ||  !snow) {
			if (front  &&  front_images) {
				return (number_seasons == 0) ? 6 : 9;
			}
			else {
				return 2;
			}
		}
		else { // winter images
			if (front  &&  front_images) {
				return 12;
			}
			else {
				return 6;
			}
		}
	}
public:
	long get_preis() const { return price; }

	long get_wartung() const { return maintenance; }

	/**
	 * Determines max speed in km/h allowed on this way
	 * @author Hj. Malthaner
	 */
	sint32 get_topspeed() const { return topspeed; }

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

	image_id get_bild_nr(ribi_t::ribi ribi, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_LEER;
		}
		int const n = image_list_base_index(season, front);
		return get_child<bildliste_besch_t>(n)->get_bild_nr(ribi);
	}

	image_id get_bild_nr_switch(ribi_t::ribi ribi, uint8 season, bool nw, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_LEER;
		}
		int const n = image_list_base_index(season, front);
		bildliste_besch_t const* const bl = get_child<bildliste_besch_t>(n);
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

	image_id get_hang_bild_nr(hang_t::typ hang, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_LEER;
		}
		int const n = image_list_base_index(season, front) + 1;
#ifndef DOUBLE_GROUNDS
		if(!hang_t::ist_einfach(hang)) {
			return IMG_LEER;
		}
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
		return get_child<bildliste_besch_t>(n)->get_bild_nr(nr);
#endif
	}

	image_id get_diagonal_bild_nr(ribi_t::ribi ribi, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_LEER;
		}
		int const n = image_list_base_index(season, front) + 2;
		return get_child<bildliste_besch_t>(n)->get_bild_nr(ribi / 3 - 1);
	}

	bool has_diagonal_bild() const {
		return get_child<bildliste_besch_t>(4)->get_bild_nr(0) != IMG_LEER
		||     get_child<bildliste_besch_t>(image_list_base_index(false, true)+2)->get_bild_nr(0) != IMG_LEER;
	}

	bool has_switch_bild() const {
		return get_child<bildliste_besch_t>(2)->get_anzahl() > 16
		||     get_child<bildliste_besch_t>(image_list_base_index(false, true))->get_anzahl() > 16;
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

	/* true, if this tile is to be drawn as a normal thing */
	bool is_draw_as_ding() const { return draw_as_ding; }

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	const skin_besch_t * get_cursor() const
	{
		return get_child<skin_besch_t>(5);
	}

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
	}
};

#endif
