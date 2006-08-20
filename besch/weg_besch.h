/*
 *
 *  weg_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __WEG_BESCH_H
#define __WEG_BESCH_H

/*
 *  includes
 */
#include "bildliste_besch.h"
#include "text_besch.h"
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
class weg_besch_t  : public obj_besch_t {
    friend class way_writer_t;
    friend class way_reader_t;

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
    uint32 intro_date;


    /**
     * Way type: i.e. road or track
     * @see weg_t::typ
     * @author Hj. Malthaner
     */
    uint8 wtyp;


    /**
     * Way system type: i.e. for wtyp == track this
     * can be used to select track system type (rail, monorail, maglev)
     * @author Hj. Malthaner
     */
    uint8 styp;

public:

    int gib_preis() const
    {
	return price;
    }


    int gib_wartung() const
    {
	return maintenance;
    }


    /**
     * Determines max speed in km/h allowed on this way
     * @author Hj. Malthaner
     */
    int  gib_topspeed() const
    {
	return topspeed;
    }


    /**
     * get way type
     * @see weg_t::typ
     * @author Hj. Malthaner
     */
    const uint8 gib_wtyp() const
    {
        return wtyp;
    }

		/**
		 * returns the system type of this way (mostly used with rails)
		 * @see weg_t::styp
		 * @author DarioK
		 */
		const uint8 gib_styp() const
		{
				return styp;
		}


    const char * gib_name() const
    {
        return static_cast<const text_besch_t *>(gib_kind(0))->gib_text();
    }

    const char * gib_copyright() const
    {
        return static_cast<const text_besch_t *>(gib_kind(1))->gib_text();
    }

    int gib_bild_nr(ribi_t::ribi ribi) const
    {
	return static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild_nr(ribi);
    }

    int gib_hang_bild_nr(hang_t::typ hang) const
    {
	if(!hang_t::ist_einfach(hang))
	    return -1;

	return static_cast<const bildliste_besch_t *>(gib_kind(3))->gib_bild_nr(hang / 3 - 1);
    }

    int gib_diagonal_bild_nr(ribi_t::ribi ribi) const
    {
	if(!ribi_t::ist_kurve(ribi))
	    return -1;

	return static_cast<const bildliste_besch_t *>(gib_kind(4))->gib_bild_nr(ribi / 3 - 1);
    }


    /**
     * @return introduction year
     * @author Hj. Malthaner
     */
    int get_intro_year() const {
      return intro_date >> 4;
    }


    /**
     * @return introduction month
     * @author Hj. Malthaner
     */
    int get_intro_month() const {
      return intro_date & 15;
    }


    /**
     * Skin: cursor (index 0) and icon (index 1)
     * @author Hj. Malthaner
     */
    const skin_besch_t * gib_cursor() const
    {
	return (const skin_besch_t *)(gib_kind(5));
    }
};

#endif // __WEG_BESCH_H
