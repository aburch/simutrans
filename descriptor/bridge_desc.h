/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  BEWARE: non-standard node structure!
 *	0   Foreground-images
 *	1   Background-images
 *	2   Cursor/Icon (Hajo: 14-Feb-02: now also icon image)
 *	3   Foreground-images - snow
 *	4   Background-images - snow
 */

#ifndef __BRUECKE_BESCH_H
#define __BRUECKE_BESCH_H


#include "skin_desc.h"
#include "image_list.h"
#include "text_desc.h"
#include "../simtypes.h"
#include "../display/simimg.h"
#include "way_desc.h"

#include "../dataobj/ribi.h"
#include "../dataobj/way_constraints.h"

class tool_t;
class checksum_t;

class bridge_desc_t : public obj_desc_transport_infrastructure_t {
    friend class bridge_reader_t;

private:

	uint8 pillars_every;	// =0 off
	bool pillars_asymmetric;	// =0 off else leave one off for north/west slopes
	uint offset;	// flag, because old bridges had their name/copyright at the wrong position
	bool has_own_way_graphics; // Whether the way graphics are built into the bridge itself (do not allow display of other way graphics if this is set). This was traditional in Simutrans for a long time, so this is on by default.
	bool has_way;

	uint8 max_length;	// =0 off, else maximum length
	uint8 max_height;	// =0 off, else maximum length
	uint32 max_weight; //@author: jamespetts. Weight limit for convoys.

	/* number of seasons (0 = none, 1 = no snow/snow
	*/

	/*Way constraints for, e.g., loading gauges, types of electrification, etc.
	* @author: jamespetts*/
	way_constraints_of_way_t way_constraints;

	sint8 number_of_seasons;

public:
	/*
	 * Numbering of all image pieces
	 */
	enum img_t {
		NS_Segment, OW_Segment, N_Start, S_Start, O_Start, W_Start, N_Ramp, S_Ramp, O_Ramp, W_Ramp, NS_Pillar, OW_Pillar,
		NS_Segment2, OW_Segment2, N_Start2, S_Start2, O_Start2, W_Start2, N_Ramp2, S_Ramp2, O_Ramp2, W_Ramp2, NS_Pillar2, OW_Pillar2
	};

	/*
	 * Name and Copyright used to be saved only with the Cursor
	 */
	const char *get_name() const { return get_cursor()->get_name(); }
	const char *get_copyright() const { return get_cursor()->get_copyright(); }

	skin_desc_t const* get_cursor() const { return get_child<skin_desc_t>(2 + offset); }

	image_id get_background(img_t img, uint8 season) const 	{
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
	 * @return true if this bridge can start or end onm a double slope
	 */
	bool has_double_start() const;

	img_t get_end(slope_t::type test_slope, slope_t::type ground_slope, slope_t::type way_slope) const;

	/**
	 * There is no way to distinguish between train bridge and tram bridge.
	 * However there are no real tram bridges possible in the game.
	 */
	waytype_t get_finance_waytype() const { return get_waytype(); }

	/**
	 * Determines max weight in tonnes for vehicles allowed on this bridge
	 * @author jamespetts
	 */
	uint32 get_max_weight() const { return max_weight; }

	/**
	 * Distance of pillars (=0 for no pillars)
	 * @author prissi
	 */
	int  get_pillar() const { return pillars_every; }

	/**
	 * skips lowest pillar on south/west slopes?
	 * @author prissi
	 */
	bool  has_pillar_asymmetric() const { return pillars_asymmetric; }

	/**
	 * maximum bridge span (=0 for infinite)
	 * @author prissi
	 */
	int  get_max_length() const { return max_length; }

	/**
	 * maximum bridge height (=0 for infinite)
	 * @author prissi
	 */
	int  get_max_height() const { return max_height; }

	/* Way constraints: determines whether vehicles
	 * can travel on this way. This method decodes
	 * the byte into bool values. See here for
	 * information on bitwise operations: 
	 * http://www.cprogramming.com/tutorial/bitwise_operators.html
	 * @author: jamespetts
	 * */
	const way_constraints_of_way_t& get_way_constraints() const { return way_constraints; }
	void set_way_constraints(const way_constraints_of_way_t& way_constraints) { this->way_constraints = way_constraints; }

	bool get_has_own_way_graphics() const { return has_own_way_graphics; }

	const way_desc_t *get_way_desc() const
	{
		if(has_way) {
			return get_child<way_desc_t>(5 + number_of_seasons * 2);
		}
		return NULL;
	}

	void calc_checksum(checksum_t *chk) const;
};

#endif
