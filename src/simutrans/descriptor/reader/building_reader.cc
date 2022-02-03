/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>
#include "../../builder/hausbauer.h"
#include "../../simdebug.h"
#include "../building_desc.h"
#include "../intro_dates.h"
#include "../obj_node_info.h"
#include "building_reader.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


/**
 * Old building types, for compatibility ...
 */
struct old_btyp
{
	/**
	 * From type "unknown" also come special buildings e.q. Townhall
	 */
	enum typ {
		wohnung,
		gewerbe,
		industrie,
		unknown
	};
};

obj_desc_t * tile_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = (v & 0x8000)!=0 ? v&0x7FFF : 0;

	building_tile_desc_t *desc = new building_tile_desc_t();

	if(version == 2) {
//  DBG_DEBUG("tile_reader_t::read_node()","version=1");
		// Versioned node, version 1
		desc->phases = (uint8)decode_uint16(p);
		desc->index = decode_uint16(p);
		desc->seasons = decode_uint8(p);
		desc->building = NULL;
	}
	else if(version == 1) {
//  DBG_DEBUG("tile_reader_t::read_node()","version=1");
		// Versioned node, version 1
		desc->phases = (uint8)decode_uint16(p);
		desc->index = decode_uint16(p);
		desc->seasons = 1;
		desc->building = NULL;
	}
	else {
		if( version ) {
			dbg->fatal( "tile_reader_t::read_node()", "Cannot handle too new node version %i", version );
		}
		// skip the pointer ...
		p += 2;
		desc->phases = (uint8)decode_uint16(p);
		desc->index = decode_uint16(p);
		desc->seasons = 1;
		desc->building = NULL;
	}
	DBG_DEBUG("tile_reader_t::read_node()","phases=%i, index=%i, seasons=%i",
		desc->phases,
		desc->index,
		desc->seasons );

	return desc;
}




void building_reader_t::register_obj(obj_desc_t *&data)
{
	building_desc_t *desc = static_cast<building_desc_t *>(data);

	if(  desc->type == building_desc_t::factory  ) {
		if(  desc->enables == 0  ) {
			// this stuff is just for compatibility
			if(  strcmp("Oelbohrinsel",desc->get_name())==0  ) {
				desc->enables = 1|2|4;
			}
			else if(  strcmp("fish_swarm",desc->get_name())==0  ) {
				desc->enables = 4;
			}
		}
	}

	if(  desc->type == building_desc_t::others  &&  desc->enables == 0x80  ) {
		// this stuff is just for compatibility
		size_t checkpos = strlen(desc->get_name());
		desc->enables = 0;
		// before station buildings were identified by their name ...
		if(  strcmp("BusStop",desc->get_name()+checkpos-7)==0  ) {
			desc->type = building_desc_t::generic_stop;
			desc->extra_data = road_wt;
			desc->enables = 1;
		}
		if(  strcmp("CarStop",desc->get_name()+checkpos-7)==0  ) {
			desc->type = building_desc_t::generic_stop;
			desc->extra_data = road_wt;
			desc->enables = 4;
		}
		else if(  strcmp("TrainStop",desc->get_name()+checkpos-9)==0  ) {
			desc->type = building_desc_t::generic_stop;
			desc->extra_data = track_wt;
			desc->enables = 1|4;
		}
		else if(  strcmp("ShipStop",desc->get_name()+checkpos-8)==0  ) {
			desc->type = building_desc_t::dock;
			desc->extra_data = water_wt;
			desc->enables = 1|4;
		}
		else if(  strcmp("ChannelStop",desc->get_name()+checkpos-11)==0  ) {
			desc->type = building_desc_t::generic_stop;
			desc->extra_data = water_wt;
			desc->enables = 1|4;
		}
		else if(  strcmp("PostOffice",desc->get_name()+checkpos-10)==0  ) {
			desc->type = building_desc_t::generic_extension;
			desc->extra_data = 0;
			desc->enables = 2;
		}
		else if(  strcmp("StationBlg",desc->get_name()+checkpos-10)==0  ) {
			desc->type = building_desc_t::generic_extension;
			desc->extra_data = 0;
			desc->enables = 1|4;
		}
	}
	// now old style depots ...
	else if(  desc->type==building_desc_t::others  ) {
		size_t checkpos = strlen(desc->get_name());
		if(  strcmp("AirDepot",desc->get_name()+checkpos-8)==0  ) {
			desc->type = building_desc_t::depot;
			desc->extra_data = (uint16)air_wt;
		}
		else if(  strcmp("TrainDepot",desc->get_name())==0  ) {
			desc->type = building_desc_t::depot;
			desc->extra_data = (uint16)track_wt;
		}
		else if(  strcmp("TramDepot",desc->get_name())==0  ) {
			desc->type = building_desc_t::depot;
			desc->extra_data = (uint16)tram_wt;
		}
		else if(  strcmp("MonorailDepot",desc->get_name())==0  ) {
			desc->type = building_desc_t::depot;
			desc->extra_data = (uint16)monorail_wt;
		}
		else if(  strcmp("CarDepot",desc->get_name())==0  ) {
			desc->type = building_desc_t::depot;
			desc->extra_data = (uint16)road_wt;
		}
		else if(  strcmp("ShipDepot",desc->get_name())==0  ) {
			desc->type = building_desc_t::depot;
			desc->extra_data = (uint16)water_wt;
		}
	}
	// and finally old stations ...
	// correct all building types in building_desc_t::old_building_types_t
	else if(  (uint8)desc->get_type()>=building_desc_t::bahnhof  &&  (uint8)desc->get_type()<=building_desc_t::lagerhalle) {
		// compatibility stuff
		static uint16 old_to_new_waytype[16] = { track_wt, road_wt, road_wt, water_wt, water_wt, air_wt, monorail_wt, 0, track_wt, road_wt, road_wt, 0 , water_wt, air_wt, monorail_wt, 0 };
		uint8 type = desc->type;
		desc->extra_data = type <= building_desc_t::monorail_geb ? old_to_new_waytype[type-building_desc_t::bahnhof] : 0;
		if(  type !=building_desc_t::dock  ) {
			desc->type = type < building_desc_t::bahnhof_geb ? building_desc_t::generic_stop : building_desc_t::generic_extension;
		}
	}
	// after this point all building_desc_t's have type in building_desc_t::btype

	// allowed layouts are 1,2,4,8,16, where 8,16 is reserved for stations
	uint8 l = desc->type == building_desc_t::generic_stop ? 16 : 4;
	while (l > 0) {
		if ((desc->layouts & l) != 0  &&  (desc->layouts != l)) {
			dbg->error( "building_reader_t::register_obj()", "Building %s has %i layouts (illegal) => set to %i", desc->get_name(), desc->layouts, l );
			desc->layouts = l;
			break;
		}
		l >>= 1;
	}

	if(  desc->allow_underground == 255  ) {
		// only old stops were allowed underground
		desc->allow_underground = desc->type==building_desc_t::generic_stop ? 2 : 0;
	}

	if (hausbauer_t::register_desc(desc)) {
		DBG_DEBUG("building_reader_t::register_obj", "Loaded '%s'", desc->get_name());

		// do not calculate checksum of factory, will be done in factory_reader_t
		if(  desc->type != building_desc_t::factory  ) {
			checksum_t *chk = new checksum_t();
			desc->calc_checksum(chk);
			pakset_info_t::append(desc->get_name(), get_type(), chk);
		}
	}
}


bool building_reader_t::successfully_loaded() const
{
	return hausbauer_t::successfully_loaded();
}


obj_desc_t * building_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char * p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = (v & 0x8000)!=0 ? v&0x7FFF : 0;

	old_btyp::typ btyp;
	building_desc_t *desc = new building_desc_t();

	if(version == 10) {
		// preservation date added
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = (climate_bits)(decode_uint16(p) & ALL_CLIMATES);
		desc->enables = decode_uint8(p);
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->animation_time = decode_uint16(p);
		desc->capacity = decode_uint16(p);
		desc->maintenance = decode_sint32(p);
		desc->price = decode_sint32(p);
		desc->allow_underground = decode_uint8(p);
		desc->preservation_year_month = decode_uint16(p);
	}
	else if(version == 8  ||  version == 9) {
		// Versioned node, version 8
		// station price, maintenance and capacity added
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = (climate_bits)(decode_uint16(p) & ALL_CLIMATES);
		desc->enables = decode_uint8(p);
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->animation_time = decode_uint16(p);
		desc->capacity = decode_uint16(p);
		desc->maintenance = decode_sint32(p);
		desc->price = decode_sint32(p);
		desc->allow_underground = decode_uint8(p);
	}
	else if(version == 7) {
		// Versioned node, version 7
		// underground mode added
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = (climate_bits)(decode_uint16(p) & ALL_CLIMATES);
		desc->enables = decode_uint8(p);
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->animation_time = decode_uint16(p);
		desc->allow_underground = decode_uint8(p);
	}
	else if(version == 5  ||  version==6) {
		// Versioned node, version 5 or 6  (only level logic is different)
		// animation interval in ms added
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = (climate_bits)(decode_uint16(p) & ALL_CLIMATES);
		desc->enables = decode_uint8(p);
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->animation_time = decode_uint16(p);
	}
	else if(version == 4) {
		// Versioned node, version 4
		// climates and seasons added
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = (climate_bits)(decode_uint16(p) & ALL_CLIMATES);
		desc->enables = decode_uint8(p);
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->animation_time = 300;
	}
	else if(version == 3) {
		// Versioned node, version 3
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = all_but_water_climate; // all but water
		desc->enables = decode_uint8(p);
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->animation_time = 300;
	}
	else if(version == 2) {
		// Versioned node, version 2
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = all_but_water_climate; // all but water
		desc->enables = 0x80;
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->animation_time = 300;
	}
	else if(version == 1) {
		// Versioned node, version 1
		btyp = (old_btyp::typ)decode_uint8(p);
		desc->type = (building_desc_t::btype)decode_uint8(p);
		desc->level = decode_uint16(p);
		desc->extra_data = decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint8(p);
		desc->allowed_climates = all_but_water_climate; // all but water
		desc->enables = 0x80;
		desc->flags = (building_desc_t::flag_t)decode_uint8(p);
		desc->distribution_weight = decode_uint8(p);

		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->animation_time = 300;
	}
	else {
		if( version ) {
			dbg->fatal( "building_reader_t::read_node()", "Cannot handle too new node version %i", version );
		}
		// old node, version 0
		btyp = (old_btyp::typ)v;
		decode_uint16(p);
		desc->type = (building_desc_t::btype)decode_uint32(p);
		desc->level = decode_uint32(p);
		desc->extra_data= decode_uint32(p);
		desc->size.x = decode_uint16(p);
		desc->size.y = decode_uint16(p);
		desc->layouts = decode_uint32(p);
		desc->allowed_climates = all_but_water_climate; // all but water
		desc->enables = 0x80;
		desc->flags = (building_desc_t::flag_t)decode_uint32(p);
		desc->distribution_weight = 100;

		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->animation_time = 300;
	}
	// there are additional nodes for cursor/icon
	if(  node.children > 2+desc->size.x*desc->size.y*desc->layouts  ) {
		desc->flags |= building_desc_t::FLAG_HAS_CURSOR;
	}

	// correct old station buildings ...
	if(  version<=3  &&  ((uint8)desc->type >= building_desc_t::bahnhof  ||  desc->type == building_desc_t::factory  ||  desc->type == building_desc_t::depot)  &&  desc->level==0  ) {
		DBG_DEBUG("building_reader_t::read_node()","old station building -> set level to 4");
		desc->level = 4;
	}
	else if(  version<=5  &&  (desc->type == building_desc_t::factory  ||  desc->type == building_desc_t::depot)  ) {
		desc->level ++;
		DBG_DEBUG("building_reader_t::read_node()","old station building -> increment level by one to %i", desc->level );
	}

	if(  version<=6  ) {
		// only stops were allowed underground
		desc->allow_underground = 255;
	}

	if(  version<=7  ) {
		// capacity, maintenance and price were set from level
		desc->capacity = desc->level * 32;
		desc->maintenance = PRICE_MAGIC;
		desc->price = PRICE_MAGIC;
	}

	if (desc->level == 65535) {
		desc->level = 0; // apparently wrong level
		dbg->warning("building_reader_t::read_node()","level was 65535, intended was probably 0 => changed." );
	}

	if (  version < 9  ) {
		switch(btyp) {
			case old_btyp::wohnung:    desc->type = building_desc_t::city_res; break;
			case old_btyp::gewerbe:    desc->type = building_desc_t::city_com; break;
			case old_btyp::industrie:  desc->type = building_desc_t::city_ind; break;
			default: ;
		}
	}

	if( version < 10 ) {
		// can always replace
		desc->preservation_year_month = DEFAULT_RETIRE_DATE*12;
	}

	DBG_DEBUG("building_reader_t::read_node()",
		"version=%d,"
		" btyp=%d,"
		" type=%d,"
		" price=%d,"
		" maintenance=%d,"
		" capacity=%d,"
		" level=%d,"
		" extra_data=%d,"
		" size.x=%d,"
		" size.y=%d,"
		" layouts=%d,"
		" enables=%x,"
		" flags=%d,"
		" chance=%d,"
		" climates=%X,"
		" metro=%d,"
		" anim=%d,"
		" intro=%d/%d,"
		" retire=%d/%d,",
		version,
		btyp,
		desc->type,
		desc->price,
		desc->maintenance,
		desc->capacity,
		desc->level,
		desc->extra_data,
		desc->size.x,
		desc->size.y,
		desc->layouts,
		desc->enables,
		desc->flags,
		desc->distribution_weight,
		desc->allowed_climates,
		desc->allow_underground,
		desc->animation_time,
		(desc->intro_date%12)+1,
		desc->intro_date/12,
		(desc->retire_date%12)+1,
		desc->retire_date/12
	);

	return desc;

}
