#include <stdio.h>

#include "../sound_besch.h"
#include "sound_reader.h"

#include "../obj_node_info.h"

#include "../../simdebug.h"


void sound_reader_t::register_obj(obj_besch_t *&data)
{
	sound_besch_t *besch = static_cast<sound_besch_t *>(data);
	sound_besch_t::register_besch(besch);
	DBG_DEBUG("sound_reader_t::read_node()","sound %s registered at %i",besch->get_name(),besch->sound_id);
	delete besch;
}


obj_besch_t * sound_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	sound_besch_t *besch = new sound_besch_t();

		// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version==1) {
		// Versioned node, version 2
		besch->nr = decode_uint16(p);
	}
	else if(  version == 2  ) {
		// Versioned node, version 2
		besch->nr = decode_uint16(p);
		uint16 len = decode_uint16(p);
		if(  len>0  ) {
			besch->nr = besch->get_sound_id(p);
		}
	}
	else {
		dbg->fatal("sound_reader_t::read_node()","version %i not supported. File corrupt?", version);
	}

	return besch;
}
