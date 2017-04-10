#include <stdio.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h"

#include "../way_desc.h"
#include "../intro_dates.h"
#include "../../bauer/wegbauer.h"

#include "way_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"


void way_reader_t::register_obj(obj_desc_t *&data)
{
    way_desc_t *desc = static_cast<way_desc_t *>(data);

    way_builder_t::register_desc(desc);
//    printf("...Weg %s geladen\n", desc->get_name());
	obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool way_reader_t::successfully_loaded() const
{
    return way_builder_t::successfully_loaded();
}


obj_desc_t * way_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	way_desc_t *desc = new way_desc_t();
	// DBG_DEBUG("way_reader_t::read_node()", "node size = %d", node.size);

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	int version = 0;

	if(node.size == 0) {
		// old node, version 0, compatibility code
		desc->price = 10000;
		desc->maintenance = 800;
		desc->topspeed = 999;
		desc->axle_load = 9999;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->wtyp = road_wt;
		desc->styp = type_flat;
		desc->draw_as_obj = false;
		desc->number_of_seasons = 0;
	}
	else {

		const uint16 v = decode_uint16(p);
		version = v & 0x7FFF;

		// Whether the read file is from Simutrans-Extended
		//@author: jamespetts

		way_constraints_of_way_t way_constraints;
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

		if(version==6) {
			// version 6, now with axle load
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->axle_load = decode_uint16(p);	// new
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = decode_uint8(p);
			desc->number_of_seasons = decode_sint8(p);
			if(extended)
			{
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				if(extended_version >= 1)
				{
					desc->topspeed_gradient_1 = decode_sint32(p);
					desc->topspeed_gradient_2 = decode_sint32(p);
					desc->max_altitude = decode_sint8(p);
					desc->max_vehicles_on_tile = decode_uint8(p);
					desc->wear_capacity = decode_uint32(p);
					desc->way_only_cost = decode_uint32(p);
					desc->upgrade_group = decode_uint8(p);
					desc->monthly_base_wear = decode_uint32(p); 
				}
				if(extended_version > 1)
				{
					dbg->fatal( "way_reader_t::read_node()","Incompatible pak file version for Simutrans-Extended, number %i", extended_version );
				}
			}
		}
		else if(version==4  ||  version==5) {
			// Versioned node, version 4+5
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_sint32(p);
			desc->axle_load = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = decode_uint8(p);
			desc->number_of_seasons = decode_sint8(p);
			if(extended)
			{
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				if(extended_version == 1)
				{
					desc->topspeed_gradient_1 = decode_sint32(p);
					desc->topspeed_gradient_2 = decode_sint32(p);
					desc->max_altitude = decode_sint8(p);
					desc->max_vehicles_on_tile = decode_uint8(p);
					desc->wear_capacity = decode_uint32(p);
					desc->way_only_cost = decode_uint32(p);
					desc->upgrade_group = decode_uint8(p);
				}
				if(extended_version > 1)
				{
					dbg->fatal("way_reader_t::read_node()","Incompatible pak file version for Simutrans-Extended, number %i", extended_version);
				}
			}

		}
		else if(version==3) {
			// Versioned node, version 3
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->axle_load = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = decode_uint8(p);
			desc->number_of_seasons = 0;
		}
		else if(version==2) {
			// Versioned node, version 2
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->axle_load = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = false;
			desc->number_of_seasons = 0;
		}
		else if(version == 1) {
			// Versioned node, version 1
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->axle_load = decode_uint32(p);
			uint32 intro_date= decode_uint32(p);
			desc->intro_date = (intro_date/16)*12 + (intro_date%16);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->retire_date = DEFAULT_RETIRE_DATE*12;
			desc->draw_as_obj = false;
			desc->number_of_seasons = 0;
		}
		else {
			dbg->fatal("way_reader_t::read_node()","Invalid version %d", version);
		}
		desc->set_way_constraints(way_constraints);
		if(extended_version < 1 || !extended)
		{
			desc->topspeed_gradient_1 = desc->topspeed_gradient_2 = desc->topspeed;
			desc->max_altitude = 0;
			desc->max_vehicles_on_tile = 251;
			desc->wear_capacity = desc->get_waytype() == road_wt ? 100000000 : 4000000000;
			desc->way_only_cost = desc->price;
			desc->upgrade_group = 0;
		}

		if(version < 6 || extended_version < 1 || !extended)
		{
			desc->monthly_base_wear = desc->wear_capacity / 600;
		}
	}

	// some internal corrections to pay for previous confusion with two waytypes
	if(desc->wtyp==tram_wt) {
		desc->styp = type_tram;
		desc->wtyp = track_wt;
	}
	else if(desc->styp==5  &&  desc->wtyp==track_wt) {
		desc->wtyp = monorail_wt;
		desc->styp = type_flat;
	}
	else if(desc->wtyp==128) {
		desc->wtyp = powerline_wt;
	}
	
	if(version<=2  &&  desc->wtyp==air_wt  &&  desc->topspeed>=250) {
		// runway!
		desc->styp = type_runway;
	}

	// front images from version 5 on
	desc->front_images = version > 4;

	desc->base_cost = desc->price;
	desc->base_maintenance = desc->maintenance;
	desc->base_way_only_cost = desc->way_only_cost;

	DBG_DEBUG("way_reader_t::read_node()",
		"version=%d, price=%d, maintenance=%d, topspeed=%d, max_weight=%d, "
		"wtype=%d, styp=%d, intro_year=%i, axle_load=%d, wear_capacity=%d, monthly_base_wear=%d, "
		"way_constraints_permissive = %d, way_constraints_prohibitive = %d",
		version,
		desc->price,
		desc->maintenance,
		desc->topspeed,
		desc->wtyp,
		desc->styp,
		desc->intro_date/12,
		desc->axle_load,
		desc->wear_capacity,
		desc->monthly_base_wear,
		desc->get_way_constraints().get_permissive(),
		desc->get_way_constraints().get_prohibitive());

  return desc;
}
