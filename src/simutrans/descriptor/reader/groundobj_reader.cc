/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../simunits.h"
#include "../../obj/simobj.h"
#include "../../simdebug.h"
#include "../../obj/groundobj.h"
#include "../../vehicle/movingobj.h"

#include "../groundobj_desc.h"
#include "../obj_node_info.h"
#include "groundobj_reader.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void groundobj_reader_t::register_obj(obj_desc_t *&data)
{
	groundobj_desc_t *desc = static_cast<groundobj_desc_t *>(data);
	if(desc->speed==0) {
		groundobj_t::register_desc(desc);
	}
	else {
		movingobj_t::register_desc(desc);
	}

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool groundobj_reader_t::successfully_loaded() const
{
	bool gok = groundobj_t::successfully_loaded();
	return movingobj_t::successfully_loaded()  &&  gok;
}


obj_desc_t * groundobj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;
	groundobj_desc_t *desc = new groundobj_desc_t();

	if(version == 1) {
		desc->allowed_climates = (climate_bits)decode_uint16(p);
		desc->distribution_weight = decode_uint16(p);
		desc->number_of_seasons = decode_uint8(p);
		desc->trees_on_top = (bool)decode_uint8(p);
		desc->speed = kmh_to_speed( decode_uint16(p) );
		desc->wtyp = (waytype_t)decode_uint16(p);
		desc->price = decode_sint32(p);
	}
	else {
		// version 0, never existed
		dbg->fatal( "groundobj_reader_t::read_node()", "Cannot handle too new node version %i", version );
	}
	DBG_DEBUG("groundobj_reader_t::read_node()", "version=%i, climates=$%X, seasons=%i, chance=%i, speed=%i, ways=%i, cost=%d, trees_on_top=%i",
		version,
		desc->allowed_climates,
		desc->number_of_seasons,
		desc->distribution_weight,
		speed_to_kmh(desc->speed),
		desc->wtyp,
		desc->price,
		desc->trees_on_top);

	return desc;
}
