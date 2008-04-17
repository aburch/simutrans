#include <stdio.h>
#include <string.h>
#include "../../bauer/hausbauer.h"
#include "../../simdebug.h"
#include "../haus_besch.h"
#include "../intro_dates.h"
#include "../obj_node_info.h"
#include "building_reader.h"


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

	if (besch->utype == haus_besch_t::fabrik) {
		// this stuff is just for compatibility
		if(  strcmp("Oelbohrinsel",besch->gib_name())==0  ) {
			besch->enables = 1|2|4;
		}
		else if(  strcmp("fish_swarm",besch->gib_name())==0  ) {
			besch->enables = 4;
		}
	}

	if (besch->utype == haus_besch_t::weitere && besch->enables == 0x80) {
		// this stuff is just for compatibility
		long checkpos=strlen(besch->gib_name());
		besch->enables = 0;
		// before station buildings were identified by their name ...
		if(  strcmp("BusStop",besch->gib_name()+checkpos-7)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = road_wt;
			besch->enables = 1;
		}
		if(  strcmp("CarStop",besch->gib_name()+checkpos-7)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = road_wt;
			besch->enables = 4;
		}
		else if(  strcmp("TrainStop",besch->gib_name()+checkpos-9)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = track_wt;
			besch->enables = 1|4;
		}
		else if(  strcmp("ShipStop",besch->gib_name()+checkpos-8)==0  ) {
			besch->utype = haus_besch_t::hafen;
			besch->enables = 1|4;
		}
		else if(  strcmp("ChannelStop",besch->gib_name()+checkpos-11)==0  ) {
			besch->utype = haus_besch_t::generic_stop;
			besch->extra_data = water_wt;
			besch->enables = 1|4;
		}
		else if(  strcmp("PostOffice",besch->gib_name()+checkpos-10)==0  ) {
			besch->utype = haus_besch_t::generic_extension;
			besch->extra_data = 0;
			besch->enables = 2;
		}
		else if(  strcmp("StationBlg",besch->gib_name()+checkpos-10)==0  ) {
			besch->utype = haus_besch_t::generic_extension;
			besch->extra_data = 0;
			besch->enables = 1|4;
		}
	}
	// now old style depots ...
	else if(besch->utype==haus_besch_t::weitere) {
		long checkpos=strlen(besch->gib_name());
		if(  strcmp("AirDepot",besch->gib_name()+checkpos-8)==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)air_wt;
		}
		else if(  strcmp("TrainDepot",besch->gib_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)track_wt;
		}
		else if(  strcmp("TramDepot",besch->gib_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)tram_wt;
		}
		else if(  strcmp("MonorailDepot",besch->gib_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)monorail_wt;
		}
		else if(  strcmp("CarDepot",besch->gib_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)road_wt;
		}
		else if(  strcmp("ShipDepot",besch->gib_name())==0  ) {
			besch->utype = haus_besch_t::depot;
			besch->extra_data = (uint16)water_wt;
		}
	}
	// and finally old stations ...
	else if(  besch->gib_utyp()>=haus_besch_t::bahnhof  &&  besch->gib_utyp()<=haus_besch_t::lagerhalle) {
		// compability stuff
		static uint16 old_to_new_waytype[16] = { track_wt, road_wt, road_wt, water_wt, water_wt, air_wt, monorail_wt, 0, track_wt, road_wt, road_wt, 0 , water_wt, air_wt, monorail_wt, 0 };
		besch->extra_data = besch->utype<=haus_besch_t::monorail_geb ? old_to_new_waytype[besch->utype-haus_besch_t::bahnhof] : 0;
		if(  besch->utype!=haus_besch_t::hafen  ) {
			besch->utype = besch->utype<haus_besch_t::bahnhof_geb ? haus_besch_t::generic_stop : haus_besch_t::generic_extension;
		}
	}

    hausbauer_t::register_besch(besch);
    DBG_DEBUG("building_reader_t::register_obj", "Loaded '%s'", besch->gib_name());
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

	if(version == 5) {
		// Versioned node, version 5
		// animation intergvall in ms added
		besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates = (climate_bits)decode_uint16(p);
		besch->enables   = decode_uint8(p);
		besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = decode_uint16(p);
	}
	else if(version == 4) {
		// Versioned node, version 4
		// climates and seasons added
		besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates = (climate_bits)decode_uint16(p);
		besch->enables   = decode_uint8(p);
		besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = 300;
	}
	else if(version == 3) {
		// Versioned node, version 3
		besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = decode_uint8(p);
		besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = 300;
	}
	else if(version == 2) {
		// Versioned node, version 2
		besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = 0x80;
		besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);
		besch->intro_date    = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->animation_time = 300;
	}
	else if(version == 1) {
		// Versioned node, version 1
		besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint8(p);
		besch->level     = decode_uint16(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint8(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = 0x80;
		besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
		besch->chance    = decode_uint8(p);

		besch->intro_date    = DEFAULT_INTRO_DATE*12;
		besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
		besch->animation_time = 300;
	} else {
		// old node, version 0
		besch->gtyp      = (enum gebaeude_t::typ)v;
		decode_uint16(p);
		besch->utype     = (haus_besch_t::utyp)decode_uint32(p);
		besch->level     = decode_uint32(p);
		besch->extra_data= decode_uint32(p);
		besch->groesse.x = decode_uint16(p);
		besch->groesse.y = decode_uint16(p);
		besch->layouts   = decode_uint32(p);
		besch->allowed_climates   =  (climate_bits)0xFFFE; // all but water
		besch->enables   = 0x80;
		besch->flags     = (enum haus_besch_t::flag_t)decode_uint32(p);
		besch->chance    = 100;

		besch->intro_date    = DEFAULT_INTRO_DATE*12;
		besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
		besch->animation_time = 300;
	}

	// correct old station buildings ...
	if (besch->level <= 0 && (besch->utype >= haus_besch_t::bahnhof || besch->utype == haus_besch_t::fabrik)) {
		DBG_DEBUG("building_reader_t::read_node()","old station building -> set level to 4");
		besch->level = 4;
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
