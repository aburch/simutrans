/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../obj/roadsign.h"
#include "../../simunits.h" // for kmh to speed conversion
#include "../roadsign_desc.h"
#include "../intro_dates.h"

#include "roadsign_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void roadsign_reader_t::register_obj(obj_desc_t *&data)
{
	roadsign_desc_t *desc = static_cast<roadsign_desc_t *>(data);

	roadsign_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool roadsign_reader_t::successfully_loaded() const
{
	return roadsign_t::successfully_loaded();
}


obj_desc_t * roadsign_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;
	roadsign_desc_t *desc = new roadsign_desc_t();

	if(version==5) {
		// Versioned node, version 5
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->price = decode_uint32(p);
		desc->flags = decode_uint16(p);
		desc->offset_left = decode_sint8(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
	}
	else if(version==4) {
		// Versioned node, version 4
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->price = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = decode_sint8(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
	}
	else if(version==3) {
		// Versioned node, version 3
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->price = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
	}
	else if(version==2) {
		// Versioned node, version 2
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->price = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->wtyp = road_wt;
	}
	else if(version==1) {
		// Versioned node, version 1
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->price = 50000;
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->wtyp = road_wt;
	}
	else {
		dbg->fatal( "roadsign_reader_t::read_node()", "Cannot handle too new node version %i", version );
	}

	if(  version<=3  &&  (  desc->is_choose_sign() ||  desc->is_private_way()  )  &&  desc->get_waytype() == road_wt  ) {
		// do not shift these signs to the left for compatibility
		desc->offset_left = 0;
	}

	DBG_DEBUG("roadsign_reader_t::read_node()",
		"version=%i, min_speed=%i, price=%i, flags=%x, wtyp=%i, offset_left=%i, intro=%i/%i, retire=%i/%i",
		version,
		desc->min_speed,
		desc->price/100,
		desc->flags,
		desc->wtyp,
		desc->offset_left,
		desc->intro_date%12+1,
		desc->intro_date/12,
		desc->retire_date%12+1,
		desc->retire_date/12);

	return desc;
}
