#include <stdio.h>

#include "../../dataobj/crossing_logic.h"

#include "../sound_besch.h"
#include "../kreuzung_besch.h"
#include "crossing_reader.h"

#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"


void crossing_reader_t::register_obj(obj_besch_t *&data)
{
	kreuzung_besch_t *desc = static_cast<kreuzung_besch_t *>(data);
	if(desc->topspeed1!=0) {
		crossing_logic_t::register_desc(desc);
	}

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


obj_besch_t * crossing_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	kreuzung_besch_t *desc = new kreuzung_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 0) {
		dbg->error("crossing_reader_t::read_node()","Old version of crossings cannot be used!");

		desc->wegtyp1 = (uint8)v;
		desc->wegtyp2 = (uint8)decode_uint16(p);
		desc->topspeed1 = 0;
		desc->topspeed2 = 0;
	}
	else if(  version==1  ||  version==2  ) {
		desc->wegtyp1 = decode_uint8(p);
		desc->wegtyp2 = decode_uint8(p);
		desc->topspeed1 = decode_uint16(p);
		desc->topspeed2 = decode_uint16(p);
		desc->open_animation_time = decode_uint32(p);
		desc->closed_animation_time = decode_uint32(p);
		desc->sound = decode_sint8(p);

		if(desc->sound==LOAD_SOUND) {
			uint8 len=decode_sint8(p);
			char wavname[256];
			wavname[len] = 0;
			for(uint8 i=0; i<len; i++) {
				wavname[i] = decode_sint8(p);
			}
			desc->sound = (sint8)sound_besch_t::get_sound_id(wavname);
		}
		else if(desc->sound>=0  &&  desc->sound<=MAX_OLD_SOUNDS) {
			sint16 old_id = desc->sound;
			desc->sound = (sint8)sound_besch_t::get_compatible_sound_id((sint8)old_id);
		}

		desc->intro_date = 0;
		desc->obsolete_date = 65535;
		if (version >= 2 ) {
			desc->intro_date = decode_uint16(p);
			desc->obsolete_date = decode_uint16(p);
		}
	}
	else {
		dbg->fatal( "crossing_reader_t::read_node()","Invalid version %d", version);
	}
	return desc;
}
