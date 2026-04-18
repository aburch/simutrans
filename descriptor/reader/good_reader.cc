/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../objversion.h"
#include <stdio.h>
#include "../../simdebug.h"
#include "../../bauer/goods_manager.h"

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

	obj_for_xref(get_type(), desc->get_name(), data);

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
	int version = v & 0x8000 ? v & 0x7FFF : 0;
	const bool extended = version > 0 ? (v & EX_VER) != 0 : false;
	uint16 extended_version = 0;
	if (extended) {
		version = version & EX_VER ? version & 0x3FFF : 0;
		while (version > 0x100) { version -= 0x100; extended_version++; }
		extended_version--;
	}

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
		const uint16 base_value = decode_uint16(p);
		desc->catg = decode_uint8(p);
		desc->speed_bonus = decode_uint16(p);
		desc->weight_per_unit = decode_uint16(p);
		desc->color = decode_uint8(p);
		if (extended) {
			// Extended has fare_stages and class data; OTRP uses simple base_value
			uint8 number_of_classes = 1;
			if (extended_version >= 1) {
				number_of_classes = decode_uint8(p);
			}
			const uint8 fare_stages = decode_uint8(p);
			if (fare_stages > 0) {
				// read first fare stage as base_value, skip rest
				desc->base_value = decode_uint16(p); // to_distance
				desc->base_value = decode_uint16(p); // price -> use as base_value
				for (int i = 1; i < fare_stages; i++) {
					decode_uint16(p); // to_distance
					decode_uint16(p); // price
				}
			}
			else {
				desc->base_value = base_value;
			}
			if (extended_version >= 1) {
				// skip class revenue percentages
				for (uint8 i = 0; i < number_of_classes; i++) {
					decode_uint16(p);
				}
			}
			if (extended_version > 1) {
				dbg->fatal("goods_reader_t::read_node()", "Incompatible Extended pak version %i", extended_version);
			}
		}
		else {
			desc->base_value = base_value;
		}
	}
	else if (version == 4) {
		// OTRP-only: base_value as sint64
		desc->base_value      = decode_sint64(p);
		desc->catg            = decode_uint8(p);
		desc->speed_bonus     = decode_uint16(p);
		desc->weight_per_unit = decode_uint16(p);
		desc->color           = decode_uint8(p);
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
