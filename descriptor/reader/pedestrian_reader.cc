/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>

#include "../../vehicle/simpeople.h"
#include "../pedestrian_desc.h"
#include "../obj_node_info.h"
#include "../intro_dates.h"

#include "pedestrian_reader.h"
#include "../../network/pakset_info.h"

/*
 *  Author:
 *      Volker Meyer
 */
void pedestrian_reader_t::register_obj(obj_desc_t *&data)
{
	pedestrian_desc_t *desc = static_cast<pedestrian_desc_t  *>(data);

	pedestrian_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}



bool pedestrian_reader_t::successfully_loaded() const
{
	return pedestrian_t::successfully_loaded();
}



/**
 * Read a pedestrian info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_desc_t * pedestrian_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	pedestrian_desc_t *desc = new pedestrian_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	desc->steps_per_frame = 0;
	desc->offset          = 20;

	// always there and never retire
	desc->intro_date = 1;
	desc->retire_date = 0xFFFEu;
	
	if(version == 0) {
		// old, nonversion node
		desc->distribution_weight = v;
	}
	else if (version == 1) {
		desc->distribution_weight = decode_uint16(p);
		desc->steps_per_frame     = decode_uint16(p);
		desc->offset              = decode_uint16(p);
	}
	else if(version == 2) {
		desc->distribution_weight = decode_uint16(p);
		desc->steps_per_frame     = decode_uint16(p);
		desc->offset              = decode_uint16(p);
		desc->intro_date          = decode_uint16(p);
		desc->retire_date         = decode_uint16(p);
	}
		DBG_DEBUG("pedestrian_reader_t::read_node()", "version=%i, chance=%i", version, desc->distribution_weight);
	return desc;
}
