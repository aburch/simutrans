/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __SKIN_BESCH_H
#define __SKIN_BESCH_H

#include "../display/simimg.h"
#include "obj_base_desc.h"
#include "image_array.h"


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *      An image list, with name and author attributes. Mostly used for gui purposes.
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image list
 */
class skin_desc_t : public obj_named_desc_t {
public:
	image_t const* get_image(int i) const { return get_child<image_list_t>(2)->get_image(i); }

	int get_count() const { return get_child<image_list_t>(2)->get_count(); }

	image_id get_image_id(int i) const
	{
		const image_t *image = get_image(i);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}
};

#endif
