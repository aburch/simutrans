/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __pedestrian_desc_h
#define __pedestrian_desc_h

#include "obj_base_desc.h"
#include "image_list.h"
#include "../dataobj/ribi.h"
#include "../network/checksum.h"

/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *	Pedestrians.
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image-list
 */
class pedestrian_desc_t : public obj_named_desc_t {
    friend class pedestrian_reader_t;

    uint16 chance;

public:
    image_id get_image_id(ribi_t::dir dir) const
    {
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
    }

    uint16 get_chance() const { return chance; }

	void calc_checksum(checksum_t *chk) const
	{
		chk->input(chance);
	}
};

#endif
