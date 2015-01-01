#include <stdio.h>

#include "../../simdebug.h"

#include "../../dataobj/ribi.h"

#include "../intro_dates.h"
#include "../tunnel_besch.h"
#include "../obj_besch.h"
#include "../obj_node_info.h"
#include "tunnel_reader.h"

#include "../../bauer/tunnelbauer.h"
#include "../../network/pakset_info.h"


void tunnel_reader_t::register_obj(obj_besch_t *&data)
{
	tunnel_besch_t *besch = static_cast<tunnel_besch_t *>(data);
	if(besch->get_topspeed()==0) {
		convert_old_tunnel(besch);
	}
	DBG_DEBUG("tunnel_reader_t::register_obj", "Loaded '%s'", besch->get_name());
	tunnelbauer_t::register_besch(besch);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}

/**
 * Sets default data for ancient tunnel paks
 */
void tunnel_reader_t::convert_old_tunnel(tunnel_besch_t *besch)
{
	// old style, need to convert
	if(strcmp(besch->get_name(),"RoadTunnel")==0) {
		besch->wt = (uint8)road_wt;
		besch->topspeed = 120;
	}
	else {
		besch->wt = (uint8)track_wt;
		besch->topspeed = 280;
	}
	besch->maintenance = 500;
	besch->cost = 200000;
	besch->intro_date = DEFAULT_INTRO_DATE*12;
	besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
	besch->has_way = false;
}


obj_besch_t * tunnel_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	tunnel_besch_t *besch = new tunnel_besch_t();
	besch->topspeed = 0;	// indicate, that we have to convert this to reasonable date, when read completely
	besch->node_info = new obj_besch_t*[node.children];

	if(node.size>0) {
		// newer versioned node
		ALLOCA(char, besch_buf, node.size);

		fread(besch_buf, node.size, 1, fp);

		char * p = besch_buf;

		const uint16 v = decode_uint16(p);
		int version = v & 0x8000 ? v & 0x7FFF : 0;

		// Whether the read file is from Simutrans-Experimental
		//@author: jamespetts
		way_constraints_of_way_t way_constraints;
		const bool experimental = version > 0 ? v & EXP_VER : false;
		uint16 experimental_version = 0;
		if(experimental)
		{
			// Experimental version to start at 0 and increment.
			version = version & EXP_VER ? version & 0x3FFF : 0;
			while(version > 0x100)
			{
				version -= 0x100;
				experimental_version ++;
			}
			experimental_version -=1;
		}

		if( version == 5 ) {
			// versioned node, version 5 - axle load
			besch->topspeed = decode_uint32(p);
			besch->cost = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wt = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->axle_load = decode_uint16(p);	// new
			besch->number_seasons = decode_uint8(p);
			besch->has_way = decode_uint8(p);
			besch->broad_portals = decode_uint8(p);
			if(experimental)
			{
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				if(experimental_version == 1)
				{
					besch->topspeed_gradient_1 = decode_uint16(p);
					besch->topspeed_gradient_2 = decode_uint16(p);
					besch->max_altitude = decode_sint8(p);
					besch->max_vehicles_on_tile = decode_uint8(p);
				}
				if(experimental_version > 1)
				{
					dbg->fatal("tunnel_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version);
				}
			}
		}
		else if( version == 4 ) {
			// versioned node, version 4 - broad portal support
			besch->topspeed = decode_uint32(p);
			besch->cost = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wt = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = decode_uint8(p);
			
			if(experimental)
			{
				besch->axle_load = decode_uint32(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				if(experimental_version == 1)
				{
					besch->topspeed_gradient_1 = decode_uint16(p);
					besch->topspeed_gradient_2 = decode_uint16(p);
					besch->max_altitude = decode_sint8(p);
					besch->max_vehicles_on_tile = decode_uint8(p);
				}
				if(experimental_version > 1)
				{
					dbg->fatal("tunnel_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version);
				}
			}
			besch->has_way = decode_uint8(p);
			besch->broad_portals = decode_uint8(p);
		}
		else if(version == 3) {
			// versioned node, version 3 - underground way image support
			besch->topspeed = decode_uint32(p);
			besch->cost = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wt = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = decode_uint8(p);
			besch->has_way = decode_uint8(p);
			if(experimental)
			{
				besch->axle_load =  decode_uint32(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				if(experimental_version > 0)
				{
					dbg->fatal("tunnel_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version);
				}
			}
			besch->broad_portals = 0;
		}
		else if(version == 2) {
			// versioned node, version 2 - snow image support
			besch->topspeed = decode_uint32(p);
			besch->cost = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wt = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = decode_uint8(p);
			if(experimental)
			{
				if(experimental_version == 0)
				{
					besch->axle_load =  decode_uint32(p);
					way_constraints.set_permissive(decode_uint8(p));
					way_constraints.set_prohibitive(decode_uint8(p));
				}
				else
				{
					dbg->fatal("tunnel_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version);
				}
			}
			besch->has_way = 0;
			besch->broad_portals = 0;
		}
		else if(version == 1) {
			// first versioned node, version 1
			besch->topspeed = decode_uint32(p);
			besch->cost = decode_uint32(p);
			besch->maintenance = decode_uint32(p);
			besch->wt = decode_uint8(p);
			besch->intro_date = decode_uint16(p);
			besch->obsolete_date = decode_uint16(p);
			besch->number_seasons = 0;
			besch->axle_load = 999;
			besch->has_way = 0;
			besch->broad_portals = 0;
		}
		else {
			dbg->fatal("tunnel_reader_t::read_node()","illegal version %d",version);
		}

		if( !experimental && version < 5  ) {
			besch->axle_load = 9999;
		}
		
		if(experimental_version < 1 || !experimental)
		{
			besch->topspeed_gradient_1 = besch->topspeed_gradient_2 = besch->topspeed;
			besch->max_altitude = 0;
			besch->max_vehicles_on_tile = 251;
		}

		besch->set_way_constraints(way_constraints);

		besch->base_cost = besch->cost;
		besch->base_maintenance = besch->maintenance;
		DBG_DEBUG("tunnel_reader_t::read_node()",
		     "version=%d waytype=%d price=%d topspeed=%d, intro_year=%d, axle_load=%d",
			 version, besch->wt, besch->cost, besch->topspeed, besch->intro_date/12, besch->axle_load);
	}

	return besch;
}
