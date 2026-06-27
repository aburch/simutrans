/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WAY_DESC_H
#define DESCRIPTOR_WAY_DESC_H


#include "image_list.h"
#include "obj_base_desc.h"
#include "skin_desc.h"
#include "../dataobj/ribi.h"

class checksum_t;
class tool_t;

/**
 * Way type description. Contains all needed values to describe a
 * way type in Simutrans.
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Images for flat ways (indexed by ribi)
 *  3   Images for slopes
 *  4   Images for straight diagonal ways
 *  5   Skin (cursor and icon)
 * if number_of_seasons == 0  (no winter images)
 *  6-8  front images of image lists 2-4
 * else
 *  6-8  winter images of image lists 2-4
 *  9-11 front images of image lists 2-4
 *  12-14 front winter images of image lists 2-4
 */
class way_desc_t : public obj_desc_transport_infrastructure_t {
	friend class way_reader_t;

private:

	/**
	 * Max weight
	 */
	uint32 max_weight;

	/**
	 * Way system type: i.e. for wtyp == track this
	 * can be used to select track system type (tramlike=7, elevated=1, ignore=255)
	 */
	uint8 styp;

	/* true, if a tile with this way should be always drawn as a thing
	*/
	uint8 draw_as_obj;

	/* number of seasons (0 = none, 1 = no snow/snow
	*/
	sint8 number_of_seasons;

	/// if true front_images lists exists as nodes
	bool front_images : 1;

	bool clip_below : 1; // only relevant for elevated ways

	// cached to not travese the list for each of those
	bool diagonals : 1;
	bool close_diagonals : 1;
	bool double_slopes : 1;
	bool switches : 1;

	/**
	 * calculates index of image list for flat ways
	 * for winter and/or front images
	 * add +1 and +2 to get slope and straight diagonal images, respectively
	 */
	uint16 image_list_base_index(bool snow, bool front) const
	{
		if (number_of_seasons == 0  ||  !snow) {
			if (front  &&  front_images) {
				return (number_of_seasons == 0) ? 6 : 9;
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
	*/
	systemtype_t get_styp() const { return (systemtype_t)styp; }

	bool is_tram() const { return wtyp == track_wt  &&  styp == type_tram; }

	bool is_clip_below() const { return clip_below; }

	image_id get_image_id(ribi_t::ribi ribi, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_EMPTY;
		}
		const uint16 n = image_list_base_index(season, front);
		return get_child<image_list_t>(n)->get_image_id(ribi);
	}

	image_id get_switch_image_id(ribi_t::ribi ribi, uint8 season, bool nw, bool front = false) const
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

	image_id get_slope_image_id(slope_t::type slope, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_EMPTY;
		}
		const uint16 n = image_list_base_index(season, front) + 1;
		uint16 nr;
		switch(slope) {
			case slope_t::north:
				nr = 0;
				break;
			case slope_t::west:
				nr = 1;
				break;
			case slope_t::east:
				nr = 2;
				break;
			case slope_t::south:
				nr = 3;
				break;
			case slope_t::north*2:
				nr = 4;
				break;
			case slope_t::west*2:
				nr = 5;
				break;
			case slope_t::east*2:
				nr = 6;
				break;
			case slope_t::south*2:
				nr = 7;
				break;
			default:
				return IMG_EMPTY;
		}
		if (!double_slopes) {
			// hack for old ways without double height images to use single slope images for both
			nr &= 3;
		}
		return get_child<image_list_t>(n)->get_image_id(nr);
	}

	image_id get_diagonal_image_id(ribi_t::ribi ribi, uint8 season, bool front = false) const
	{
		if (front  &&  !front_images) {
			return IMG_EMPTY;
		}
		assert(ribi_t::is_bend(ribi));
		const uint16 n = image_list_base_index(season, front) + 2;
		return get_child<image_list_t>(n)->get_image_id(ribi / 3 - 1);
	}

	image_id get_close_diagonal_image_id(uint8 nr, uint8 season, bool front = false) const
	{
		if (front && !front_images) {
			return IMG_EMPTY;
		}
		const uint16 n = image_list_base_index(season, front) + 2;
		return get_child<image_list_t>(n)->get_image_id(4+nr);
	}

	// quick query functions
	bool has_double_slopes() const { return double_slopes; 	}
	bool has_diagonal_image() const { return diagonals; }
	bool has_close_diagonal_image() const { return close_diagonals; }
	bool has_switch_image() const {	return switches; }

	/* true, if this tile is to be drawn as a normal thing */
	bool is_draw_as_obj() const { return draw_as_obj; }

	/**
	* Skin: cursor (index 0) and icon (index 1)
	*/
	const skin_desc_t * get_cursor() const
	{
		return get_child<skin_desc_t>(5);
	}

	void calc_checksum(checksum_t *chk) const;
};

#endif
