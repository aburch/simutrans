#include <stdio.h>

#include "../../obj/roadsign.h"
#include "../../simunits.h"	// for kmh to speed conversion
#include "../roadsign_desc.h"
#include "../intro_dates.h"

#include "roadsign_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"


void roadsign_reader_t::register_obj(obj_desc_t *&data)
{
    roadsign_desc_t *desc = static_cast<roadsign_desc_t *>(data);

    roadsign_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool roadsign_reader_t::successfully_loaded() const
{
    return roadsign_t::successfully_loaded();
}


obj_desc_t * roadsign_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	roadsign_desc_t *desc = new roadsign_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Extended
	// @author: jamespetts

	uint16 extended_version = 0;
	const bool extended = version > 0 ? v & EX_VER : false;
	if(version > 0)
	{
		if(extended)
		{
			// Extended version to start at 0 and increment.
			version = version & EX_VER ? version & 0x3FFF : 0;
			while(version > 0x100)
			{
				version -= 0x100;
				extended_version ++;
			}
			extended_version -= 1;
		}
	}

	if(version==4) {
		// Versioned node, version 4
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->base_cost = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = decode_sint8(p);
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		if(extended)
		{
			if(extended_version > 3)
			{
				dbg->fatal( "roadsign_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", extended_version );
			}
			desc->allow_underground = decode_uint8(p);
			if(extended_version >= 1)
			{
				desc->signal_group = decode_uint32(p);
				desc->base_maintenance = decode_uint32(p);
				desc->max_distance_to_signalbox = decode_uint32(p); 
				desc->aspects = decode_uint8(p);
				desc->has_call_on = decode_sint8(p); 
				desc->has_selective_choose = decode_sint8(p);
				desc->working_method = (working_method_t)decode_uint8(p);
				desc->permissive = decode_sint8(p); 
				desc->max_speed = kmh_to_speed(decode_uint32(p));
				desc->base_way_only_cost = decode_uint32(p);
				desc->upgrade_group = decode_uint8(p); 
				desc->intermediate_block = decode_uint8(p);
				desc->normal_danger = decode_uint8(p);
			}
			else
			{
				desc->signal_group = 0;
				desc->base_maintenance = 0;
				desc->max_distance_to_signalbox = 0;
				desc->aspects = desc->is_choose_sign() ? 3 : 2;
				desc->has_call_on = 0; 
				desc->has_selective_choose = 0;
				desc->working_method = track_circuit_block;
				desc->permissive = 0; 
				desc->max_speed = kmh_to_speed(160); 
				desc->base_way_only_cost = desc->base_cost;
				desc->upgrade_group = 0;
				desc->intermediate_block = false;
				desc->normal_danger = false;
			}
			if (extended_version >= 2)
			{
				desc->double_block = decode_uint8(p);
			}
			else
			{
				desc->double_block = false; 
			}
		}
	}
	else if(version==3) {
		// Versioned node, version 3
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->base_cost = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->wtyp = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		if(extended)
		{
			if(extended_version > 1)
			{
				dbg->fatal( "roadsign_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", extended_version );
			}
			desc->allow_underground = decode_uint8(p);
		}
	}
	else if(version==2) {
		// Versioned node, version 2
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->base_cost = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->wtyp = road_wt;
	}
	else if(version==1) {
		// Versioned node, version 1
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->base_cost = 50000;
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->wtyp = road_wt;
	}
	else {
		dbg->fatal("roadsign_reader_t::read_node()","version 0 not supported. File corrupt?");
	}

	if(  version<=3  &&  (  desc->is_choose_sign() ||  desc->is_private_way()  )  &&  desc->get_waytype() == road_wt  ) {
		// do not shift these signs to the left for compatibility
		desc->offset_left = 0;
	}

	if(!extended)
	{
		// Standard roadsigns can be placed both underground and above ground.
		desc->allow_underground = 2;
	}
	if(!extended || extended_version < 1)
	{
		desc->signal_group = 0;
		desc->base_maintenance = 0;
		desc->max_distance_to_signalbox = 0;
		desc->aspects = desc->is_choose_sign() ? 3 : 2;
		desc->has_call_on = 0; 
		desc->has_selective_choose = 0;
		desc->working_method = track_circuit_block;
		desc->permissive = 0; 
		desc->max_speed = kmh_to_speed(160); 
		desc->base_way_only_cost = desc->base_cost;
		desc->upgrade_group = 0;
		desc->intermediate_block = false;
		desc->normal_danger = false;
	}

	if (!extended || extended_version < 2)
	{
		desc->double_block = false;
	}

	DBG_DEBUG("roadsign_reader_t::read_node()",
		"version=%i, min_speed=%i, price=%i, flags=%x, wtyp=%i, offset_left=%i, intro=%i/%i, retire=%i/%i",
		version,
		desc->min_speed,
		desc->price / 100,
		desc->flags,
		desc->wtyp,
		desc->offset_left,
		desc->intro_date % 12 + 1,
		desc->intro_date / 12,
		desc->retire_date % 12 + 1,
		desc->retire_date / 12);
	return desc;
}
