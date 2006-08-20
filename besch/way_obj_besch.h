/*
 *
 *  way_obj_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      This files describes way objects like a powerline
 *
 */
#ifndef __WAY_OBJ_BESCH_H
#define __WAY_OBJ_BESCH_H

/*
 *  includes
 */
#include "bildliste_besch.h"
#include "text_besch.h"
#include "../dataobj/ribi.h"

#include "intro_dates.h"


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
class way_obj_besch_t  : public obj_besch_t {
    friend class way_obj_writer_t;
    friend class way_obj_reader_t;

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
     * Introduction date
     * @author Hj. Malthaner
     */
    uint16 intro_date;
    uint16 obsolete_date;

    /**
     * Way type: i.e. road or track
     * @see weg_t::typ
     * @author Hj. Malthaner
     */
    uint8 wtyp;

    /**
     * set to powerline of overheadwire or ignore
     * @see weg_t::typ
     * @author Hj. Malthaner
     */
	uint8 own_wtyp;

public:
	const char * gib_name() const { return static_cast<const text_besch_t *>(gib_kind(0))->gib_text(); }

	const char * gib_copyright() const { return static_cast<const text_besch_t *>(gib_kind(1))->gib_text(); }

	const long gib_preis() const { return price; }

	const long gib_wartung() const { return maintenance; }

    /**
     * Determines max speed in km/h allowed on this way
     * @author Hj. Malthaner
     */
    const uint32 gib_topspeed() const { return topspeed; }

    /**
     * get way type
     * @see weg_t::typ
     * @author Hj. Malthaner
     */
	const uint8 gib_wtyp() const { return wtyp; }

	/**
	* returns the system type of this way (mostly used with rails)
	* @see weg_t::styp
	* @author DarioK
	*/
	const uint8 gib_own_wtyp() const { return own_wtyp; }


	// way objects can have a front and a backimage, unlike ways ...
	image_id get_front_image_id(ribi_t::ribi ribi) const { return static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild_nr(ribi); }

	image_id get_back_image_id(ribi_t::ribi ribi) const { return static_cast<const bildliste_besch_t *>(gib_kind(3))->gib_bild_nr(ribi); }

	image_id get_front_slope_image_id(hang_t::typ hang) const
	{
#ifndef DOUBLE_GROUNDS
		if(!hang_t::ist_einfach(hang)) {
			return IMG_LEER;
		}
		return static_cast<const bildliste_besch_t *>(gib_kind(4))->gib_bild_nr(hang / 3 - 1);
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
		return static_cast<const bildliste_besch_t *>(gib_kind(4))->gib_bild_nr(nr);
#endif
	  }

	image_id get_back_slope_image_id(hang_t::typ hang) const
	{
#ifndef DOUBLE_GROUNDS
		if(!hang_t::ist_einfach(hang)) {
			return IMG_LEER;
		}
		return static_cast<const bildliste_besch_t *>(gib_kind(5))->gib_bild_nr(hang / 3 - 1);
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
		return static_cast<const bildliste_besch_t *>(gib_kind(5))->gib_bild_nr(nr);
#endif
	  }

	image_id get_front_diagonal_image_id(ribi_t::ribi ribi) const
	{
		if(!ribi_t::ist_kurve(ribi)) {
			return IMG_LEER;
		}
		return static_cast<const bildliste_besch_t *>(gib_kind(6))->gib_bild_nr(ribi / 3 - 1);
	}

	image_id get_back_diagonal_image_id(ribi_t::ribi ribi) const
	{
		if(!ribi_t::ist_kurve(ribi)) {
			return IMG_LEER;
		}
		return static_cast<const bildliste_besch_t *>(gib_kind(7))->gib_bild_nr(ribi / 3 - 1);
	}

	/**
	* @return introduction year
	* @author Hj. Malthaner
	*/
	const uint16 get_intro_year_month() const { return intro_date; }

	/**
	* @return introduction month
	* @author Hj. Malthaner
	*/
	const uint16 get_retire_year_month() const { return obsolete_date; }

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	const skin_besch_t * gib_cursor() const { return (const skin_besch_t *)(gib_kind(8)); }

};

#endif // __WEG_BESCH_H
