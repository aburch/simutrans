/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __WEG_BESCH_H
#define __WEG_BESCH_H

#include "bildliste_besch.h"
#include "obj_besch_std_name.h"
#include "../dataobj/ribi.h"


class skin_besch_t;

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

	/* number of seasons (0 = none, 1 = no snow/snow
	*/
	sint8 number_seasons;

public:
	long get_preis() const { return price; }

	long get_wartung() const { return maintenance; }

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
	uint8 get_styp() const { return styp; }

	image_id get_bild_nr(ribi_t::ribi ribi, uint8 season) const
	{
		if(season && number_seasons == 1) {
			return static_cast<const bildliste_besch_t *>(get_child(6))->get_bild_nr(ribi);
		}
		return static_cast<const bildliste_besch_t *>(get_child(2))->get_bild_nr(ribi);
	}

	image_id get_bild_nr_switch(ribi_t::ribi ribi, uint8 season, bool nw) const
	{
		uint8 listen_nr = (season && number_seasons == 1) ? 6 : 2;
		const bildliste_besch_t *bl = static_cast<const bildliste_besch_t *>(get_child(listen_nr));
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
		if(season && number_seasons == 1) {
			return static_cast<const bildliste_besch_t *>(get_child(7))->get_bild_nr(hang / 3 - 1);
		}
		return static_cast<const bildliste_besch_t *>(get_child(3))->get_bild_nr(hang / 3 - 1);
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
		if(season && number_seasons == 1) {
			return static_cast<const bildliste_besch_t *>(get_child(7))->get_bild_nr(nr);
		}
		return static_cast<const bildliste_besch_t *>(get_child(3))->get_bild_nr(nr);
#endif
	}

	image_id get_diagonal_bild_nr(ribi_t::ribi ribi, uint8 season) const
	{
		if(season && number_seasons == 1) {
			return static_cast<const bildliste_besch_t *>(get_child(8))->get_bild_nr(ribi / 3 - 1);
		}
		return static_cast<const bildliste_besch_t *>(get_child(4))->get_bild_nr(ribi / 3 - 1);
	}

	bool has_diagonal_bild() const {
		return static_cast<const bildliste_besch_t *>(get_child(4))->get_bild_nr(0)!=IMG_LEER;
	}

	bool has_switch_bild() const {
		return static_cast<const bildliste_besch_t *>(get_child(2))->get_anzahl()>16;
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
		return (const skin_besch_t *)(get_child(5));
	}
};

#endif
