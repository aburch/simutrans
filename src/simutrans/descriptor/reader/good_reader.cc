/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"
#include "../../builder/goods_manager.h"

#include "good_reader.h"
#include "../obj_node_info.h"
#include "../goods_desc.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void goods_reader_t::register_obj(obj_desc_t *&data)
{
	goods_desc_t *desc = static_cast<goods_desc_t *>(data);

	goods_manager_t::register_desc(desc);
	DBG_DEBUG("goods_reader_t::register_obj()","loaded good '%s'", desc->get_name());

	pakset_manager_t::obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool goods_reader_t::successfully_loaded() const
{
	return goods_manager_t::successfully_loaded();
}


obj_desc_t * goods_reader_t::read_node(FILE *fp, obj_node_info_t &node)
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

	// some defaults
	goods_desc_t *desc = new goods_desc_t();
	desc->speed_bonus = 0;
	desc->weight_per_unit = 100;
	desc->color = 255;

	if(version == 1) {
		// Versioned node, version 1
		desc->base_value = decode_uint16(p);
		desc->catg = (uint8)decode_uint16(p);
		desc->speed_bonus = decode_uint16(p);
		desc->weight_per_unit = 100;

	}
	else if(version == 2) {
		// Versioned node, version 2

		desc->base_value = decode_uint16(p);
		desc->catg = (uint8)decode_uint16(p);
		desc->speed_bonus = decode_uint16(p);
		desc->weight_per_unit = decode_uint16(p);

	}
	else if(version == 3) {
		// Versioned node, version 3
		desc->base_value = decode_uint16(p);
		desc->catg = decode_uint8(p);
		desc->speed_bonus = decode_uint16(p);
		desc->weight_per_unit = decode_uint16(p);
		desc->color = decode_uint8(p);

	}
	else {
		if( version ) {
			dbg->fatal( "goods_reader_t::read_node()", "Cannot handle too new node version %i", version );
		}
		// old node, version 0
		desc->base_value = v;
		desc->catg = (uint8)decode_uint16(p);
	}

	DBG_DEBUG("goods_reader_t::read_node()","version=%d, value=%d, catg=%d, bonus=%d, weight=%i, color=%i",
		version,
		desc->base_value,
		desc->catg,
		desc->speed_bonus,
		desc->weight_per_unit,
		desc->color);


	return desc;
}
