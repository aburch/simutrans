#include <stdio.h>
#include "../../simdebug.h"
#include "../../simconst.h"
#include "../../bauer/vehikelbauer.h"
#include "../../boden/wege/weg.h"
#include "../sound_besch.h"
#include "../vehikel_besch.h"
#include "../intro_dates.h"

#include "vehicle_reader.h"
#include "../obj_node_info.h"



void
vehicle_reader_t::register_obj(obj_besch_t *&data)
{
	vehikel_besch_t *besch = static_cast<vehikel_besch_t *>(data);
	vehikelbauer_t::register_besch(besch);
	obj_for_xref(get_type(), besch->get_name(), data);
}



bool
vehicle_reader_t::successfully_loaded() const
{
	return vehikelbauer_t::alles_geladen();
}


obj_besch_t *
vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	vehikel_besch_t *besch = new vehikel_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;
	
	// Whether the read file is from Simutrans-Experimental
	//@author: jamespetts
	way_constraints_of_vehicle_t way_constraints;
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

	if(version == 1) {
		// Versioned node, version 1

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	} else if(version == 2) {
		// Versioned node, version 2

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	} else if (version==3   ||  version==4  ||  version==5) {
		// Versioned node, version 3 with retire date
		// version 4 identical, just other values for the waytype
		// version 5 just uses the new scheme for data calculation

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);
	} else if (version==6) {
		// version 5 just 32 bit for power and 16 Bit for gear

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	} else if (version==7) {
		// different length of cars ...

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	} else if (version==8) {
		// multiple freight images...
		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p); //"performance" (Google)
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p); //"Predecessors" (Google)
		besch->nachfolger = decode_uint8(p); //"Successor" (Google)
		besch->freight_image_type = decode_uint8(p);
		if(experimental)
		{
			if(experimental_version <= 4)
			{
				besch->is_tilting = decode_uint8(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				besch->catering_level = decode_uint8(p);
				besch->bidirectional = decode_uint8(p);
				besch->can_lead_from_rear = decode_uint8(p);
				besch->comfort = decode_uint8(p);
				besch->overcrowded_capacity = decode_uint16(p);
				besch->loading_time = decode_uint16(p);
				besch->upgrades = decode_uint8(p);
				besch->upgrade_price = decode_uint32(p);
				besch->available_only_as_upgrade = decode_uint8(p);
				if(experimental_version == 1)
				{
					besch->fixed_maintenance = decode_uint16(p);
				}
				else if(experimental_version >= 2)
				{
					besch->fixed_maintenance = decode_uint32(p);
				}
				else
				{
					besch->fixed_maintenance = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
				}
				if(experimental_version >= 3)
				{
					besch->tractive_effort = decode_uint16(p);
				}
				else
				{
					besch->tractive_effort = 0;
				}

				if(experimental_version >=4)
				{
					uint16 air_resistance_hundreds = decode_uint16(p);
					besch->air_resistance = (float)air_resistance_hundreds / 100.0F;
					besch->can_be_at_rear = (bool)decode_uint8(p);
					besch->increase_maintenance_after_years = decode_uint16(p);
					besch->increase_maintenance_by_percent = decode_uint16(p);
					besch->years_before_maintenance_max_reached = decode_uint8(p);
				}
				else
				{
					uint16 air_default;
					switch(besch->get_waytype())
					{
						default:
						case road_wt:
							air_default = 252; //2.52 when read
							break;
						case track_wt:
						case tram_wt:
						case monorail_wt:
						case narrowgauge_wt:
							air_default = 1300; //13 when read
							break;
						case water_wt:
							air_default = 2500; //25 when read
							break;
						case maglev_wt:		
							air_default = 1000; //10 when read
							break;
						case air_wt:
							air_default = 100; //1 when read
					};
					besch->air_resistance = (float) air_default / 100.0F;
					besch->can_be_at_rear = true;
					besch->increase_maintenance_after_years = 0;
					besch->increase_maintenance_by_percent = 0;
					besch->years_before_maintenance_max_reached = 0;
				}
			}
			else
			{
				dbg->fatal( "vehicle_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version );
			}
		}
	}
	else {
		if(  version!=0  ) {
			dbg->fatal( "vehicle_reader_t::read_node()","Do not know how to handle version=%i", version );
		}
		// old node, version 0

		besch->typ = (sint8)v;
		besch->zuladung = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);
		besch->sound = (sint8)decode_sint16(p);
		besch->vorgaenger = (sint8)decode_uint16(p);
		besch->nachfolger = (sint8)decode_uint16(p);

		besch->intro_date = DEFAULT_INTRO_DATE*16;
		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
		besch->gear = 64;
	}

	// correct the engine type for old vehicles
	if(version<2) {
		// steam eangines usually have a sound of 3
		// electric engines will be overridden further down ...
		besch->engine_type = (besch->sound==3) ? vehikel_besch_t::steam : vehikel_besch_t::diesel;
	}

	//change the vehicle type
	if(version<4) {
		if(besch->typ==4) {
			besch->engine_type = vehikel_besch_t::electric;
			besch->typ = 1;
		}
		// convert to new standard
		static const waytype_t convert_from_old[8]={road_wt, track_wt, water_wt, air_wt, invalid_wt, monorail_wt, invalid_wt, tram_wt };
		besch->typ = convert_from_old[besch->typ];
	}

	// before version 5 dates were based on base 12 ...
	if(version<5) {
		uint16 date=besch->intro_date;
		besch->intro_date = (date/16)*12 + (date%16);
		date=besch->obsolete_date;
		besch->obsolete_date = (date/16)*12 + (date%16);
	}

	// before the length was always 1/8 (=half a tile)
	if(version<7) {
		besch->len = 8;
	}

	// adjust length for different step sizes (which may arise in future)
	besch->len *= TILE_STEPS/16;

	// before version 8 vehicles could only have one freight image in each direction
	if(version<8) 
	{
		besch->freight_image_type = 0;
	}

	if(!experimental)
	{
		// Default values for items not in the standard vehicle format.
		besch->is_tilting = false;
		//besch->way_constraints_permissive = 0;
		//besch->way_constraints_prohibitive = 0;
		besch->catering_level = 0;
		besch->bidirectional = false;
		besch->can_lead_from_rear = false;
		besch->comfort = 100;
		besch->overcrowded_capacity = 0;
		besch->tractive_effort = 0;

		switch(besch->get_waytype())
		{
		default:	
		case tram_wt:
		case road_wt:
			besch->loading_time = 2000;
			break;

		case monorail_wt:
		case maglev_wt:
		case narrowgauge_wt:
		case track_wt:
			besch->loading_time = 4000;
			break;

		case water_wt:
			besch->loading_time = 20000;
			break;

		case air_wt:
			besch->loading_time = 30000;
			break;
		}
		uint16 air_default;

		switch(besch->get_waytype())
		{
			default:
			case road_wt:
				air_default = 252; //2.52 when read
				break;
			case track_wt:
			case tram_wt:
			case monorail_wt:
			case narrowgauge_wt:
				air_default = 1300; //13 when read
				break;
			case water_wt:
				air_default = 2500; //25 when read
				break;
			case maglev_wt:		
				air_default = 1000; //10 when read
				break;
			case air_wt:
				air_default = 100; //1 when read
		};
		besch->air_resistance = (float) air_default / 100.0F;
		besch->upgrades = 0;
		besch->upgrade_price = besch->preis;
		besch->available_only_as_upgrade = false;
		besch->fixed_maintenance = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
		besch->can_be_at_rear = true;
		besch->increase_maintenance_after_years = 0;
		besch->increase_maintenance_by_percent = 0;
		besch->years_before_maintenance_max_reached = 0;
	}
	besch->set_way_constraints(way_constraints);

	if(besch->sound==LOAD_SOUND) {
		uint8 len=decode_sint8(p);
		char wavname[256];
		wavname[len] = 0;
		for(uint8 i=0; i<len; i++) {
			wavname[i] = decode_sint8(p);
		}
		besch->sound = (sint8)sound_besch_t::get_sound_id(wavname);
DBG_MESSAGE("vehicle_reader_t::register_obj()","sound %s to %i",wavname,besch->sound);
	}
	else if(besch->sound>=0  &&  besch->sound<=MAX_OLD_SOUNDS) {
		sint16 old_id = besch->sound;
		besch->sound = (sint8)sound_besch_t::get_compatible_sound_id(old_id);
DBG_MESSAGE("vehicle_reader_t::register_obj()","old sound %i to %i",old_id,besch->sound);
	}
	besch->loaded();

	DBG_DEBUG("vehicle_reader_t::read_node()",
		"version=%d "
		"way=%d zuladung=%d preis=%d geschw=%d gewicht=%d leistung=%d "
		"betrieb=%d sound=%d vor=%d nach=%d "
		"date=%d/%d gear=%d engine_type=%d len=%d is_tilting=%d catering_level=%d "
		"way_constraints_permissive=%d way_constraints_prohibitive%d bidirectional%d can_lead_from_rear%d",
		version,
		besch->typ,
		besch->zuladung,
		besch->preis,
		besch->geschw,
		besch->gewicht,
		besch->leistung,
		besch->betriebskosten,
		besch->sound,
		besch->vorgaenger,
		besch->nachfolger,
		(besch->intro_date%12)+1,
		besch->intro_date/12,
		besch->gear,
		besch->engine_type,
		besch->len,
		besch->is_tilting,
		besch->catering_level,
		besch->get_way_constraints().get_permissive(),
		besch->get_way_constraints().get_prohibitive(),
		besch->bidirectional,
		besch->can_lead_from_rear);

	return besch;
}
