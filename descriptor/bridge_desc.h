/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_BRIDGE_DESC_H
#define DESCRIPTOR_BRIDGE_DESC_H


#include "skin_desc.h"
#include "image_list.h"
#include "text_desc.h"
#include "../simtypes.h"
#include "../display/simimg.h"

#include "../dataobj/ribi.h"

class tool_t;
class checksum_t;


/*
 *  BEWARE: non-standard node structure!
 *  0   Foreground-images
 *  1   Background-images
 *  2   Cursor/Icon
 *  3   Foreground-images - snow
 *  4   Background-images - snow
 */
class bridge_desc_t : public obj_desc_transport_infrastructure_t {
	friend class bridge_reader_t;

private:
	uint8 pillars_every;     // =0 off
	bool pillars_asymmetric; // =0 off else leave one off for north/west slopes
	uint offset;             // flag, because old bridges had their name/copyright at the wrong position

	uint8 max_length;        // =0 off, else maximum length
	uint8 max_height;        // =0 off, else maximum length

	// number of seasons (0 = none, 1 = no snow/snow
	sint8 number_of_seasons;

public:
	/*
	 * Numbering of all image pieces
	 */
	enum img_t {
		NS_Segment,  OW_Segment,  N_Start,  S_Start,  O_Start,  W_Start,  N_Ramp,  S_Ramp,  O_Ramp,  W_Ramp,  NS_Pillar,  OW_Pillar,
		NS_Segment2, OW_Segment2, N_Start2, S_Start2, O_Start2, W_Start2, N_Ramp2, S_Ramp2, O_Ramp2, W_Ramp2, NS_Pillar2, OW_Pillar2
	};

	/*
	 * Name and Copyright used to be saved only with the Cursor
	 */
	const char *get_name() const { return get_cursor()->get_name(); }
	const char *get_copyright() const { return get_cursor()->get_copyright(); }

	skin_desc_t const* get_cursor() const { return get_child<skin_desc_t>(2 + offset); }

	image_id get_background(img_t img, uint8 season) const {
		const image_t *image = NULL;
		if(season && number_of_seasons == 1) {
			image = get_child<image_list_t>(3 + offset)->get_image(img);
		}
		if(image == NULL) {
			image = get_child<image_list_t>(0 + offset)->get_image(img);
		}
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	image_id get_foreground(img_t img, uint8 season) const {
		const image_t *image = NULL;
		if(season && number_of_seasons == 1) {
			image = get_child<image_list_t>(4 + offset)->get_image(img);
		}
		if(image == NULL) {
			image = get_child<image_list_t>(1 + offset)->get_image(img);
		}
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	img_t get_straight(ribi_t::ribi ribi, uint8 height) const;
	img_t get_start(slope_t::type slope) const;
	img_t get_ramp(slope_t::type slope) const;
	static img_t get_pillar(ribi_t::ribi ribi);

	/**
	 * @return true if this bridge can raise two level from flat terrain
	 */
	bool has_double_ramp() const;

	/**
	 * @return true if this bridge can start or end on a double slope
	 */
	bool has_double_start() const;

	img_t get_end(slope_t::type test_slope, slope_t::type ground_slope, slope_t::type way_slope) const;

	/**
	 * There is no way to distinguish between train bridge and tram bridge.
	 * However there are no real tram bridges possible in the game.
	 */
	waytype_t get_finance_waytype() const { return get_waytype(); }

	/**
	 * Distance of pillars (=0 for no pillars)
	 */
	uint8  get_pillar() const { return pillars_every; }

	/**
	 * skips lowest pillar on south/west slopes?
	 */
	bool  has_pillar_asymmetric() const { return pillars_asymmetric; }

	/**
	 * maximum bridge span (=0 for infinite)
	 */
	uint8  get_max_length() const { return max_length; }

	/**
	 * maximum bridge height (=0 for infinite)
	 */
	uint8  get_max_height() const { return max_height; }

	void calc_checksum(checksum_t *chk) const;
};

#endif
