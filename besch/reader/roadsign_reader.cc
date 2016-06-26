#include <stdio.h>

#include "../../obj/roadsign.h"
#include "../../simunits.h"	// for kmh to speed conversion
#include "../roadsign_besch.h"
#include "../intro_dates.h"

#include "roadsign_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"


void roadsign_reader_t::register_obj(obj_besch_t *&data)
{
    roadsign_besch_t *besch = static_cast<roadsign_besch_t *>(data);

    roadsign_t::register_besch(besch);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool roadsign_reader_t::successfully_loaded() const
{
    return roadsign_t::alles_geladen();
}


obj_besch_t * roadsign_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	roadsign_besch_t *besch = new roadsign_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Experimental
	// @author: jamespetts

	uint16 experimental_version = 0;
	const bool experimental = version > 0 ? v & EXP_VER : false;
	if(version > 0)
	{
		if(experimental)
		{
			// Experimental version to start at 0 and increment.
			version = version & EXP_VER ? version & 0x3FFF : 0;
			while(version > 0x100)
			{
				version -= 0x100;
				experimental_version ++;
			}
			experimental_version -= 1;
		}
	}

	if(version==4) {
		// Versioned node, version 4
		besch->min_speed = kmh_to_speed(decode_uint16(p));
		besch->base_cost = decode_uint32(p);
		besch->flags = decode_uint8(p);
		besch->offset_left = decode_sint8(p);
		besch->wt = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		if(experimental)
		{
			if(experimental_version > 2)
			{
				dbg->fatal( "roadsign_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version );
			}
			besch->allow_underground = decode_uint8(p);
			if(experimental_version >= 1)
			{
				besch->signal_group = decode_uint32(p);
				besch->base_maintenance = decode_uint32(p);
				besch->max_distance_to_signalbox = decode_uint32(p); 
				besch->aspects = decode_uint8(p);
				besch->has_call_on = decode_sint8(p); 
				besch->has_selective_choose = decode_sint8(p);
				besch->working_method = (working_method_t)decode_uint8(p);
				besch->permissive = decode_sint8(p); 
				besch->max_speed = kmh_to_speed(decode_uint32(p));
				besch->base_way_only_cost = decode_uint32(p);
				besch->upgrade_group = decode_uint8(p); 
				besch->intermediate_block = decode_uint8(p);
				besch->normal_danger = decode_uint8(p);
			}
			else
			{
				besch->signal_group = 0;
				besch->base_maintenance = 0;
				besch->max_distance_to_signalbox = 0;
				besch->aspects = besch->is_choose_sign() ? 3 : 2;
				besch->has_call_on = 0; 
				besch->has_selective_choose = 0;
				besch->working_method = track_circuit_block;
				besch->permissive = 0; 
				besch->max_speed = kmh_to_speed(160); 
				besch->base_way_only_cost = besch->base_cost;
				besch->upgrade_group = 0;
				besch->intermediate_block = false;
				besch->normal_danger = false;
			}
		}
	}
	else if(version==3) {
		// Versioned node, version 3
		besch->min_speed = kmh_to_speed(decode_uint16(p));
		besch->base_cost = decode_uint32(p);
		besch->flags = decode_uint8(p);
		besch->offset_left = 14;
		besch->wt = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		if(experimental)
		{
			if(experimental_version > 1)
			{
				dbg->fatal( "roadsign_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version );
			}
			besch->allow_underground = decode_uint8(p);
		}
	}
	else if(version==2) {
		// Versioned node, version 2
		besch->min_speed = kmh_to_speed(decode_uint16(p));
		besch->base_cost = decode_uint32(p);
		besch->flags = decode_uint8(p);
		besch->offset_left = 14;
		besch->intro_date = DEFAULT_INTRO_DATE*12;
		besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
		besch->wt = road_wt;
	}
	else if(version==1) {
		// Versioned node, version 1
		besch->min_speed = kmh_to_speed(decode_uint16(p));
		besch->base_cost = 50000;
		besch->flags = decode_uint8(p);
		besch->offset_left = 14;
		besch->intro_date = DEFAULT_INTRO_DATE*12;
		besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
		besch->wt = road_wt;
	}
	else {
		dbg->fatal("roadsign_reader_t::read_node()","version 0 not supported. File corrupt?");
	}

	if(  version<=3  &&  (  besch->is_choose_sign() ||  besch->is_private_way()  )  &&  besch->get_waytype() == road_wt  ) {
		// do not shift these signs to the left for compatibility
		besch->offset_left = 0;
	}

	if(!experimental)
	{
		// Standard roadsigns can be placed both underground and above ground.
		besch->allow_underground = 2;
	}
	if(!experimental || experimental_version < 1)
	{
		besch->signal_group = 0;
		besch->base_maintenance = 0;
		besch->max_distance_to_signalbox = 0;
		besch->aspects = besch->is_choose_sign() ? 3 : 2;
		besch->has_call_on = 0; 
		besch->has_selective_choose = 0;
		besch->working_method = track_circuit_block;
		besch->permissive = 0; 
		besch->max_speed = kmh_to_speed(160); 
		besch->base_way_only_cost = besch->base_cost;
		besch->upgrade_group = 0;
		besch->intermediate_block = false;
		besch->normal_danger = false;
	}

	DBG_DEBUG("roadsign_reader_t::read_node()","min_speed=%i, cost=%i, flags=%x, waytype=%i, intro=%i%i, retire=%i,%i",
		besch->min_speed, besch->cost/100, besch->flags, besch->wt, besch->intro_date % 12 + 1, besch->intro_date / 12, besch->obsolete_date % 12 + 1, besch->obsolete_date / 12 );
	return besch;
}
