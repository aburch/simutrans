/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../simunits.h"
#include "../../vehicle/vehicle.h"
#include "../../vehicle/simroadtraffic.h"
#include "../citycar_desc.h"
#include "../intro_dates.h"

#include "citycar_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void citycar_reader_t::register_obj(obj_desc_t *&data)
{
	citycar_desc_t *desc = static_cast<citycar_desc_t *>(data);

	private_car_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool citycar_reader_t::successfully_loaded() const
{
	return private_car_t::successfully_loaded();
}


obj_desc_t * citycar_reader_t::read_node(FILE *fp, obj_node_info_t &node)
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

	citycar_desc_t *desc = new citycar_desc_t();

	if(version == 2) {
		// Versioned node, version 1

		desc->distribution_weight = decode_uint16(p);
		desc->topspeed = kmh_to_speed(decode_uint16(p)/16);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
	}
	else if(version == 1) {
		// Versioned node, version 1

		desc->distribution_weight = decode_uint16(p);
		desc->topspeed = kmh_to_speed(decode_uint16(p)/16);
		uint16 intro_date = decode_uint16(p);
		desc->intro_date = (intro_date/16)*12  + (intro_date%12);
		uint16 retire_date = decode_uint16(p);
		desc->retire_date= (retire_date/16)*12  + (retire_date%12);
	}
	else {
		if( version ) {
			dbg->fatal( "citycar_reader_t::read_node()", "Cannot handle too new node version %i", version );
		}
		// old version 0 ...
		desc->distribution_weight = v;
		desc->topspeed = kmh_to_speed(80);
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
	}
	// zero speed not allowed, we want something that moves!
	if(  desc->topspeed<=16  ) {
		dbg->warning( "citycar_reader_t::read_node()", "citycar must have minimum speed => changed to 1.25 km/h!" );
		desc->topspeed = 16;
	}
	DBG_DEBUG("citycar_reader_t::read_node()","version=%i, speed=%i, chance=%i, intro=%i/%i, retire=%i/%i",
		version,
		speed_to_kmh(desc->topspeed),
		desc->distribution_weight,
		(desc->intro_date%12)+1,
		desc->intro_date/12,
		(desc->retire_date%12)+1,
		desc->retire_date/12);

	return desc;
}
