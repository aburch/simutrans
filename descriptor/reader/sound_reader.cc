/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../sound_desc.h"
#include "sound_reader.h"

#include "../obj_node_info.h"

#include "../../simdebug.h"


void sound_reader_t::register_obj(obj_desc_t *&data)
{
	sound_desc_t *desc = static_cast<sound_desc_t *>(data);
	sound_desc_t::register_desc(desc);
	DBG_DEBUG("sound_reader_t::read_node()","sound %s registered at %i",desc->get_name(),desc->sound_id);
	delete desc;
}


obj_desc_t * sound_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	sound_desc_t *desc = new sound_desc_t();

	// Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version==1) {
		// Versioned node, version 2
		desc->nr = decode_uint16(p);
	}
	else if(  version == 2  ) {
		// Versioned node, version 2
		desc->nr = decode_uint16(p);
		uint16 len = decode_uint16(p);
		if(  len>0  ) {
			desc->nr = desc->get_sound_id(p);
		}
	}
	else {
		dbg->fatal("sound_reader_t::read_node()","version %i not supported. File corrupt?", version);
	}

	return desc;
}
