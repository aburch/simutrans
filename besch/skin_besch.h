/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __SKIN_BESCH_H
#define __SKIN_BESCH_H

#include "../display/simimg.h"
#include "obj_besch_std_name.h"
#include "bildliste2d_besch.h"


/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      Skin ist im wesentlichen erstmal eine Image-list.
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image list
 */
class skin_besch_t : public obj_besch_std_name_t {
public:
	image_t const* get_image(uint16 i) const { return get_child<image_list_t>(2)->get_image(i); }

	uint16 get_count() const { return get_child<image_list_t>(2)->get_count(); }

	image_id get_image_id(uint16 i) const
	{
		const image_t *image = get_image(i);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}
};

#endif
