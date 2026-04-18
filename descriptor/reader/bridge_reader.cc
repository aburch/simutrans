/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../objversion.h"
#include <stdio.h>
#include "../../simdebug.h"

#include "../../bauer/brueckenbauer.h"
#include "../bridge_desc.h"
#include "../intro_dates.h"

#include "bridge_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void bridge_reader_t::register_obj(obj_desc_t *&data)
{
	bridge_desc_t *desc = static_cast<bridge_desc_t *>(data);

	bridge_builder_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


obj_desc_t *bridge_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;
	const bool extended = version > 0 ? (v & EX_VER) != 0 : false;
	uint16 extended_version = 0;
	if (extended) {
		version = version & EX_VER ? version & 0x3FFF : 0;
		while (version > 0x100) {
			version -= 0x100;
			extended_version++;
		}
		extended_version--;
	}

	// some defaults
	bridge_desc_t *desc = new bridge_desc_t();
	desc->maintenance = 800;
	desc->pillars_every = 0;
	desc->pillars_asymmetric = false;
	desc->max_length = 0;
	desc->max_height = 0;
	desc->intro_date = DEFAULT_INTRO_DATE*12;
	desc->retire_date = DEFAULT_RETIRE_DATE*12;
	desc->number_of_seasons = 0;

	if(version == 1) {
		// Versioned node, version 1

		desc->wtyp = (uint8)decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);

	} else if (version == 2) {

		// Versioned node, version 2

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);

	} else if (version == 3) {

		// Versioned node, version 3
		// pillars added

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = 0;

	} else if (version == 4) {

		// Versioned node, version 3
		// pillars added

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);

	} else if (version == 5) {

		// Versioned node, version 5
		// timeline

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);

	} else if (version == 6) {

		// Versioned node, version 6
		// snow

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->number_of_seasons = decode_uint8(p);

	}
	else if (version==7  ||  version==8) {

		// Versioned node, version 7/8
		// max_height, asymmetric pillars

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->pillars_asymmetric = (decode_uint8(p)!=0);
		desc->max_height = decode_uint8(p);
		desc->number_of_seasons = decode_uint8(p);
		if (extended) {
			// skip Extended-specific: axle_load(4), way_constraints(2), optional gradient fields
			desc->axle_load = decode_uint32(p);
			decode_uint8(p); // permissive
			decode_uint8(p); // prohibitive
			if (extended_version >= 1) {
				decode_uint16(p); // topspeed_gradient_1
				decode_uint16(p); // topspeed_gradient_2
				decode_sint8(p);  // max_altitude
				decode_uint8(p);  // max_vehicles_on_tile
				decode_uint8(p);  // has_own_way_graphics
				decode_uint8(p);  // has_way
			}
			if (extended_version > 1) {
				dbg->fatal("bridge_reader_t::read_node()", "Incompatible Extended pak version %i", extended_version);
			}
		}

	}
	else if (version==9) {

		// Extended v9 has different field order than OTRP v9:
		// Extended: ..., pillars_asymmetric, max_height, axle_load(u16), [extended fields], number_of_seasons
		// OTRP:     ..., pillars_asymmetric, axle_load(u16), max_height, number_of_seasons

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->pillars_asymmetric = (decode_uint8(p)!=0);
		if (extended) {
			// Extended reads max_height before axle_load
			desc->max_height = decode_uint8(p);
			desc->axle_load = decode_uint16(p);
			// skip Extended-specific fields: max_weight(4), way_constraints(2), optionals
			decode_uint32(p); // max_weight (Extended distinguishes from axle_load)
			decode_uint8(p);  // permissive
			decode_uint8(p);  // prohibitive
			if (extended_version >= 1) {
				decode_uint16(p); // topspeed_gradient_1
				decode_uint16(p); // topspeed_gradient_2
				decode_sint8(p);  // max_altitude
				decode_uint8(p);  // max_vehicles_on_tile
				decode_uint8(p);  // has_own_way_graphics
				decode_uint8(p);  // has_way
			}
			if (extended_version > 1) {
				dbg->fatal("bridge_reader_t::read_node()", "Incompatible Extended pak version %i", extended_version);
			}
			desc->number_of_seasons = decode_uint8(p);
		}
		else {
			// OTRP reads axle_load before max_height
			desc->axle_load = decode_uint16(p);
			desc->max_height = decode_uint8(p);
			desc->number_of_seasons = decode_uint8(p);
		}

	}
	else if (version==10) {

		desc->topspeed = decode_uint16(p);
		desc->price = decode_sint64(p);
		desc->maintenance = decode_sint64(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->pillars_asymmetric = (decode_uint8(p)!=0);
		desc->axle_load = decode_uint16(p); // new
		desc->max_height = decode_uint8(p);
		desc->number_of_seasons = decode_uint8(p);

	}
	else if (version == 11) {
		// cost/maintenance as 64 bit ints
		desc->topspeed = decode_uint16(p);
		desc->price = decode_sint64(p);
		desc->maintenance = decode_sint64(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->pillars_asymmetric = decode_uint8(p) != 0;
		desc->axle_load = decode_uint16(p);
		desc->max_height = decode_uint8(p);
		desc->number_of_seasons = decode_uint8(p);
		desc->clip_below = decode_uint8(p);
	}
	else {
		if( version ) {
			dbg->fatal( "bridge_reader_t::read_node()", "Cannot handle too new node version %i", version );
		}
		// old node, version 0
		desc->wtyp = (uint8)v;
		decode_uint16(p);                    // Menupos, no more used
		desc->price = decode_uint32(p);
		desc->topspeed = 999;               // Safe default ...
	}

	// pillars cannot be higher than this to avoid drawing errors
	if(desc->pillars_every>0  &&  desc->max_height==0) {
		desc->max_height = 7;
	}
	// indicate for different copyright/name lookup
	desc->offset = version<8 ? 0 : 2;

	if(  version < 9  ) {
		desc->axle_load = 9999;
	}
	if (version < 11) {
		desc->clip_below = desc->wtyp != powerline_wt;
	}

	DBG_DEBUG("bridge_reader_t::read_node()",
		"version=%d, waytype=%d, price=%d, maintenance=%d, topspeed=%d, axle_load=%i, max_length=%i, max_height=%i, pillars=%i, asymmetric=%i, seasons=%i, intro=%i/%i, retire=%i/%i",
		version,
		desc->wtyp,
		desc->price,
		desc->maintenance,
		desc->topspeed,
		desc->axle_load,
		desc->max_length,
		desc->max_height,
		desc->pillars_every,
		desc->pillars_asymmetric,
		desc->number_of_seasons,
		(desc->intro_date%12)+1,
		desc->intro_date/12,
		(desc->retire_date%12)+1,
		desc->retire_date/12);

	return desc;
}
