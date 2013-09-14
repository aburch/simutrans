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
#include "../network/checksum.h"


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
class way_obj_besch_t : public obj_besch_transport_infrastructure_t {
    friend class way_obj_reader_t;

private:

    /**
     * set to powerline of overheadwire or ignore
     * @see waytype_t
     * @author Hj. Malthaner
     */
	uint8 own_wtyp;

public:

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
			case 8:
				nr = 4;
				break;
			case 24:
				nr = 5;
				break;
			case 56:
				nr = 6;
				break;
			case 72:
				nr = 7;
				break;
			default:
				return IMG_LEER;
		}
		image_id hang_img = get_child<bildliste_besch_t>(4)->get_bild_nr(nr);
		if(  nr > 3  &&  hang_img == IMG_LEER  ) {
			// hack for old ways without double height images to use single slope images for both
			nr -= 4;
			hang_img = get_child<bildliste_besch_t>(4)->get_bild_nr(nr);
		}
		return hang_img;
	  }

	image_id get_back_slope_image_id(hang_t::typ hang) const
	{
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
			case 8:
				nr = 4;
				break;
			case 24:
				nr = 5;
				break;
			case 56:
				nr = 6;
				break;
			case 72:
				nr = 7;
				break;
			default:
				return IMG_LEER;
		}
		image_id hang_img = get_child<bildliste_besch_t>(5)->get_bild_nr(nr);
		if(  nr > 3  &&  hang_img == IMG_LEER  ) {
			// hack for old ways without double height images to use single slope images for both
			nr -= 4;
			hang_img = get_child<bildliste_besch_t>(5)->get_bild_nr(nr);
		}
		return hang_img;
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
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	skin_besch_t const* get_cursor() const { return get_child<skin_besch_t>(8); }


	void calc_checksum(checksum_t *chk) const
	{
		obj_besch_transport_infrastructure_t::calc_checksum(chk);
		chk->input(own_wtyp);
	}
};

#endif
