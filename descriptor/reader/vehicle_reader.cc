#include <stdio.h>
#include "../../simdebug.h"
#include "../../simconst.h"
#include "../../bauer/vehikelbauer.h"
#include "../sound_desc.h"
#include "../vehicle_desc.h"
#include "../intro_dates.h"

#include "vehicle_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"



void vehicle_reader_t::register_obj(obj_desc_t *&data)
{
	vehicle_desc_t *desc = static_cast<vehicle_desc_t *>(data);
	vehicle_builder_t::register_desc(desc);
	obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool vehicle_reader_t::successfully_loaded() const
{
	return vehicle_builder_t::successfully_loaded();
}


obj_desc_t *vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	vehicle_desc_t *desc = new vehicle_desc_t();

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
		extended_version -= 1;
	}

	way_constraints_of_vehicle_t way_constraints;

	if(version == 1) {
		// Versioned node, version 1

		desc->base_cost = decode_uint32(p);
		desc->capacity = new uint16[1];
		desc->capacity[0] = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint16(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->gear = decode_uint8(p);

		desc->wtyp = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->leader_count = decode_uint8(p);
		desc->trailer_count = decode_uint8(p);

		desc->retire_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if(version == 2) {
		// Versioned node, version 2

		desc->base_cost = decode_uint32(p);
		desc->capacity = new uint16[1];
		desc->capacity[0] = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint16(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->gear = decode_uint8(p);

		desc->wtyp = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->leader_count = decode_uint8(p);
		desc->trailer_count = decode_uint8(p);
		desc->engine_type = decode_uint8(p);

		desc->retire_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if (version==3   ||  version==4  ||  version==5) {
		// Versioned node, version 3 with retire date
		// version 4 identical, just other values for the waytype
		// version 5 just uses the new scheme for data calculation

		desc->base_cost = decode_uint32(p);
		desc->capacity = new uint16[1];
		desc->capacity[0] = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint16(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->gear = decode_uint8(p);

		desc->wtyp = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->leader_count = decode_uint8(p);
		desc->trailer_count = decode_uint8(p);
		desc->engine_type = decode_uint8(p);
	}
	else if (version==6) {
		// version 5 just 32 bit for power and 16 Bit for gear

		desc->base_cost = decode_uint32(p);
		desc->capacity = new uint16[1];
		desc->capacity[0] = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint32(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wtyp = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->leader_count = decode_uint8(p);
		desc->trailer_count = decode_uint8(p);
	}
	else if (version==7) {
		// different length of cars ...

		desc->base_cost = decode_uint32(p);
		desc->capacity = new uint16[1];
		desc->capacity[0] = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint32(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wtyp = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->leader_count = decode_uint8(p);
		desc->trailer_count = decode_uint8(p);
	}
	else if (version==8) {
		// multiple freight images...

		desc->base_cost = decode_uint32(p);
		desc->capacity = new uint16[1];
		desc->capacity[0] = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint32(p);
		desc->running_cost = decode_uint16(p);

		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wtyp = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->leader_count = decode_uint8(p);
		desc->trailer_count = decode_uint8(p);
		desc->freight_image_type = decode_uint8(p);
		if(extended)
		{
			if(extended_version <= 6)
			{
				desc->classes = 1;
				desc->is_tilting = decode_uint8(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				desc->catering_level = decode_uint8(p);
				desc->bidirectional = decode_uint8(p);
				desc->can_lead_from_rear = decode_uint8(p);
				desc->comfort = new uint8[1];
				desc->comfort[0] = decode_uint8(p);
				desc->overcrowded_capacity = decode_uint16(p);
				desc->min_loading_time = desc->max_loading_time = decode_uint16(p);
				desc->upgrades = decode_uint8(p);
				desc->base_upgrade_price = decode_uint32(p);
				desc->available_only_as_upgrade = decode_uint8(p);
				desc->brake_force = BRAKE_FORCE_UNKNOWN;
				desc->minimum_runway_length = 10;
				desc->rolling_resistance = vehicle_desc_t::get_rolling_default(desc->wtyp) * float32e8_t::ten_thousandth;
				if(extended_version == 1)
				{
					desc->base_fixed_cost = decode_uint16(p);
				}
				else if(extended_version >= 2)
				{
					desc->base_fixed_cost = decode_uint32(p);
				}
				else
				{
					desc->base_fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
				}
				if(extended_version >= 3)
				{
					desc->tractive_effort = decode_uint16(p);
				}
				else
				{
					desc->tractive_effort = 0;
				}

				if(extended_version >=4)
				{
					uint32 air_resistance_hundreds = decode_uint16(p);
					desc->air_resistance = air_resistance_hundreds * float32e8_t::centi;
					desc->can_be_at_rear = (bool)decode_uint8(p);
					desc->increase_maintenance_after_years = decode_uint16(p);
					desc->increase_maintenance_by_percent = decode_uint16(p);
					desc->years_before_maintenance_max_reached = decode_uint8(p);
				}
				else
				{
					desc->air_resistance = vehicle_desc_t::get_air_default(desc->wtyp) * float32e8_t::centi;
					desc->can_be_at_rear = true;
					desc->increase_maintenance_after_years = 0;
					desc->increase_maintenance_by_percent = 0;
					desc->years_before_maintenance_max_reached = 0;
				}
				if(extended_version >= 5)
				{
					desc->livery_image_type = decode_uint8(p);
				}
				else
				{
					desc->livery_image_type = 0;
				}
				if(extended_version >= 6)
				{
					// With minimum and maximum loading times in seconds
					desc->min_loading_time_seconds = decode_uint16(p);
					desc->max_loading_time_seconds = decode_uint16(p);
				}
				else
				{
					desc->min_loading_time_seconds = desc->max_loading_time_seconds = 65535;
				}
				desc->is_tall = false;
			}
			else
			{
				dbg->fatal( "vehicle_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", extended_version );
			}
		}
	}
	else if (version==9) {
		// new: fixed_cost (previously Extended only), loading_time, axle_load
		desc->base_cost = decode_uint32(p);
		desc->capacity = new uint16[1];
		desc->capacity[0] = decode_uint16(p);
		if(extended_version == 0)
		{
			// The new Standard datum for loading times is read here.
			desc->min_loading_time = desc->max_loading_time = decode_uint16(p);
		}
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->axle_load = decode_uint16(p);
		desc->power = decode_uint32(p);
		desc->running_cost = decode_uint16(p);
		if(extended_version == 0)
		{
			// Extended has this as a 32-bit integer, and reads it later.
			desc->base_fixed_cost = decode_uint16(p);
		}

		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wtyp = decode_uint8(p);
		desc->sound = decode_sint8(p);
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->leader_count = decode_uint8(p);		
		desc->trailer_count = decode_uint8(p);		
		desc->freight_image_type = decode_uint8(p);
		if(extended)
		{
			if(extended_version <= 7)
			{
				desc->classes = 1;
				desc->is_tilting = decode_uint8(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				desc->catering_level = decode_uint8(p);
				desc->bidirectional = decode_uint8(p);
				desc->can_lead_from_rear = decode_uint8(p);
				desc->comfort = new uint8[1];
				desc->comfort[0] = decode_uint8(p);
				desc->overcrowded_capacity = decode_uint16(p);
				desc->min_loading_time = desc->max_loading_time = decode_uint16(p);
				desc->upgrades = decode_uint8(p);
				desc->base_upgrade_price = decode_uint32(p);
				desc->available_only_as_upgrade = decode_uint8(p);
				if(extended_version == 1)
				{
					desc->base_fixed_cost = decode_uint16(p);
				}
				else if(extended_version >= 2)
				{
					desc->base_fixed_cost = decode_uint32(p);
				}
				else
				{
					desc->base_fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
				}
				if(extended_version >= 3)
				{
					desc->tractive_effort = decode_uint16(p);
				}
				else
				{
					desc->tractive_effort = 0;
				}

				if(extended_version >= 4)
				{
					uint32 air_resistance_hundreds = decode_uint16(p);
					desc->air_resistance = air_resistance_hundreds * float32e8_t::centi;
					desc->can_be_at_rear = (bool)decode_uint8(p);
					desc->increase_maintenance_after_years = decode_uint16(p);
					desc->increase_maintenance_by_percent = decode_uint16(p);
					desc->years_before_maintenance_max_reached = decode_uint8(p);
				}
				else
				{
					desc->air_resistance = vehicle_desc_t::get_air_default(desc->wtyp) * float32e8_t::centi;
					desc->can_be_at_rear = true;
					desc->increase_maintenance_after_years = 0;
					desc->increase_maintenance_by_percent = 0;
					desc->years_before_maintenance_max_reached = 0;
				}
				if(extended_version >= 5)
				{
					desc->livery_image_type = decode_uint8(p);
				}
				else
				{
					desc->livery_image_type = 0;
				}
				if(extended_version >= 6)
				{
					// With minimum and maximum loading times in seconds
					desc->min_loading_time_seconds = decode_uint16(p);
					desc->max_loading_time_seconds = decode_uint16(p);
				}
				else
				{
					desc->min_loading_time_seconds = desc->max_loading_time_seconds = 65535;
				}
				if(extended_version >= 7)
				{
					uint32 rolling_resistance_tenths_thousands = decode_uint16(p);
					desc->rolling_resistance = rolling_resistance_tenths_thousands * float32e8_t::ten_thousandth;
					desc->brake_force = decode_uint16(p);
					desc->minimum_runway_length = decode_uint16(p);
				}
				else
				{
					desc->rolling_resistance = vehicle_desc_t::get_rolling_default(desc->wtyp) * float32e8_t::ten_thousandth;
					desc->brake_force = BRAKE_FORCE_UNKNOWN;
					desc->minimum_runway_length = 10;
				}
				desc->is_tall = false;
			}
			else
			{
				dbg->fatal( "vehicle_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", extended_version );
			}
		}
	}
	else if (version == 10 || version == 11) {
		// new: weight in kgs
		desc->base_cost = decode_uint32(p);

		if (extended && extended_version >= 4)
		{
			// Multiple classes, therefore multiple capacities.
			desc->classes = decode_uint8(p);
		}
		else
		{
			desc->classes = 1;
		}
	
		// Initialise the arrays
		desc->capacity = new uint16[desc->classes];
		desc->comfort = new uint8[desc->classes];

		for (uint32 i = 0; i < desc->classes; i++)
		{
			desc->capacity[i] = decode_uint16(p);
		}
		
		if (!extended)
		{
			// The new Standard datum for loading times is read here.
			desc->min_loading_time = desc->max_loading_time = decode_uint16(p);
		}
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint32(p);
		desc->axle_load = decode_uint16(p);
		desc->power = decode_uint32(p);
		desc->running_cost = decode_uint16(p);
		if (!extended)
		{
			// Extended has this as a 32-bit integer, and reads it later.
			desc->base_fixed_cost = decode_uint16(p);
		}

		desc->intro_date = decode_uint16(p);
		desc->retire_date = decode_uint16(p);
		desc->gear = decode_uint16(p);

		desc->wtyp = decode_uint8(p);
		if (extended_version >= 3)
		{
			desc->sound = decode_sint16(p);
		}
		else
		{
			desc->sound = decode_sint8(p);
		}
		desc->engine_type = decode_uint8(p);
		desc->len = decode_uint8(p);
		desc->leader_count = decode_uint8(p);
		desc->trailer_count = decode_uint8(p);
		desc->freight_image_type = decode_uint8(p);
		if(extended)
		{
			if(extended_version < 5)
			{
				// NOTE: Extended version reset to 1 with incrementing of
				// Standard version to 10.
				desc->is_tilting = decode_uint8(p);
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
				desc->catering_level = decode_uint8(p);
				desc->bidirectional = decode_uint8(p);
				desc->can_lead_from_rear = decode_uint8(p);
				for (uint32 i = 0; i < desc->classes; i++)
				{
					desc->comfort[i] = decode_uint8(p);
				}
				desc->overcrowded_capacity = decode_uint16(p);
				desc->min_loading_time = desc->max_loading_time = decode_uint16(p);
				desc->upgrades = decode_uint8(p);
				desc->base_upgrade_price = decode_uint32(p);
				desc->available_only_as_upgrade = decode_uint8(p);
				if (!extended && version == 10)
				{
					desc->base_fixed_cost = decode_uint16(p);
				}
				else
				{
					desc->base_fixed_cost = decode_uint32(p);
				}
				desc->tractive_effort = decode_uint16(p);
				uint32 air_resistance_hundreds = decode_uint16(p);
				desc->air_resistance = air_resistance_hundreds * float32e8_t::centi;
				desc->can_be_at_rear = (bool)decode_uint8(p);
				desc->increase_maintenance_after_years = decode_uint16(p);
				desc->increase_maintenance_by_percent = decode_uint16(p);
				desc->years_before_maintenance_max_reached = decode_uint8(p);
				desc->livery_image_type = decode_uint8(p);
				desc->min_loading_time_seconds = decode_uint16(p);
				desc->max_loading_time_seconds = decode_uint16(p);
				uint32 rolling_resistance_tenths_thousands = decode_uint16(p);
				desc->rolling_resistance = rolling_resistance_tenths_thousands * float32e8_t::ten_thousandth;
				desc->brake_force = decode_uint16(p);
				desc->minimum_runway_length = decode_uint16(p);
				if(extended_version == 0)
				{
					desc->range = 0;
					desc->way_wear_factor = UINT32_MAX_VALUE; 
				}
				else
				{
					desc->range = decode_uint16(p);
					desc->way_wear_factor = decode_uint32(p);
				}
				if (extended_version > 1)
				{
					desc->is_tall = decode_uint8(p);
				}
				else
				{
					desc->is_tall = false;
				}
			}
			else
			{
				dbg->fatal( "vehicle_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", extended_version );
			}
		}
	}
	else {
		if(  version!=0  ) {
			dbg->fatal( "vehicle_reader_t::read_node()","Do not know how to handle version=%i", version );
		}
		// old node, version 0

		desc->wtyp = (sint8)v;
		desc->capacity[0] = decode_uint16(p);
		desc->base_cost = decode_uint32(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint16(p);
		desc->running_cost = decode_uint16(p);
		desc->sound = decode_sint16(p);
		desc->leader_count = (sint8)decode_uint16(p);
		desc->trailer_count = (sint8)decode_uint16(p);

		desc->intro_date = DEFAULT_INTRO_DATE*16;
		desc->retire_date = (DEFAULT_RETIRE_DATE*16);
		desc->gear = 64;
	}

	// correct the engine type for old vehicles
	if(version<2) {
		// steam eangines usually have a sound of 3
		// electric engines will be overridden further down ...
		desc->engine_type = (desc->sound==3) ? vehicle_desc_t::steam : vehicle_desc_t::diesel;
	}

	//change the vehicle type
	if(version<4) {
		if(desc->wtyp==4) {
			desc->engine_type = vehicle_desc_t::electric;
			desc->wtyp = 1;
		}
		// convert to new standard
		static const waytype_t convert_from_old[8]={road_wt, track_wt, water_wt, air_wt, invalid_wt, monorail_wt, invalid_wt, tram_wt };
		desc->wtyp = convert_from_old[desc->wtyp];
	}

	// before version 5 dates were based on base 12 ...
	if(version<5) {
		uint16 date=desc->intro_date;
		desc->intro_date = (date/16)*12 + (date%16);
		date=desc->retire_date;
		desc->retire_date = (date/16)*12 + (date%16);
	}

	// before the length was always 1/8 (=half a tile)
	if(version<7) {
		desc->len = CARUNITS_PER_TILE/2;
	}

	// adjust length for different offset step sizes (which may arise in future)
	desc->len *= OBJECT_OFFSET_STEPS/CARUNITS_PER_TILE;

	// before version 8 vehicles could only have one freight image in each direction
	if(version<8) 
	{
		desc->freight_image_type = 0;
	}

	if(!extended)
	{
		// Default values for items not in the standard vehicle format.
		desc->classes = 1;
		desc->is_tilting = false;
		desc->catering_level = 0;
		desc->bidirectional = false;
		desc->can_lead_from_rear = false;
		desc->comfort = new uint8[1];
		desc->comfort[0] = 100;
		desc->overcrowded_capacity = 0;
		desc->tractive_effort = 0;

		switch(desc->get_waytype())
		{
		default:	
		case tram_wt:
		case road_wt:
			desc->min_loading_time = desc->max_loading_time = 2000;
			break;

		case monorail_wt:
		case maglev_wt:
		case narrowgauge_wt:
		case track_wt:
			desc->min_loading_time = desc->max_loading_time = 4000;
			break;

		case water_wt:
			desc->min_loading_time = desc->max_loading_time = 20000;
			break;

		case air_wt:
			desc->min_loading_time = desc->max_loading_time = 30000;
			break;
		}

		desc->air_resistance = vehicle_desc_t::get_air_default(desc->wtyp) * float32e8_t::centi;
		desc->rolling_resistance = vehicle_desc_t::get_rolling_default(desc->wtyp) * float32e8_t::ten_thousandth;
		desc->upgrades = 0;
		desc->base_upgrade_price = desc->base_cost;
		desc->available_only_as_upgrade = false;
		desc->base_fixed_cost = DEFAULT_FIXED_VEHICLE_MAINTENANCE;
		desc->can_be_at_rear = true;
		desc->increase_maintenance_after_years = 0;
		desc->increase_maintenance_by_percent = 0;
		desc->years_before_maintenance_max_reached = 0;
		desc->livery_image_type = 0;
		desc->min_loading_time_seconds = 20;
		desc->max_loading_time_seconds = 60;
		desc->brake_force = BRAKE_FORCE_UNKNOWN;
		desc->minimum_runway_length = 10;
		desc->range = 0;
		desc->way_wear_factor = 1;
		desc->is_tall = false;
	}
	desc->set_way_constraints(way_constraints);

	if(version<9) {
		desc->base_fixed_cost = 0;
		desc->axle_load = desc->weight;
	}

	// old weights were tons
	if(version<10) {
		desc->weight *= 1000;
		desc->range = 0;
		desc->way_wear_factor = UINT32_MAX_VALUE;
	}

	if(desc->sound==LOAD_SOUND) {
		uint8 len=decode_sint8(p);
		char wavname[256];
		wavname[len] = 0;
		for(uint8 i=0; i<len; i++) {
			wavname[i] = decode_sint8(p);
		}
		desc->sound = (sint16)sound_desc_t::get_sound_id(wavname);
DBG_MESSAGE("vehicle_reader_t::register_obj()","sound %s to %i",wavname,desc->sound);
	}
	else if(desc->sound>=0  &&  desc->sound<=MAX_OLD_SOUNDS) {
		sint16 old_id = desc->sound;
		desc->sound = (sint16)sound_desc_t::get_compatible_sound_id(old_id);
DBG_MESSAGE("vehicle_reader_t::register_obj()","old sound %i to %i",old_id,desc->sound);
	}
	desc->loaded();

	DBG_DEBUG("vehicle_reader_t::read_node()",
		"version=%d "
		"way=%d classes=%d capacity=%d comfort=%d cost=%d topspeed=%d weight=%g axle_load=%d power=%d "
		"betrieb=%d sound=%d vor=%d nach=%d "
		"date=%d/%d gear=%d engine_type=%d len=%d is_tilting=%d catering_level=%d "
		"way_constraints_permissive=%d way_constraints_prohibitive%d bidirectional%d can_lead_from_rear%d",
		version,
		desc->wtyp,
		desc->classes,
		desc->capacity[0],
		desc->comfort[0],
		desc->base_cost,
		desc->topspeed,
		desc->weight/1000.0,
		desc->axle_load,
		desc->power,
		desc->running_cost,
		desc->sound,
		desc->leader_count,
		desc->trailer_count,
		(desc->intro_date%12)+1,
		desc->intro_date/12,
		desc->gear,
		desc->engine_type,
		desc->len,
		desc->is_tilting,
		desc->catering_level,
		desc->get_way_constraints().get_permissive(),
		desc->get_way_constraints().get_prohibitive(),
		desc->bidirectional,
		desc->can_lead_from_rear);

	return desc;
}
