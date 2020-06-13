/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_PEDESTRIAN_DESC_H
#define DESCRIPTOR_PEDESTRIAN_DESC_H


#include "obj_base_desc.h"
#include "image_array.h"
#include "image_list.h"
#include "../dataobj/ribi.h"
#include "../network/checksum.h"

/**
 * Pedestrians.
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Image-list or 2d
 */
class pedestrian_desc_t : public obj_desc_timelined_t {
	friend class pedestrian_reader_t;

	uint16 distribution_weight;
	uint16 steps_per_frame;
	uint16 offset;

public:
	image_id get_image_id(ribi_t::dir dir, uint16 phase=0) const
	{
		image_t const* image = NULL;
		if (steps_per_frame > 0) {
			image = get_child<image_array_t>(2)->get_image(dir, phase);
		}
		else {
			image = get_child<image_list_t>(2)->get_image(dir);
		}
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	uint16 get_distribution_weight() const { return distribution_weight; }

	uint16 get_steps_per_frame() const { return steps_per_frame; }

	uint16 get_animation_count(ribi_t::dir dir) const
	{
		return steps_per_frame>0 ? get_child<image_array_t>(2)->get_list(dir)->get_count() : 1;
	}

	void calc_checksum(checksum_t *chk) const
	{
		chk->input(distribution_weight);
	}

	// images are offset steps away from boundary
	uint16 get_offset() const { return offset; }
};

#endif
