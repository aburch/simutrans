/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../dataobj/crossing_logic.h"

#include "../sound_desc.h"
#include "../crossing_desc.h"
#include "crossing_reader.h"

#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void crossing_reader_t::register_obj(obj_desc_t *&data)
{
	crossing_desc_t *desc = static_cast<crossing_desc_t *>(data);
	if(desc->topspeed1!=0) {
		crossing_logic_t::register_desc(desc);
	}

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


obj_desc_t * crossing_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	crossing_desc_t *desc = new crossing_desc_t();

	if(version == 0) {
		dbg->error("crossing_reader_t::read_node()","Old version of crossings cannot be used!");

		desc->waytype1 = (waytype_t)v;
		desc->waytype2 = (waytype_t)decode_uint16(p);
		desc->topspeed1 = 0;
		desc->topspeed2 = 0;
	}
	else if(  version==1  ||  version==2  ) {
		desc->waytype1 = (waytype_t)decode_uint8(p);
		desc->waytype2 = (waytype_t)decode_uint8(p);
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
			desc->sound = (sint8)sound_desc_t::get_sound_id(wavname);
		}
		else if(desc->sound>=0  &&  desc->sound<=MAX_OLD_SOUNDS) {
			sint16 old_id = desc->sound;
			desc->sound = (sint8)sound_desc_t::get_compatible_sound_id((sint8)old_id);
		}

		desc->intro_date = 0;
		desc->retire_date = 65535;
		if (version >= 2 ) {
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
		}
	}
	else {
		dbg->fatal( "crossing_reader_t::read_node()", "Cannot handle too new node version %i", version );
	}

	DBG_DEBUG("crossing_reader_t::read_node()",
		"version=%i, waytype1=%d, waytype2=%d, topspeed1=%i, topspeed2=%i, open_time=%i, close_time=%i, sound=%i",
		version,
		desc->waytype1,
		desc->waytype2,
		desc->topspeed1,
		desc->topspeed2,
		desc->open_animation_time,
		desc->closed_animation_time,
		desc->sound);

	return desc;
}
