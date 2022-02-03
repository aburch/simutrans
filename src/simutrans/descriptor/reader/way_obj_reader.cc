/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h"

#include "../way_obj_desc.h"
#include "../../obj/wayobj.h"

#include "way_obj_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void way_obj_reader_t::register_obj(obj_desc_t *&data)
{
	way_obj_desc_t *desc = static_cast<way_obj_desc_t *>(data);
	wayobj_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool way_obj_reader_t::successfully_loaded() const
{
	return wayobj_t::successfully_loaded();
}


obj_desc_t * way_obj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const uint16 version = v & 0x7FFF;

	way_obj_desc_t *desc = new way_obj_desc_t();

	if(version==1) {
		// Versioned node, version 3
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->topspeed = decode_uint32(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->wtyp = decode_uint8(p);
		desc->own_wtyp = decode_uint8(p);
	}
	else {
		dbg->fatal( "way_obj_reader_t::read_node()", "Cannot handle too new node version %i", version );
	}
	DBG_DEBUG("way_obj_reader_t::read_node()",
	     "version=%d price=%d maintenance=%d topspeed=%d wtype=%d own_wtype=%d intro=%i/%i, retire=%i/%i",
	     version,
	     desc->price,
	     desc->maintenance,
	     desc->topspeed,
	     desc->wtyp,
	     desc->own_wtyp,
	     (desc->intro_date%12)+1,
	     desc->intro_date/12,
	     (desc->retire_date%12)+1,
	     desc->retire_date/12);

	return desc;
}
