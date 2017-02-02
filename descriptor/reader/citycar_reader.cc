#include <stdio.h>

#include "../../simunits.h"
#include "../../vehicle/simvehicle.h"
#include "../../vehicle/simroadtraffic.h"
#include "../citycar_desc.h"
#include "../intro_dates.h"

#include "citycar_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"


void citycar_reader_t::register_obj(obj_desc_t *&data)
{
	citycar_desc_t *desc = static_cast<citycar_desc_t *>(data);

	private_car_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool citycar_reader_t::successfully_loaded() const
{
	return private_car_t::successfully_loaded();
}


obj_desc_t * citycar_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	citycar_desc_t *desc = new citycar_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 2) {
		// Versioned node, version 1

		desc->chance = decode_uint16(p);
		desc->geschw = kmh_to_speed(decode_uint16(p)/16);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
	}
	else if(version == 1) {
		// Versioned node, version 1

		desc->chance = decode_uint16(p);
		desc->geschw = kmh_to_speed(decode_uint16(p)/16);
		uint16 intro_date = decode_uint16(p);
		desc->intro_date = (intro_date/16)*12  + (intro_date%12);
		uint16 obsolete_date = decode_uint16(p);
		desc->obsolete_date= (obsolete_date/16)*12  + (obsolete_date%12);
	}
	else {
		desc->chance = v;
		desc->geschw = kmh_to_speed(80);
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->obsolete_date = DEFAULT_RETIRE_DATE*12;
	}
	// zero speed not allowed, we want something that moves!
	if(  desc->geschw<=16  ) {
		dbg->warning( "citycar_reader_t::read_node()", "citycar must have minimum speed => changed to 1.25 km/h!" );
		desc->geschw = 16;
	}
	DBG_DEBUG("citycar_reader_t::read_node()","version=%i, weight=%i, intro=%i, retire=%i speed=%i",version,desc->chance,desc->intro_date/12,desc->obsolete_date/12, speed_to_kmh(desc->geschw) );
	return desc;
}
