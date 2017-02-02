#include <stdio.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h"

#include "../way_obj_besch.h"
#include "../../obj/wayobj.h"

#include "way_obj_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"


void way_obj_reader_t::register_obj(obj_desc_t *&data)
{
    way_obj_desc_t *desc = static_cast<way_obj_desc_t *>(data);
    wayobj_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool way_obj_reader_t::successfully_loaded() const
{
    return wayobj_t::successfully_loaded();
}


obj_desc_t * way_obj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	way_obj_desc_t *desc = new way_obj_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const uint16 version = v & 0x7FFF;

	if(version==1) {
		// Versioned node, version 3
		desc->cost = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->topspeed = decode_uint32(p);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
		desc->wt = decode_uint8(p);
		desc->own_wtyp = decode_uint8(p);
	}
	else {
		dbg->fatal("way_obj_reader_t::read_node()","Invalid version %d", version);
	}
  DBG_DEBUG("way_obj_reader_t::read_node()",
	     "version=%d cost=%d maintenance=%d topspeed=%d wtype=%d styp=%d intro_year=%i",
	     version,
	     desc->cost,
	     desc->maintenance,
	     desc->topspeed,
	     desc->wt,
	     desc->own_wtyp,
	     desc->intro_date/12);

  return desc;
}
