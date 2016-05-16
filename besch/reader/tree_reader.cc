#include <stdio.h>

#include "../../simobj.h"
#include "../../simdebug.h"
#include "../../obj/baum.h"

#include "../baum_besch.h"
#include "../obj_node_info.h"
#include "tree_reader.h"
#include "../../network/pakset_info.h"


void tree_reader_t::register_obj(obj_besch_t *&data)
{
    baum_besch_t *besch = static_cast<baum_besch_t *>(data);

    baum_t::register_besch(besch);
//    printf("...Baum %s geladen\n", besch->get_name());
	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool tree_reader_t::successfully_loaded() const
{
    return baum_t::alles_geladen();
}


obj_besch_t * tree_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	baum_besch_t *besch = new baum_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;
	if(version==2) {
		// Versioned node, version 2
		besch->allowed_climates = (climate_bits)decode_uint16(p);
		besch->distribution_weight = (uint8)decode_uint8(p);
		besch->number_of_seasons = (uint8)decode_uint8(p);
	} else if(version == 1) {
		// Versioned node, version 1
		besch->allowed_climates = all_but_arctic_climate;
		besch->number_of_seasons = 0;
		decode_uint8(p);	// ignore hoehenlage
		besch->distribution_weight = (uint8)decode_uint8(p);
	} else {
		// old node, version 0
		besch->number_of_seasons = 0;
		besch->allowed_climates = all_but_arctic_climate;
		besch->distribution_weight = 3;
	}
	DBG_DEBUG("tree_reader_t::read_node()", "climates=$%X, seasons %i, and weight=%i (ver=%i, node.size=%i)",besch->allowed_climates,besch->number_of_seasons,besch->distribution_weight, version, node.size);

	return besch;
}
