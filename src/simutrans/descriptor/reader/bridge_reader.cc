/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../simdebug.h"

#include "../../builder/brueckenbauer.h"
#include "../bridge_desc.h"
#include "../intro_dates.h"

#include "bridge_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"

#include <inttypes.h>
#include <stdio.h>


void bridge_reader_t::register_obj(obj_desc_t *&data)
{
	bridge_desc_t *desc = static_cast<bridge_desc_t *>(data);

	bridge_builder_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type_name(), chk);
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
	const uint16 version = v & 0x8000 ? v & 0x7FFF : 0;

	// some defaults
	bridge_desc_t *desc = new bridge_desc_t;
	desc->maintenance = 800; // non-default maintenance for old objects

	if (version == 0) {
		desc->wtyp = (uint8)v;
		decode_uint16(p);               // Menupos, not used any more
		desc->price = decode_uint32(p);
	}
	else if (version == 1) {
		desc->wtyp     = (uint8)decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->price    = decode_uint32(p);
	}
	else if (version == 2) {
		desc->topspeed    = decode_uint16(p);
		desc->price       = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp        = decode_uint8(p);
	}
	else if (version == 3) {
		// pillars added
		desc->topspeed      = decode_uint16(p);
		desc->price         = decode_uint32(p);
		desc->maintenance   = decode_uint32(p);
		desc->wtyp          = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
	}
	else if (version == 4) {
		// max length
		desc->topspeed      = decode_uint16(p);
		desc->price         = decode_uint32(p);
		desc->maintenance   = decode_uint32(p);
		desc->wtyp          = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length    = decode_uint8(p);
	}
	else if (version == 5) {
		// timeline
		desc->topspeed      = decode_uint16(p);
		desc->price         = decode_uint32(p);
		desc->maintenance   = decode_uint32(p);
		desc->wtyp          = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length    = decode_uint8(p);
		desc->intro_date    = decode_uint16(p);
		desc->retire_date   = decode_uint16(p);
	}
	else if (version == 6) {
		// snow
		desc->topspeed          = decode_uint16(p);
		desc->price             = decode_uint32(p);
		desc->maintenance       = decode_uint32(p);
		desc->wtyp              = decode_uint8(p);
		desc->pillars_every     = decode_uint8(p);
		desc->max_length        = decode_uint8(p);
		desc->intro_date        = decode_uint16(p);
		desc->retire_date       = decode_uint16(p);
		desc->number_of_seasons = decode_uint8(p);
	}
	else if (version==7  ||  version==8) {
		// max_height, asymmetric pillars
		desc->topspeed           = decode_uint16(p);
		desc->price              = decode_uint32(p);
		desc->maintenance        = decode_uint32(p);
		desc->wtyp               = decode_uint8(p);
		desc->pillars_every      = decode_uint8(p);
		desc->max_length         = decode_uint8(p);
		desc->intro_date         = decode_uint16(p);
		desc->retire_date        = decode_uint16(p);
		desc->pillars_asymmetric = decode_uint8(p) != 0;
		desc->max_height         = decode_uint8(p);
		desc->number_of_seasons  = decode_uint8(p);
	}
	else if (version==9) {
		// axle load
		desc->topspeed           = decode_uint16(p);
		desc->price              = decode_uint32(p);
		desc->maintenance        = decode_uint32(p);
		desc->wtyp               = decode_uint8(p);
		desc->pillars_every      = decode_uint8(p);
		desc->max_length         = decode_uint8(p);
		desc->intro_date         = decode_uint16(p);
		desc->retire_date        = decode_uint16(p);
		desc->pillars_asymmetric = decode_uint8(p) != 0;
		desc->axle_load          = decode_uint16(p);
		desc->max_height         = decode_uint8(p);
		desc->number_of_seasons  = decode_uint8(p);
	}
	else if (version==10) {
		// cost/maintenance as 64 bit ints
		desc->topspeed           = decode_uint16(p);
		desc->price              = decode_sint64(p);
		desc->maintenance        = decode_sint64(p);
		desc->wtyp               = decode_uint8(p);
		desc->pillars_every      = decode_uint8(p);
		desc->max_length         = decode_uint8(p);
		desc->intro_date         = decode_uint16(p);
		desc->retire_date        = decode_uint16(p);
		desc->pillars_asymmetric = decode_uint8(p) != 0;
		desc->axle_load          = decode_uint16(p);
		desc->max_height         = decode_uint8(p);
		desc->number_of_seasons  = decode_uint8(p);
	}
	else {
		dbg->fatal( "bridge_reader_t::read_node()", "Cannot handle too new node version %i", version );
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

	PAKSET_INFO("bridge_reader_t::read_node()",
		"version=%d, waytype=%d, price=%" PRId64 ", maintenance=%" PRId64 ", topspeed=%d, axle_load=%i, max_length=%i, max_height=%i, pillars=%i, asymmetric=%i, seasons=%i, intro=%i/%i, retire=%i/%i",
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
