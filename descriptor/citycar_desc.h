/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __STADTAUTO_BESCH_H
#define __STADTAUTO_BESCH_H

#include "obj_base_desc.h"
#include "image_list.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../network/checksum.h"


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *	Pedestrians
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image-list
 */
class citycar_desc_t : public obj_desc_timelined_t {
	friend class citycar_reader_t;

	uint16 distribution_weight;

	/// topspeed in internal speed units !!! not km/h!!!
	uint16 topspeed;

public:
	int get_image_id(ribi_t::dir dir) const
	{
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	int get_distribution_weight() const { return distribution_weight; }

	/// topspeed in internal speed units !!! not km/h!!!
	sint32 get_topspeed() const { return topspeed; }

	void calc_checksum(checksum_t *chk) const
	{
		obj_desc_timelined_t::calc_checksum(chk);
		chk->input(distribution_weight);
		chk->input(topspeed);
	}
};

#endif
