/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>

#include "../../vehicle/simpeople.h"
#include "../fussgaenger_besch.h"
#include "../obj_node_info.h"

#include "pedestrian_reader.h"
#include "../../network/pakset_info.h"

/*
 *  Author:
 *      Volker Meyer
 */
void pedestrian_reader_t::register_obj(obj_desc_t *&data)
{
	fussgaenger_besch_t *desc = static_cast<fussgaenger_besch_t  *>(data);

	pedestrian_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}



bool pedestrian_reader_t::successfully_loaded() const
{
	return pedestrian_t::alles_geladen();
}



/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_desc_t * pedestrian_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	fussgaenger_besch_t *desc = new fussgaenger_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 0) {
		// old, nonversion node
		desc->gewichtung = v;
	}
	DBG_DEBUG("pedestrian_reader_t::read_node()","version=%i, gewichtung",version,desc->gewichtung);
	return desc;
}
