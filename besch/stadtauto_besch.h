/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __STADTAUTO_BESCH_H
#define __STADTAUTO_BESCH_H

#include "obj_besch_std_name.h"
#include "bildliste_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../network/checksum.h"


/*
 *  Author:
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
class citycar_desc_t : public obj_desc_timelined_t {
	friend class citycar_reader_t;

	uint16 chance;

	/// topspeed in internal speed units !!! not km/h!!!
	uint16 geschw;

public:
	image_id get_image_id(ribi_t::dir dir) const
	{
		image_t const* const image = get_child<image_list_t>(2)->get_image(dir);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	uint16 get_chance() const { return chance; }

	/// topspeed in internal speed units !!! not km/h!!!
	uint16 get_geschw() const { return geschw; }

	void calc_checksum(checksum_t *chk) const
	{
		obj_desc_timelined_t::calc_checksum(chk);
		chk->input(chance);
		chk->input(geschw);
	}
};

#endif
