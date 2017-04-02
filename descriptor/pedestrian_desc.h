/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __fussgaenger_desc_h
#define __fussgaenger_desc_h

#include "obj_base_desc.h"
#include "image_list.h"
#include "../dataobj/ribi.h"
#include "../network/checksum.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *	Private city cars, not player owned. They automatically appear in cities.
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image-list
 */
class pedestrian_desc_t : public obj_desc_timelined_t {
    friend class pedestrian_reader_t;

    uint16 distribution_weight;
public:
    int get_image_id(ribi_t::dir dir) const
    {
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
    }
    int get_distribution_weight() const
    {
		return distribution_weight;
    }
	void calc_checksum(checksum_t *chk) const
	{
		chk->input(distribution_weight);
	}
};

#endif
