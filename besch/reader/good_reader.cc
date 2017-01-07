#include <stdio.h>
#include "../../simdebug.h"
#include "../../bauer/warenbauer.h"

#include "good_reader.h"
#include "../obj_node_info.h"
#include "../ware_besch.h"
#include "../../network/pakset_info.h"


void good_reader_t::register_obj(obj_desc_t *&data)
{
	ware_besch_t *desc = static_cast<ware_besch_t *>(data);

	warenbauer_t::register_desc(desc);
	DBG_DEBUG("good_reader_t::register_obj()","loaded good '%s'", desc->get_name());

	obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool good_reader_t::successfully_loaded() const
{
	return warenbauer_t::successfully_loaded();
}


obj_desc_t * good_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	ware_besch_t *desc = new ware_besch_t();

	// some defaults
	desc->speed_bonus = 0;
	desc->weight_per_unit = 100;
	desc->color = 255;

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

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
		// old node, version 0
		desc->base_value = v;
		desc->catg = (uint8)decode_uint16(p);
	}

	DBG_DEBUG("good_reader_t::read_node()","version=%d value=%d catg=%d bonus=%d",version, desc->value, desc->catg, desc->speed_bonus);


  return desc;
}
