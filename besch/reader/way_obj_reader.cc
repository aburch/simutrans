#include <stdio.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h"

#include "../way_obj_besch.h"
#include "../../obj/wayobj.h"

#include "way_obj_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"


void way_obj_reader_t::register_obj(obj_besch_t *&data)
{
    way_obj_besch_t *besch = static_cast<way_obj_besch_t *>(data);
    wayobj_t::register_besch(besch);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool way_obj_reader_t::successfully_loaded() const
{
    return wayobj_t::alles_geladen();
}


obj_besch_t * way_obj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	way_obj_besch_t *besch = new way_obj_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const uint16 version = v & 0x7FFF;

	if(version==1) {
		// Versioned node, version 3
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->topspeed = decode_uint32(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->wt = decode_uint8(p);
		besch->own_wtyp = decode_uint8(p);
	}
	else {
		dbg->fatal("way_obj_reader_t::read_node()","Invalid version %d", version);
	}
  DBG_DEBUG("way_obj_reader_t::read_node()",
	     "version=%d cost=%d maintenance=%d topspeed=%d wtype=%d styp=%d intro_year=%i",
	     version,
	     besch->cost,
	     besch->maintenance,
	     besch->topspeed,
	     besch->wt,
	     besch->own_wtyp,
	     besch->intro_date/12);

  return besch;
}
