/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
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
	 * can be used to select track system type (rail, monorail, maglev)
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
	const long gib_preis() const { return price; }

	const long gib_wartung() const { return maintenance; }

	/**
	 * Determines max speed in km/h allowed on this way
	 * @author Hj. Malthaner
	 */
	const uint32 gib_topspeed() const { return topspeed; }

	/**
	 * get way type
	 * @see waytype_t
	 * @author Hj. Malthaner
	 */
	const uint8 gib_wtyp() const { return wtyp; }

	/**
	* returns the system type of this way (mostly used with rails)
	* @see weg_t::styp
	* @author DarioK
	*/
	const uint8 gib_styp() const { return styp; }

	image_id gib_bild_nr(ribi_t::ribi ribi, uint8 season) const
	{
		if(season && number_seasons == 1) {
			return static_cast<const bildliste_besch_t *>(gib_kind(6))->gib_bild_nr(ribi);
		}
		return static_cast<const bildliste_besch_t *>(gib_kind(2))->gib_bild_nr(ribi);
	}

	image_id gib_hang_bild_nr(hang_t::typ hang, uint8 season) const
	{
#ifndef DOUBLE_GROUNDS
	if(!hang_t::ist_einfach(hang)) {
		return IMG_LEER;
	}
	if(season && number_seasons == 1) {
		return static_cast<const bildliste_besch_t *>(gib_kind(7))->gib_bild_nr(hang / 3 - 1);
	}
	return static_cast<const bildliste_besch_t *>(gib_kind(3))->gib_bild_nr(hang / 3 - 1);
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
			return static_cast<const bildliste_besch_t *>(gib_kind(7))->gib_bild_nr(nr);
		}
		return static_cast<const bildliste_besch_t *>(gib_kind(3))->gib_bild_nr(nr);
#endif
	}

	image_id gib_diagonal_bild_nr(ribi_t::ribi ribi, uint8 season) const
	{
		if(!ribi_t::ist_kurve(ribi)) {
			return IMG_LEER;
		}
		if(season && number_seasons == 1) {
			return static_cast<const bildliste_besch_t *>(gib_kind(8))->gib_bild_nr(ribi / 3 - 1);
		}
		return static_cast<const bildliste_besch_t *>(gib_kind(4))->gib_bild_nr(ribi / 3 - 1);
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

	/* true, if this tile is to be drawn as lie a normal thing */
	bool is_draw_as_ding() const { return draw_as_ding; }

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	const skin_besch_t * gib_cursor() const
	{
		return (const skin_besch_t *)(gib_kind(5));
	}
};

#endif
