/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_CROSSING_DESC_H
#define DESCRIPTOR_CROSSING_DESC_H


#include "obj_base_desc.h"
#include "image.h"
#include "image_list.h"
#include "../simtypes.h"
#include "../network/checksum.h"


class checksum_t;

/**
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Image-list
 */
class crossing_desc_t : public obj_desc_timelined_t {
	friend class crossing_reader_t;

private:
	waytype_t waytype1;
	waytype_t waytype2;

	sint8 sound;

	uint32 closed_animation_time;
	uint32 open_animation_time;

	sint32 topspeed1; // the topspeed depeds strongly on the crossing ...
	sint32 topspeed2;

public:
	/* the imagelists are:
	 * open_ns
	 * open_ew
	 * front_open_ns
	 * front_open_ew
	 * closed_ns
	 * ....
	 * =>ns=0 NorthSouth ns=1, East-West
	 */
	const image_t *get_background(uint8 ns, bool open, uint16 phase) const
	{
		if(open) {
			return get_child<image_list_t>(2 + ns)->get_image(phase);
		}
		else {
			image_list_t const* const imglist = get_child<image_list_t>(6 + ns);
			return imglist ? imglist->get_image(phase) : NULL;
		}
	}

	const image_t *get_foreground(uint8 ns, bool open, uint16 phase) const
	{
		uint8 const n = ns + (open ? 4 : 8);
		image_list_t const* const imglist = get_child<image_list_t>(n);
		return imglist ? imglist->get_image(phase) : 0;
	}

	waytype_t get_waytype(int i) const { return i==0? waytype1 : waytype2; }
	sint32 get_maxspeed(int i) const { return i==0 ? topspeed1 : topspeed2; }
	uint16 get_phases(bool open, bool front) const { return get_child<image_list_t>(6 - 4 * open + 2 * front)->get_count(); }
	uint32 get_animation_time(bool open) const { return open ? open_animation_time : closed_animation_time; }

	sint8 get_sound() const { return sound; }

	void calc_checksum(checksum_t *chk) const
	{
		chk->input(waytype1);
		chk->input(waytype2);
		chk->input(closed_animation_time);
		chk->input(open_animation_time);
		chk->input(topspeed1);
		chk->input(topspeed2);
		chk->input(intro_date);
		chk->input(retire_date);
	}
};

#endif
