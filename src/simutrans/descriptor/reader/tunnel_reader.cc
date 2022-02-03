/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../simdebug.h"

#include "../../dataobj/ribi.h"

#include "../intro_dates.h"
#include "../tunnel_desc.h"
#include "../obj_desc.h"
#include "../obj_node_info.h"
#include "tunnel_reader.h"

#include "../../builder/tunnelbauer.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void tunnel_reader_t::register_obj(obj_desc_t *&data)
{
	tunnel_desc_t *desc = static_cast<tunnel_desc_t *>(data);
	if(desc->get_topspeed()==0) {
		convert_old_tunnel(desc);
	}
	DBG_DEBUG("tunnel_reader_t::register_obj", "Loaded '%s'", desc->get_name());
	tunnel_builder_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}

/**
 * Sets default data for ancient tunnel paks
 */
void tunnel_reader_t::convert_old_tunnel(tunnel_desc_t *desc)
{
	// old style, need to convert
	if(strcmp(desc->get_name(),"RoadTunnel")==0) {
		desc->wtyp = (uint8)road_wt;
		desc->topspeed = 120;
	}
	else {
		desc->wtyp = (uint8)track_wt;
		desc->topspeed = 280;
	}
	desc->maintenance = 500;
	desc->price = 200000;
	desc->intro_date = DEFAULT_INTRO_DATE*12;
	desc->retire_date = DEFAULT_RETIRE_DATE*12;
	desc->has_way = false;
}


obj_desc_t * tunnel_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	tunnel_desc_t *desc = new tunnel_desc_t();
	desc->topspeed = 0; // indicate, that we have to convert this to reasonable date, when read completely

	if(node.size==0) {
		return desc;
	}

	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		delete desc;
		return NULL;
	}
	char *p = desc_buf.begin();

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if( version == 5 ) {
		// versioned node, version 5 - axle load
		desc->topspeed = decode_uint32(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->axle_load = decode_uint16(p); // new
		desc->number_of_seasons = decode_uint8(p);
		desc->has_way = decode_uint8(p);
		desc->broad_portals = decode_uint8(p);
	}
	else if( version == 4 ) {
		// versioned node, version 4 - broad portal support
		desc->topspeed = decode_uint32(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->number_of_seasons = decode_uint8(p);
		desc->has_way = decode_uint8(p);
		desc->broad_portals = decode_uint8(p);
	}
	else if(version == 3) {
		// versioned node, version 3 - underground way image support
		desc->topspeed = decode_uint32(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->number_of_seasons = decode_uint8(p);
		desc->has_way = decode_uint8(p);
		desc->broad_portals = 0;
	}
	else if(version == 2) {
		// versioned node, version 2 - snow image support
		desc->topspeed = decode_uint32(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->number_of_seasons = decode_uint8(p);
		desc->has_way = 0;
		desc->broad_portals = 0;
	}
	else if(version == 1) {
		// first versioned node, version 1
		desc->topspeed = decode_uint32(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->number_of_seasons = 0;
		desc->has_way = 0;
		desc->broad_portals = 0;
	}
	else {
		dbg->fatal( "tunnel_reader_t::read_node()", "Cannot handle too new node version %i", version );
	}

	if(  version < 5  ) {
		desc->axle_load = 9999;
	}

	DBG_DEBUG("tunnel_reader_t::read_node()",
		"version=%d, waytype=%d, price=%d, maintenance=%d, topspeed=%d, intro=%d/%d, retire=%d/%d, axle_load=%d, has_way=%i, seasons=%i, b_portals=%i",
		version,
		desc->wtyp,
		desc->price,
		desc->maintenance,
		desc->topspeed,
		(desc->intro_date%12)+1,
		desc->intro_date/12,
		(desc->retire_date%12)+1,
		desc->retire_date/12,
		desc->axle_load,
		desc->has_way,
		desc->number_of_seasons,
		desc->broad_portals);

	return desc;
}
