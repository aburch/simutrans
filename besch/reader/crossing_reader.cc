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
	kreuzung_besch_t *besch = static_cast<kreuzung_besch_t *>(data);
	if(besch->topspeed1!=0) {
		crossing_logic_t::register_besch(besch);
	}

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


obj_besch_t * crossing_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	kreuzung_besch_t *besch = new kreuzung_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 0) {
		dbg->error("crossing_reader_t::read_node()","Old version of crossings cannot be used!");

		besch->wegtyp1 = (uint8)v;
		besch->wegtyp2 = (uint8)decode_uint16(p);
		besch->topspeed1 = 0;
		besch->topspeed2 = 0;
	}
	else if(  version==1  ||  version==2  ) {
		besch->wegtyp1 = decode_uint8(p);
		besch->wegtyp2 = decode_uint8(p);
		besch->topspeed1 = decode_uint16(p);
		besch->topspeed2 = decode_uint16(p);
		besch->open_animation_time = decode_uint32(p);
		besch->closed_animation_time = decode_uint32(p);
		besch->sound = decode_sint8(p);

		if(besch->sound==LOAD_SOUND) {
			uint8 len=decode_sint8(p);
			char wavname[256];
			wavname[len] = 0;
			for(uint8 i=0; i<len; i++) {
				wavname[i] = decode_sint8(p);
			}
			besch->sound = (sint8)sound_besch_t::get_sound_id(wavname);
		}
		else if(besch->sound>=0  &&  besch->sound<=MAX_OLD_SOUNDS) {
			sint16 old_id = besch->sound;
			besch->sound = (sint8)sound_besch_t::get_compatible_sound_id((sint8)old_id);
		}

		besch->intro_date = 0;
		besch->obsolete_date = 65535;
		if (version >= 2 ) {
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
		}
	}
	else {
		dbg->fatal( "crossing_reader_t::read_node()","Invalid version %d", version);
	}
	return besch;
}
