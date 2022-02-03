/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_CITYCAR_DESC_H
#define DESCRIPTOR_CITYCAR_DESC_H


#include "obj_base_desc.h"
#include "image_list.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../network/checksum.h"


/**
 * Private city cars, not player owned. They automatically appear in cities.
 *
 * Child nodes:
 *  0   Name
 *  1   Copyright
 *  2   Image-list
 */
class citycar_desc_t : public obj_desc_timelined_t {
	friend class citycar_reader_t;

	uint16 distribution_weight;

	/// topspeed in internal speed units !!! not km/h!!!
	uint16 topspeed;

public:
	image_id get_image_id(ribi_t::dir dir) const
	{
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	uint16 get_distribution_weight() const { return distribution_weight; }

	/// topspeed in internal speed units !!! not km/h!!!
	uint16 get_topspeed() const { return topspeed; }

	void calc_checksum(checksum_t *chk) const
	{
		obj_desc_timelined_t::calc_checksum(chk);
		chk->input(distribution_weight);
		chk->input(topspeed);
	}
};

#endif
