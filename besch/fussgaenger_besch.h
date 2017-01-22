/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __fussgaenger_desc_h
#define __fussgaenger_desc_h

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
#include "../network/checksum.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Description:
 *	Automatisch generierte Autos, die in der Stadt umherfahren.
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image-list
 */
class fussgaenger_desc_t : public obj_desc_timelined_t {
    friend class pedestrian_reader_t;

    uint16 chance;
public:
    int get_image_id(ribi_t::dir dir) const
    {
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
    }
    int get_chance() const
    {
		return chance;
    }
	void calc_checksum(checksum_t *chk) const
	{
		chk->input(chance);
	}
};

#endif
