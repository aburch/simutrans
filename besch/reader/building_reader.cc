#include <stdio.h>
#include <string.h>
#include "../../bauer/hausbauer.h"
#include "../../simdebug.h"
#include "../haus_besch.h"
#include "../intro_dates.h"
#include "../obj_node_info.h"
#include "building_reader.h"
#include "../../dataobj/pakset_info.h"


obj_besch_t * tile_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	haus_tile_besch_t *besch = new haus_tile_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = (v & 0x8000)!=0 ? v&0x7FFF : 0;

	if(version == 2) {
//  DBG_DEBUG("tile_reader_t::read_node()","version=1");
		// Versioned node, version 1
		besch->phasen = (uint8)decode_uint16(p);
		besch->index = decode_uint16(p);
		besch->seasons = decode_uint8(p);
		besch->haus = NULL;
	}
	else if(version == 1) {
//  DBG_DEBUG("tile_reader_t::read_node()","version=1");
		// Versioned node, version 1
		besch->phasen = (uint8)decode_uint16(p);
		besch->index = decode_uint16(p);
		besch->seasons = 1;
		besch->haus = NULL;
	}
	else {
		// skip the pointer ...
		p += 2;
		besch->phasen = (uint8)decode_uint16(p);
		besch->index = decode_uint16(p);
		besch->seasons = 1;
		besch->haus = NULL;
	}
	DBG_DEBUG("tile_reader_t::read_node()","phasen=%i index=%i seasons=%i", besch->phasen, besch->index, besch->seasons );

	return besch;
}




void building_reader_t::register_obj(obj_besch_t *&data)
{
	haus_besch_t *besch = static_cast<haus_besch_t *>(data);

	if(  besch->utype == haus_besch_t::fabrik  ) {
		if(  besch->enables == 0  ) {
			// this stuff is just for compatibility
			if(  strcmp("Oelbohrinsel",besch->get_name())==0  ) {
				besch->enables = 1|2|4;
			}
			else if(  strcmp("fish_swarm",besch->get_name())==0  ) {
				besch->enables = 4;
			}
		}
	}

	if(  besch->utype == haus_besch_t::weitere  &&  besch->enables == 0x80  ) {
		// this stuff is just for compatibility
		size_t checkpos = strlen(besch->get_name());
		besch->enables = 0;
		// before station buildings were identified by their name ...
		if(  strcmp("BusStop",besch->get_name()+checkpos-7)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = road_wt;
			besch->enables = 1;
		}
		if(  strcmp("CarStop",besch->get_name()+checkpos-7)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = road_wt;
			besch->enables = 4;
		}
		else if(  strcmp("TrainStop",besch->get_name()+checkpos-9)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = track_wt;
			besch->enables = 1|4;
		}
		else if(  strcmp("ShipStop",besch->get_name()+checkpos-8)==0  ) {
			besch->utype = haus_besch_t::hafen;
			besch->extra_data = water_wt;
			besch->enables = 1|4;
		}
		else if(  strcmp("ChannelStop",besch->get_name()+checkpos-11)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = water_wt;
			besch->enables = 1|4;
		}
		else if(  strcmp("PostOffice",besch->get_name()+checkpos-10)==0  ) {
			besch->utype = haus_besch_t::generic_extension;
			besch->extra_data = 0;
			besch->enables = 2;
		}
		else if(  strcmp("StationBlg",besch->get_name()+checkpos-10)==0  ) {
			besch->utype = haus_besch_t::generic_extension;
			besch->extra_data = 0;
			besch->enables = 1|4;
		}
	}
	// now old style depots ...
	else if(  besch->utype==haus_besch_t::weitere  ) {
		size_t checkpos = strlen(besch->get_name());
		if(  strcmp("AirDepot",besch->get_name()+checkpos-8)==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)air_wt;
		}
		else if(  strcmp("TrainDepot",besch->get_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)track_wt;
		}
		else if(  strcmp("TramDepot",besch->get_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)tram_wt;
		}
		else if(  strcmp("MonorailDepot",besch->get_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)monorail_wt;
		}
		else if(  strcmp("CarDepot",besch->get_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)road_wt;
		}
		else if(  strcmp("ShipDepot",besch->get_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)water_wt;
		}
	}
	// and finally old stations ...
	else if(  besch->get_utyp()>=haus_besch_t::bahnhof  &&  besch->get_utyp()<=haus_besch_t::lagerhalle) {
		// compability stuff
		static uint16 old_to_new_waytype[16] = { track_wt, road_wt, road_wt, water_wt, water_wt, air_wt, monorail_wt, 0, track_wt, road_wt, road_wt, 0 , water_wt, air_wt, monorail_wt, 0 };
		besch->extra_data = besch->utype<=haus_besch_t::monorail_geb ? old_to_new_waytype[besch->utype-haus_besch_t::bahnhof] : 0;
		if(  besch->utype!=haus_besch_t::hafen  ) {
			besch->utype = besch->utype<haus_besch_t::bahnhof_geb ? haus_besch_t::generic_stop : haus_besch_t::generic_extension;
		}
	}

	if(  besch->layouts>2  &&  besch->layouts&1  ) {
		uint8 l = besch->layouts>4 ? 4 : 2;
		dbg->error( "building_reader_t::register_obj()", "Building %s has %i layouts (illegal) => set to %i", besch->get_name(), besch->layouts, l );
		besch->layouts = l;
	}

	hausbauer_t::register_besch(besch);
	DBG_DEBUG("building_reader_t::register_obj", "Loaded '%s'", besch->get_name());

	// do not calculate checksum if factory, will be done in factory_reader_t
	if(  besch->utype!=haus_besch_t::fabrik  ) {
		checksum_t *chk = new checksum_t();
		besch->calc_checksum(chk);
		pakset_info_t::append(besch->get_name(), chk);
	}
}


bool building_reader_t::successfully_loaded() const
{
	return hausbauer_t::alles_geladen();
}


obj_besch_t * building_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	haus_besch_t *besch = new haus_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;
	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = (v & 0x8000)!=0 ? v&0x7FFF : 0;

	if(version >= 5) {
		// Versioned node, version 5+
		// animation intervall in ms added
		besch->gtyp      = (gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates = (climate_bits)decode_uint16(p);
		besch->enables   = decode_uint8(p);
		besch->flags     = (haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = decode_uint16(p);
		if (version >= 7) {
			// allow_underground added
			besch->allow_underground = decode_uint8(p);
		}
		else {
			besch->allow_underground = 2;
		}
	}
	else if(version == 4) {
		// Versioned node, version 4
		// climates and seasons added
		besch->gtyp      = (gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates = (climate_bits)decode_uint16(p);
		besch->enables   = decode_uint8(p);
		besch->flags     = (haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = 300;
	}
	else if(version == 3) {
		// Versioned node, version 3
		besch->gtyp      = (gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = decode_uint8(p);
		besch->flags     = (haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = 300;
	}
	else if(version == 2) {
		// Versioned node, version 2
		besch->gtyp      = (gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = 0x80;
		besch->flags     = (haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = 300;
	}
	else if(version == 1) {
		// Versioned node, version 1
		besch->gtyp      = (gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = 0x80;
		besch->flags     = (haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);

		besch->intro_date    = DEFAULT_INTRO_DATE*12;
		besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
		besch->animation_time = 300;
	}
	else {
		// old node, version 0
		besch->gtyp      = (gebaeude_t::typ)v;
		decode_uint16(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint32(p);
		besch->level     = decode_uint32(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint32(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = 0x80;
		besch->flags     = (haus_besch_t::flag_t)decode_uint32(p);
		besch->chance    = 100;

		besch->intro_date    = DEFAULT_INTRO_DATE*12;
		besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
		besch->animation_time = 300;
	}
	// there are additional nodes for cursor/icon
	if(  node.children > 2+besch->groesse.x*besch->groesse.y*besch->layouts  ) {
		besch->flags |= haus_besch_t::FLAG_HAS_CURSOR;
	}

	// correct old station buildings ...
	if(  version<=3  &&  (besch->utype >= haus_besch_t::bahnhof  ||  besch->utype == haus_besch_t::fabrik  ||  besch->utype == haus_besch_t::depot)  &&  besch->level==0  ) {
		DBG_DEBUG("building_reader_t::read_node()","old station building -> set level to 4");
		besch->level = 4;
	}
	else if(  version<=5  &&  (besch->utype == haus_besch_t::fabrik  ||  besch->utype == haus_besch_t::depot)  ) {
		besch->level ++;
		DBG_DEBUG("building_reader_t::read_node()","old station building -> increment level by one to %i", besch->level );
	}


	if (besch->level == 65535) {
		besch->level = 0;	// apparently wrong level
		dbg->warning("building_reader_t::read_node()","level was 65535, intended was probably 0 => changed." );
	}

	DBG_DEBUG("building_reader_t::read_node()",
		"version=%d"
		" gtyp=%d"
		" utyp=%d"
		" level=%d"
		" extra_data=%d"
		" groesse.x=%d"
		" groesse.y=%d"
		" layouts=%d"
		" enables=%x"
		" flags=%d"
		" chance=%d"
		" climates=%X"
		" anim=%d"
		" intro=%d"
		" retire=%d",
		version,
		besch->gtyp,
		besch->utype,
		besch->level,
		besch->extra_data,
		besch->groesse.x,
		besch->groesse.y,
		besch->layouts,
		besch->enables,
		besch->flags,
		besch->chance,
		besch->allowed_climates,
		besch->animation_time,
		besch->intro_date,
		besch->obsolete_date
	);

	return besch;

}
