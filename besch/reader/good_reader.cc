#include <stdio.h>
#include "../../simdebug.h"
#include "../../bauer/warenbauer.h"

#include "good_reader.h"
#include "../obj_node_info.h"
#include "../ware_besch.h"
#include "../../network/pakset_info.h"


void good_reader_t::register_obj(obj_besch_t *&data)
{
	ware_besch_t *besch = static_cast<ware_besch_t *>(data);

	warenbauer_t::register_besch(besch);
	DBG_DEBUG("good_reader_t::register_obj()","loaded good '%s'", besch->get_name());

	obj_for_xref(get_type(), besch->get_name(), data);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool good_reader_t::successfully_loaded() const
{
	return warenbauer_t::alles_geladen();
}


obj_besch_t * good_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	ware_besch_t *besch = new ware_besch_t();

	// some defaults
	besch->speed_bonus = 0;
	besch->weight_per_unit = 100;
	besch->color = 255;

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 1) {
		// Versioned node, version 1
		besch->base_value = decode_uint16(p);
		besch->catg = (uint8)decode_uint16(p);
		besch->speed_bonus = decode_uint16(p);
		besch->weight_per_unit = 100;

	} else if(version == 2) {
		// Versioned node, version 2

		besch->base_value = decode_uint16(p);
		besch->catg = (uint8)decode_uint16(p);
		besch->speed_bonus = decode_uint16(p);
		besch->weight_per_unit = decode_uint16(p);

	} else if(version == 3) {
		// Versioned node, version 3
		besch->base_value = decode_uint16(p);
		besch->catg = decode_uint8(p);
		besch->speed_bonus = decode_uint16(p);
		besch->weight_per_unit = decode_uint16(p);
		besch->color = decode_uint8(p);

	}
	else {
		// old node, version 0
		besch->base_value = v;
		besch->catg = (uint8)decode_uint16(p);
	}

	DBG_DEBUG("good_reader_t::read_node()","version=%d value=%d catg=%d bonus=%d",version, besch->value, besch->catg, besch->speed_bonus);


  return besch;
}
