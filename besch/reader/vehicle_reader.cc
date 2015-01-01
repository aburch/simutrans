#include <stdio.h>
#include "../../simdebug.h"
#include "../../simconst.h"
#include "../../bauer/vehikelbauer.h"
#include "../sound_besch.h"
#include "../vehikel_besch.h"
#include "../intro_dates.h"

#include "vehicle_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"



void vehicle_reader_t::register_obj(obj_besch_t *&data)
{
	vehikel_besch_t *besch = static_cast<vehikel_besch_t *>(data);
	vehikelbauer_t::register_besch(besch);
	obj_for_xref(get_type(), besch->get_name(), data);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool vehicle_reader_t::successfully_loaded() const
{
	return vehikelbauer_t::alles_geladen();
}


obj_besch_t *vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
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
		experimental_version -= 1;
	}

	way_constraints_of_vehicle_t way_constraints;

	if(version == 1) {
		// Versioned node, version 1

		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if(version == 2) {
		// Versioned node, version 2

		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if (version==3   ||  version==4  ||  version==5) {
		// Versioned node, version 3 with retire date
		// version 4 identical, just other values for the waytype
		// version 5 just uses the new scheme for data calculation

		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);
	}
	else if (version==6) {
		// version 5 just 32 bit for power and 16 Bit for gear

		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	}
	else if (version==7) {
		// different length of cars ...

		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	}
	else if (version==8) {
		// multiple freight images...

		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->freight_image_type = decode_uint8(p);
		if(experimental)
		{
			if(experimental_version <= 6)
			{
				besch->is_tilting = decode_uint8(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				besch->catering_level = decode_uint8(p);
				besch->bidirectional = decode_uint8(p);
				besch->can_lead_from_rear = decode_uint8(p);
				besch->comfort = decode_uint8(p);
				besch->overcrowded_capacity = decode_uint16(p);
				besch->min_loading_time = besch->max_loading_time = decode_uint16(p);
				besch->upgrades = decode_uint8(p);
				besch->base_upgrade_price = decode_uint32(p);
				besch->available_only_as_upgrade = decode_uint8(p);
				besch->brake_force = BRAKE_FORCE_UNKNOWN;
				besch->minimum_runway_length = 10;
				besch->rolling_resistance = vehikel_besch_t::get_rolling_default(besch->wt) / float32e8_t::tenthousand;
				if(experimental_version == 1)
				{
					besch->base_fixed_cost = decode_uint16(p);
				}
				else if(experimental_version >= 2)
				{
					besch->base_fixed_cost = decode_uint32(p);
				}
				else
				{
					besch->base_fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
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
					uint32 air_resistance_hundreds = decode_uint16(p);
					besch->air_resistance = air_resistance_hundreds / float32e8_t::hundred;
					besch->can_be_at_rear = (bool)decode_uint8(p);
					besch->increase_maintenance_after_years = decode_uint16(p);
					besch->increase_maintenance_by_percent = decode_uint16(p);
					besch->years_before_maintenance_max_reached = decode_uint8(p);
				}
				else
				{
					besch->air_resistance = vehikel_besch_t::get_air_default(besch->wt) / float32e8_t::hundred;
					besch->can_be_at_rear = true;
					besch->increase_maintenance_after_years = 0;
					besch->increase_maintenance_by_percent = 0;
					besch->years_before_maintenance_max_reached = 0;
				}
				if(experimental_version >= 5)
				{
					besch->livery_image_type = decode_uint8(p);
				}
				else
				{
					besch->livery_image_type = 0;
				}
				if(experimental_version >= 6)
				{
					// With minimum and maximum loading times in seconds
					besch->min_loading_time_seconds = decode_uint16(p);
					besch->max_loading_time_seconds = decode_uint16(p);
				}
				else
				{
					besch->min_loading_time_seconds = besch->max_loading_time_seconds = 65535;
				}
			}
			else
			{
				dbg->fatal( "vehicle_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version );
			}
		}
	}
	else if (version==9) {
		// new: fixed_cost (previously Experimental only), loading_time, axle_load
		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		if(experimental_version == 0)
		{
			// The new Standard datum for loading times is read here.
			besch->min_loading_time = besch->max_loading_time = decode_uint16(p);
		}
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->axle_load = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);
		if(experimental_version == 0)
		{
			// Experimental has this as a 32-bit integer, and reads it later.
			besch->base_fixed_cost = decode_uint16(p);
		}

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);		//"Predecessors" (Google)
		besch->nachfolger = decode_uint8(p);		//"Successor" (Google)
		besch->freight_image_type = decode_uint8(p);
		if(experimental)
		{
			if(experimental_version <= 7)
			{
				besch->is_tilting = decode_uint8(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				besch->catering_level = decode_uint8(p);
				besch->bidirectional = decode_uint8(p);
				besch->can_lead_from_rear = decode_uint8(p);
				besch->comfort = decode_uint8(p);
				besch->overcrowded_capacity = decode_uint16(p);
				besch->min_loading_time = besch->max_loading_time = decode_uint16(p);
				besch->upgrades = decode_uint8(p);
				besch->base_upgrade_price = decode_uint32(p);
				besch->available_only_as_upgrade = decode_uint8(p);
				if(experimental_version == 1)
				{
					besch->base_fixed_cost = decode_uint16(p);
				}
				else if(experimental_version >= 2)
				{
					besch->base_fixed_cost = decode_uint32(p);
				}
				else
				{
					besch->base_fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
				}
				if(experimental_version >= 3)
				{
					besch->tractive_effort = decode_uint16(p);
				}
				else
				{
					besch->tractive_effort = 0;
				}

				if(experimental_version >= 4)
				{
					uint32 air_resistance_hundreds = decode_uint16(p);
					besch->air_resistance = air_resistance_hundreds / float32e8_t::hundred;
					besch->can_be_at_rear = (bool)decode_uint8(p);
					besch->increase_maintenance_after_years = decode_uint16(p);
					besch->increase_maintenance_by_percent = decode_uint16(p);
					besch->years_before_maintenance_max_reached = decode_uint8(p);
				}
				else
				{
					besch->air_resistance = vehikel_besch_t::get_air_default(besch->wt) / float32e8_t::hundred;
					besch->can_be_at_rear = true;
					besch->increase_maintenance_after_years = 0;
					besch->increase_maintenance_by_percent = 0;
					besch->years_before_maintenance_max_reached = 0;
				}
				if(experimental_version >= 5)
				{
					besch->livery_image_type = decode_uint8(p);
				}
				else
				{
					besch->livery_image_type = 0;
				}
				if(experimental_version >= 6)
				{
					// With minimum and maximum loading times in seconds
					besch->min_loading_time_seconds = decode_uint16(p);
					besch->max_loading_time_seconds = decode_uint16(p);
				}
				else
				{
					besch->min_loading_time_seconds = besch->max_loading_time_seconds = 65535;
				}
				if(experimental_version >= 7)
				{
					uint32 rolling_resistance_tenths_thousands = decode_uint16(p);
					besch->rolling_resistance = rolling_resistance_tenths_thousands / float32e8_t::tenthousand;
					besch->brake_force = decode_uint16(p);
					besch->minimum_runway_length = decode_uint16(p);
				}
				else
				{
					besch->rolling_resistance = vehikel_besch_t::get_rolling_default(besch->wt) / float32e8_t::tenthousand;
					besch->brake_force = BRAKE_FORCE_UNKNOWN;
					besch->minimum_runway_length = 10;
				}
			}
			else
			{
				dbg->fatal( "vehicle_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version );
			}
		}
	}
	else if (version==10) {
		// new: weight in kgs
		besch->base_cost = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		if(!experimental)
		{
			// The new Standard datum for loading times is read here.
			besch->min_loading_time = besch->max_loading_time = decode_uint16(p);
		}
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint32(p);
		besch->axle_load = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);
		if(!experimental)
		{
			// Experimental has this as a 32-bit integer, and reads it later.
			besch->base_fixed_cost = decode_uint16(p);
		}

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->wt = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->freight_image_type = decode_uint8(p);
		if(experimental)
		{
			if(experimental_version < 2)
			{
				// NOTE: Experimental version reset to 1 with incrementing of
				// Standard version to 10.
				besch->is_tilting = decode_uint8(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				besch->catering_level = decode_uint8(p);
				besch->bidirectional = decode_uint8(p);
				besch->can_lead_from_rear = decode_uint8(p);
				besch->comfort = decode_uint8(p);
				besch->overcrowded_capacity = decode_uint16(p);
				besch->min_loading_time = besch->max_loading_time = decode_uint16(p);
				besch->upgrades = decode_uint8(p);
				besch->base_upgrade_price = decode_uint32(p);
				besch->available_only_as_upgrade = decode_uint8(p);
				besch->base_fixed_cost = decode_uint32(p);
				besch->tractive_effort = decode_uint16(p);
				uint32 air_resistance_hundreds = decode_uint16(p);
				besch->air_resistance = air_resistance_hundreds / float32e8_t::hundred;
				besch->can_be_at_rear = (bool)decode_uint8(p);
				besch->increase_maintenance_after_years = decode_uint16(p);
				besch->increase_maintenance_by_percent = decode_uint16(p);
				besch->years_before_maintenance_max_reached = decode_uint8(p);
				besch->livery_image_type = decode_uint8(p);
				besch->min_loading_time_seconds = decode_uint16(p);
				besch->max_loading_time_seconds = decode_uint16(p);
				uint32 rolling_resistance_tenths_thousands = decode_uint16(p);
				besch->rolling_resistance = rolling_resistance_tenths_thousands / float32e8_t::tenthousand;
				besch->brake_force = decode_uint16(p);
				besch->minimum_runway_length = decode_uint16(p);
				if(experimental_version == 0)
				{
					besch->range = 0;
					besch->way_wear_factor = UINT32_MAX_VALUE; 
				}
				else
				{
					besch->range = decode_uint16(p);
					besch->way_wear_factor = decode_uint32(p);
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

		besch->wt = (sint8)v;
		besch->zuladung = decode_uint16(p);
		besch->base_cost = decode_uint32(p);
		besch->topspeed = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);
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
		if(besch->wt==4) {
			besch->engine_type = vehikel_besch_t::electric;
			besch->wt = 1;
		}
		// convert to new standard
		static const waytype_t convert_from_old[8]={road_wt, track_wt, water_wt, air_wt, invalid_wt, monorail_wt, invalid_wt, tram_wt };
		besch->wt = convert_from_old[besch->wt];
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
		besch->len = CARUNITS_PER_TILE/2;
	}

	// adjust length for different offset step sizes (which may arise in future)
	besch->len *= OBJECT_OFFSET_STEPS/CARUNITS_PER_TILE;

	// before version 8 vehicles could only have one freight image in each direction
	if(version<8) 
	{
		besch->freight_image_type = 0;
	}

	if(!experimental)
	{
		// Default values for items not in the standard vehicle format.
		besch->is_tilting = false;
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
			besch->min_loading_time = besch->max_loading_time = 2000;
			break;

		case monorail_wt:
		case maglev_wt:
		case narrowgauge_wt:
		case track_wt:
			besch->min_loading_time = besch->max_loading_time = 4000;
			break;

		case water_wt:
			besch->min_loading_time = besch->max_loading_time = 20000;
			break;

		case air_wt:
			besch->min_loading_time = besch->max_loading_time = 30000;
			break;
		}

		besch->air_resistance = vehikel_besch_t::get_air_default(besch->wt) / float32e8_t::hundred;
		besch->rolling_resistance = vehikel_besch_t::get_rolling_default(besch->wt) / float32e8_t::tenthousand;
		besch->upgrades = 0;
		besch->base_upgrade_price = besch->base_cost;
		besch->available_only_as_upgrade = false;
		besch->base_fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
		besch->can_be_at_rear = true;
		besch->increase_maintenance_after_years = 0;
		besch->increase_maintenance_by_percent = 0;
		besch->years_before_maintenance_max_reached = 0;
		besch->livery_image_type = 0;
		besch->min_loading_time_seconds = 20;
		besch->max_loading_time_seconds = 60;
		besch->brake_force = BRAKE_FORCE_UNKNOWN;
		besch->minimum_runway_length = 10;
		besch->range = 0;
		besch->way_wear_factor = UINT32_MAX_VALUE;
	}
	besch->set_way_constraints(way_constraints);

	if(version<9) {
		besch->base_fixed_cost = 0;
		besch->axle_load = besch->gewicht;
	}

	// old weights were tons
	if(version<10) {
		besch->gewicht *= 1000;
		besch->range = 0;
		besch->way_wear_factor = UINT32_MAX_VALUE;
	}

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
		besch->sound = (sint8)sound_besch_t::get_compatible_sound_id((sint8)old_id);
DBG_MESSAGE("vehicle_reader_t::register_obj()","old sound %i to %i",old_id,besch->sound);
	}
	besch->loaded();

	DBG_DEBUG("vehicle_reader_t::read_node()",
		"version=%d "
		"way=%d zuladung=%d cost=%d topspeed=%d gewicht=%g axle_load=%d leistung=%d "
		"betrieb=%d sound=%d vor=%d nach=%d "
		"date=%d/%d gear=%d engine_type=%d len=%d is_tilting=%d catering_level=%d "
		"way_constraints_permissive=%d way_constraints_prohibitive%d bidirectional%d can_lead_from_rear%d",
		version,
		besch->wt,
		besch->zuladung,
		besch->base_cost,
		besch->topspeed,
		besch->gewicht/1000.0,
		besch->axle_load,
		besch->leistung,
		besch->running_cost,
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
