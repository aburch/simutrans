/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"

#include "../../bauer/brueckenbauer.h"
#include "../bridge_desc.h"
#include "../intro_dates.h"

#include "bridge_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"


void bridge_reader_t::register_obj(obj_desc_t *&data)
{
	bridge_desc_t *desc = static_cast<bridge_desc_t *>(data);
	bridge_builder_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}

bool bridge_reader_t::successfully_loaded() const
{
	return bridge_builder_t::successfully_loaded();
}

obj_desc_t * bridge_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("bridge_reader_t::read_node()", "called");
	ALLOCA(char, desc_buf, node.size);

	bridge_desc_t *desc = new bridge_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);

	char * p = desc_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Extended
	//@author: jamespetts

	const bool extended = version > 0 ? v & EX_VER : false;
	uint16 extended_version = 0;
	if(extended)
	{
		// Extended version to start at 0 and increment.
		version = version & EX_VER ? version & 0x3FFF : 0;
		while(version > 0x100)
		{
			version -= 0x100;
			extended_version ++;
		}
		extended_version -=1;
	}

	// some defaults
	desc->maintenance = 800;
	desc->pillars_every = 0;
	desc->pillars_asymmetric = false;
	desc->max_length = 0;
	desc->max_height = 0;
	desc->max_weight = 999;
	desc->intro_date = DEFAULT_INTRO_DATE*12;
	desc->retire_date = DEFAULT_RETIRE_DATE*12;
	desc->number_of_seasons = 0;
	way_constraints_of_way_t way_constraints;

	if(version == 1) {
		// Versioned node, version 1

		desc->wtyp = (uint8)decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);

	} else if (version == 2) {

		// Versioned node, version 2

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);

	} else if (version == 3) {

		// Versioned node, version 3
		// pillars added

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = 0;

	} else if (version == 4) {

		// Versioned node, version 3
		// pillars added

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);

	} else if (version == 5) {

		// Versioned node, version 5
		// timeline

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);

	} else if (version == 6) {

		// Versioned node, version 6
		// snow

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->number_of_seasons = decode_uint8(p);

	}
	else if (version==7  ||  version==8) {

		// Versioned node, version 7/8
		// max_height, asymmetric pillars

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->pillars_asymmetric = (decode_uint8(p)!=0);
		desc->max_height = decode_uint8(p);
		desc->number_of_seasons = decode_uint8(p);
		if(extended)
		{
			desc->max_weight = desc->axle_load = decode_uint32(p);
			way_constraints.set_permissive(decode_uint8(p));
			way_constraints.set_prohibitive(decode_uint8(p));
			if(extended_version == 1)
			{
				desc->topspeed_gradient_1 = decode_uint16(p);
				desc->topspeed_gradient_2 = decode_uint16(p);
				desc->max_altitude = decode_sint8(p);
				desc->max_vehicles_on_tile = decode_uint8(p);
				desc->has_own_way_graphics = decode_uint8(p);
				desc->has_way = decode_uint8(p);
			}
			if(extended_version > 1)
			{
				dbg->fatal("bridge_reader_t::read_node()","Incompatible pak file version for Simutrans-Extended, number %i", extended_version);
			}
		}

	}
	else if (version==9) {

		desc->topspeed = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->maintenance = decode_uint32(p);
		desc->wtyp = decode_uint8(p);
		desc->pillars_every = decode_uint8(p);
		desc->max_length = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->pillars_asymmetric = (decode_uint8(p)!=0);
		desc->max_height = decode_uint8(p);
		desc->axle_load = decode_uint16(p);	// new
		if(extended)
		{
			desc->max_weight = decode_uint32(p); // DIFFERENT to axle load.
			way_constraints.set_permissive(decode_uint8(p));
			way_constraints.set_prohibitive(decode_uint8(p));
			if(extended_version == 1)
			{
				desc->topspeed_gradient_1 = decode_uint16(p);
				desc->topspeed_gradient_2 = decode_uint16(p);
				desc->max_altitude = decode_sint8(p);
				desc->max_vehicles_on_tile = decode_uint8(p);
				desc->has_own_way_graphics = decode_uint8(p);
				desc->has_way = decode_uint8(p);
			}
			if(extended_version > 1)
			{
				dbg->fatal("bridge_reader_t::read_node()","Incompatible pak file version for Simutrans-Extended, number %i", extended_version);
			}
		}
		desc->number_of_seasons = decode_uint8(p);

	}
	else {
		// old node, version 0

		desc->wtyp = (uint8)v;
		decode_uint16(p);                    // Menupos, no more used
		desc->price = decode_uint32(p);
		desc->topspeed = 999;               // Safe default ...
	}

	desc->set_way_constraints(way_constraints);

	if(extended_version < 1 || !extended)
	{
		desc->topspeed_gradient_1 = desc->topspeed_gradient_2 = desc->topspeed;
		desc->max_altitude = 0;
		desc->max_vehicles_on_tile = 251;
		desc->has_own_way_graphics = true;
		desc->has_way = false;
	}

	// pillars cannot be heigher than this to avoid drawing errors
	if(desc->pillars_every>0  &&  desc->max_height==0) {
		desc->max_height = 7;
	}
	// indicate for different copyright/name lookup
	desc->offset = version<8 ? 0 : 2;

	desc->base_cost = desc->price;
	desc->base_maintenance = desc->maintenance;

	if(  version < 9  ) {
		desc->axle_load = 9999;
	}

	DBG_DEBUG("bridge_reader_t::read_node()",
		"version=%d, waytype=%d, price=%d, topspeed=%d, pillars=%i, max_length=%i, max_weight%d, axle_load=%i",
		version,
		desc->wtyp,
		desc->price,
		desc->maintenance,
		desc->topspeed,
		desc->axle_load,
		desc->max_length,
		desc->max_height,
		desc->pillars_every,
		desc->pillars_asymmetric,
		desc->number_of_seasons,
		(desc->intro_date % 12) + 1,
		desc->intro_date / 12,
		(desc->retire_date % 12) + 1,
		desc->retire_date / 12,
		desc->max_weight);

  return desc;
}
