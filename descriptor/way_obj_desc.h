/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WAY_OBJ_DESC_H
#define DESCRIPTOR_WAY_OBJ_DESC_H


#include "image_list.h"
#include "obj_base_desc.h"
#include "skin_desc.h"
#include "../dataobj/ribi.h"
#include "../network/checksum.h"


class tool_t;
class checksum_t;

/**
 * Way objects type description (like overhead lines).
 *
 * Child nodes:
 *  0  Name
 *  1  Copyright
 *  2  Image on flat ways
 *  3  Image on sloped ways
 *  4  Image on diagonal ways
 *  5  Skin (cursor and icon)
 */
class way_obj_desc_t : public obj_desc_transport_infrastructure_t {
	friend class way_obj_reader_t;

private:

	/**
	 * Type of the object, only overheadlines_wt is currently used.
	 */
	uint8 own_wtyp;

public:

	bool is_overhead_line() const { return (waytype_t)own_wtyp == overheadlines_wt; }

	// way objects can have a front and a backimage, unlike ways ...
	image_id get_front_image_id(ribi_t::ribi ribi) const { return get_child<image_list_t>(2)->get_image_id(ribi); }

	image_id get_crossing_image_id(ribi_t::ribi ribi, bool nw, bool front = false) const
	{
		if(  front  &&  !get_child<image_list_t>(2)->get_count()  ) {
			return IMG_EMPTY;
		}
		image_list_t const* const imglist = get_child<image_list_t>(3-front);
		// only do this if extended switches are there
		if(  imglist->get_count()>16  ) {
			static uint8 ribi_to_extra[16] = {
				255, 255, 255, 255, 255, 255, 255, 0,
				255, 255, 255, 1, 255, 2, 3, 4
			};
			return ribi < 16 ? imglist->get_image_id( ribi_to_extra[ribi]+16+(nw*5) )  : IMG_EMPTY;
		}
		// else return standard values
		return imglist->get_image_id( ribi );
	}

	image_id get_back_image_id(ribi_t::ribi ribi) const { return get_child<image_list_t>(3)->get_image_id(ribi); }

	image_id get_front_slope_image_id(slope_t::type slope) const
	{
		uint16 nr;
		switch(slope) {
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
		image_id slope_img = get_child<image_list_t>(4)->get_image_id(nr);
		if(  nr > 3  &&  slope_img == IMG_EMPTY  ) {
			// hack for old ways without double height images to use single slope images for both
			nr -= 4;
			slope_img = get_child<image_list_t>(4)->get_image_id(nr);
		}
		return slope_img;
	}

	image_id get_back_slope_image_id(slope_t::type slope) const
	{
		uint16 nr;
		switch(slope) {
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
		image_id slope_img = get_child<image_list_t>(5)->get_image_id(nr);
		if(  nr > 3  &&  slope_img == IMG_EMPTY  ) {
			// hack for old ways without double height images to use single slope images for both
			nr -= 4;
			slope_img = get_child<image_list_t>(5)->get_image_id(nr);
		}
		return slope_img;
	}

	image_id get_front_diagonal_image_id(ribi_t::ribi ribi) const
	{
		if(!ribi_t::is_bend(ribi)) {
			return IMG_EMPTY;
		}
		return get_child<image_list_t>(6)->get_image_id(ribi / 3 - 1);
	}

	image_id get_back_diagonal_image_id(ribi_t::ribi ribi) const
	{
		if(!ribi_t::is_bend(ribi)) {
			return IMG_EMPTY;
		}
		return get_child<image_list_t>(7)->get_image_id(ribi / 3 - 1);
	}

	bool has_diagonal_image() const {
		if (get_child<image_list_t>(4)->get_image(0)) {
			// has diagonal fontimage
			return true;
		}
		if (get_child<image_list_t>(5)->get_image(0)) {
			// or diagonal back image
			return true;
		}
		return false;
	}

	/**
	* Skin: cursor (index 0) and icon (index 1)
	*/
	skin_desc_t const* get_cursor() const { return get_child<skin_desc_t>(8); }


	void calc_checksum(checksum_t *chk) const
	{
		obj_desc_transport_infrastructure_t::calc_checksum(chk);
		chk->input(own_wtyp);
	}
};

#endif
