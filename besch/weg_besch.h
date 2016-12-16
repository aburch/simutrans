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

class checksum_t;
class tool_t;

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
class weg_besch_t : public obj_besch_transport_infrastructure_t {
	friend class way_reader_t;

private:

	/**
	 * Max weight
	 * @author Hj. Malthaner
	 */
	uint32 max_weight;

	/**
	 * Way system type: i.e. for wtyp == track this
	 * can be used to select track system type (tramlike=7, elevated=1, ignore=255)
	 * @author Hj. Malthaner
	 */
	uint8 styp;

	/* true, if a tile with this way should be always drawn as a thing
	*/
	uint8 draw_as_obj;

	/* number of seasons (0 = none, 1 = no snow/snow
	*/
	sint8 number_seasons;

	/// if true front_images lists exists as nodes
	bool front_images;

	/**
	 * calculates index of image list for flat ways
	 * for winter and/or front images
	 * add +1 and +2 to get slope and straight diagonal images, respectively
	 */
	uint16 image_list_base_index(bool snow, bool front) const
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

	/**
	* @return waytype used in finance stats (needed to distinguish \
	* between train track and tram track
	*/
	waytype_t get_finance_waytype() const;

	/**
	* returns the system type of this way (mostly used with rails)
	* @see systemtype_t
	* @author DarioK
	*/
	uint8 get_styp() const { return styp; }

	bool is_tram() const { return wt == track_wt  &&  styp == type_tram; }

	image_id get_image_id(ribi_t::ribi ribi, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_EMPTY;
		}
		const uint16 n = image_list_base_index(season, front);
		return get_child<image_list_t>(n)->get_image_id(ribi);
	}

	image_id get_image_id_switch(ribi_t::ribi ribi, uint8 season, bool nw, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_EMPTY;
		}
		const uint16 n = image_list_base_index(season, front);
		image_list_t const* const imglist = get_child<image_list_t>(n);
		// only do this if extended switches are there
		if(  imglist->get_count()>16  ) {
			static uint8 ribi_to_extra[16] = {
				255, 255, 255, 255, 255, 255, 255, 0,
				255, 255, 255, 1, 255, 2, 3, 4
			};
			return imglist->get_image_id( ribi_to_extra[ribi]+16+(nw*5) );
		}
		// else return standard values
		return imglist->get_image_id( ribi );
	}

	image_id get_hang_imageid(slope_t::type hang, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_EMPTY;
		}
		const uint16 n = image_list_base_index(season, front) + 1;
		uint16 nr;
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
				return IMG_EMPTY;
		}
		image_id hang_img = get_child<image_list_t>(n)->get_image_id(nr);
		if(  nr > 3  &&  hang_img == IMG_EMPTY  &&  get_child<image_list_t>(n)->get_count()<=4  ) {
			// hack for old ways without double height images to use single slope images for both
			nr -= 4;
			hang_img = get_child<image_list_t>(n)->get_image_id(nr);
		}
		return hang_img;
	}

	image_id get_diagonal_imageid(ribi_t::ribi ribi, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_EMPTY;
		}
		const uint16 n = image_list_base_index(season, front) + 2;
		return get_child<image_list_t>(n)->get_image_id(ribi / 3 - 1);
	}

	bool has_double_slopes() const {
		return get_child<image_list_t>(3)->get_count() > 4
		||     get_child<image_list_t>(image_list_base_index(false, true) + 1)->get_count() > 4;
	}

	bool has_diagonal_image() const {
		return get_child<image_list_t>(4)->get_image_id(0) != IMG_EMPTY
		||     get_child<image_list_t>(image_list_base_index(false, true)+2)->get_image_id(0) != IMG_EMPTY;
	}

	bool has_switch_image() const {
		return get_child<image_list_t>(2)->get_count() > 16
		||     get_child<image_list_t>(image_list_base_index(false, true))->get_count() > 16;
	}

	/* true, if this tile is to be drawn as a normal thing */
	bool is_draw_as_obj() const { return draw_as_obj; }

	/**
	* Skin: cursor (index 0) and icon (index 1)
	* @author Hj. Malthaner
	*/
	const skin_besch_t * get_cursor() const
	{
		return get_child<skin_besch_t>(5);
	}

	void calc_checksum(checksum_t *chk) const;
};

#endif
