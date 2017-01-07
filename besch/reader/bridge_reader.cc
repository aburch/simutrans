#include <stdio.h>
#include "../../simdebug.h"

#include "../../bauer/brueckenbauer.h"
#include "../bruecke_besch.h"
#include "../intro_dates.h"

#include "bridge_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"


void bridge_reader_t::register_obj(obj_desc_t *&data)
{
	bruecke_besch_t *desc = static_cast<bruecke_besch_t *>(data);

	brueckenbauer_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


obj_desc_t * bridge_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("bridge_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	bruecke_besch_t *desc = new bruecke_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	// some defaults
	desc->maintenance = 800;
	desc->pillars_every = 0;
	desc->pillars_asymmetric = false;
	desc->max_length = 0;
	desc->max_height = 0;
	desc->intro_date = DEFAULT_INTRO_DATE*12;
	desc->obsolete_date = DEFAULT_RETIRE_DATE*12;
	desc->number_seasons = 0;

	if(version == 1) {
		// Versioned node, version 1

		desc->wt = (uint8)decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);

	} else if (version == 2) {

		// Versioned node, version 2

		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wt = decode_uint8(p);

	} else if (version == 3) {

		// Versioned node, version 3
		// pillars added

		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wt = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = 0;

	} else if (version == 4) {

		// Versioned node, version 3
		// pillars added

		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wt = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);

	} else if (version == 5) {

		// Versioned node, version 5
		// timeline

		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wt = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);

	} else if (version == 6) {

		// Versioned node, version 6
		// snow

		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wt = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->number_seasons = decode_uint8(p);

	}
	else if (version==7  ||  version==8) {

		// Versioned node, version 7/8
		// max_height, assymetric pillars

		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wt = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->pillars_asymmetric = (decode_uint8(p)!=0);
		desc->max_height = decode_uint8(p);
		desc->number_seasons = decode_uint8(p);

	}
	else if (version==9) {

		desc->topspeed = decode_uint16(p);
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wt = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->pillars_asymmetric = (decode_uint8(p)!=0);
		desc->axle_load = decode_uint16(p);	// new
		desc->max_height = decode_uint8(p);
		desc->number_seasons = decode_uint8(p);

	}
	else {
		// old node, version 0

		desc->wt = (uint8)v;
		decode_uint16(p);                    // Menupos, no more used
		desc->cost = decode_uint32(p);
		desc->topspeed = 999;               // Safe default ...
	}

	// pillars cannot be heigher than this to avoid drawing errors
	if(desc->pillars_every>0  &&  desc->max_height==0) {
		desc->max_height = 7;
	}
	// indicate for different copyright/name lookup
	desc->offset = version<8 ? 0 : 2;

	if(  version < 9  ) {
		desc->axle_load = 9999;
	}

	DBG_DEBUG("bridge_reader_t::read_node()",
	"version=%d waytype=%d price=%d topspeed=%d, pillars=%i, max_length=%i, axle_load=%i",
	version, desc->wt, desc->cost, desc->topspeed,desc->pillars_every,desc->max_length,desc->axle_load);

  return desc;
}
