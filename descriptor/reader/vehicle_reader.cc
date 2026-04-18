/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../objversion.h"
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
#include "../../tpl/array_tpl.h"


void vehicle_reader_t::register_obj(obj_desc_t *&data)
{
	vehicle_desc_t *desc = static_cast<vehicle_desc_t *>(data);
	vehicle_builder_t::register_desc(desc);
	obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool vehicle_reader_t::successfully_loaded() const
{
	return vehicle_builder_t::successfully_loaded();
}


obj_desc_t *vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;
	const bool extended = version > 0 ? (v & EX_VER) != 0 : false;
	uint16 extended_version = 0;
	if (extended) {
		version = version & EX_VER ? version & 0x3FFF : 0;
		while (version > 0x100) { version -= 0x100; extended_version++; }
		extended_version--;
	}

	vehicle_desc_t *desc = new vehicle_desc_t();

	if(version == 1) {
		// Versioned node, version 1

		desc->price = decode_uint32(p);
		desc->capacity = decode_uint16(p);
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

		desc->price = decode_uint32(p);
		desc->capacity = decode_uint16(p);
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

		desc->price = decode_uint32(p);
		desc->capacity = decode_uint16(p);
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

		desc->price = decode_uint32(p);
		desc->capacity = decode_uint16(p);
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

		desc->price = decode_uint32(p);
		desc->capacity = decode_uint16(p);
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
		// multiple freight images; Extended v8 adds optional trailing block
		desc->price = decode_uint32(p);
		desc->capacity = decode_uint16(p);
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
		if (extended) {
			// skip Extended-specific trailing block (is_tilting, way_constraints, catering, etc.)
			if (extended_version > 6) {
				dbg->fatal("vehicle_reader_t::read_node()", "Incompatible Extended pak version %i", extended_version);
			}
			decode_uint8(p); // is_tilting
			decode_uint8(p); // way_constraints permissive
			decode_uint8(p); // way_constraints prohibitive
			decode_uint8(p); // catering_level
			decode_uint8(p); // bidirectional
			decode_uint8(p); // can_lead_from_rear
			decode_uint8(p); // comfort[0]
			decode_uint16(p); // overcrowded_capacity
			decode_uint16(p); // min/max_loading_time
			decode_uint8(p);  // upgrades
			decode_uint32(p); // base_upgrade_price
			decode_uint8(p);  // available_only_as_upgrade
			if (extended_version == 1) {
				decode_uint16(p); // base_fixed_cost
			} else if (extended_version >= 2) {
				decode_uint32(p); // base_fixed_cost
			}
			if (extended_version >= 3) { decode_uint16(p); } // tractive_effort
			if (extended_version >= 4) {
				decode_uint16(p); // air_resistance_hundreds
				decode_uint8(p);  // can_be_at_rear
				decode_uint16(p); // increase_maintenance_after_years
				decode_uint16(p); // increase_maintenance_by_percent
				decode_uint8(p);  // years_before_maintenance_max_reached
			}
			if (extended_version >= 5) { desc->livery_image_type = decode_uint8(p); } // livery_image_type
			if (extended_version >= 6) { decode_uint16(p); decode_uint16(p); } // min/max_loading_time_seconds
		}
	}
	else if (version==9) {
		// Extended v9 has different field layout than OTRP v9:
		// Extended v9 omits loading_time and base_fixed_cost in the standard positions
		if (extended) {
			desc->price = decode_uint32(p);
			desc->capacity = decode_uint16(p);
			// Extended v9: no loading_time here (it's in Extended block)
			desc->topspeed = decode_uint16(p);
			desc->weight = decode_uint16(p);
			desc->axle_load = decode_uint16(p);
			desc->power = decode_uint32(p);
			desc->running_cost = decode_uint16(p);
			// Extended v9: no base_fixed_cost here (it's in Extended block)
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
			// skip Extended trailing block
			if (extended_version > 7) {
				dbg->fatal("vehicle_reader_t::read_node()", "Incompatible Extended pak version %i", extended_version);
			}
			decode_uint8(p);  // is_tilting
			decode_uint8(p);  // permissive
			decode_uint8(p);  // prohibitive
			decode_uint8(p);  // catering_level
			decode_uint8(p);  // bidirectional
			decode_uint8(p);  // can_lead_from_rear
			decode_uint8(p);  // comfort[0]
			decode_uint16(p); // overcrowded_capacity
			decode_uint16(p); // min/max_loading_time -> use as loading_time
			desc->loading_time = 0; // already consumed above as dummy
			decode_uint8(p);  // upgrades
			decode_uint32(p); // base_upgrade_price
			decode_uint8(p);  // available_only_as_upgrade
			if (extended_version == 1) {
				decode_uint16(p);
			} else if (extended_version >= 2) {
				decode_uint32(p); // base_fixed_cost
			}
			if (extended_version >= 3) { decode_uint16(p); } // tractive_effort
			if (extended_version >= 4) {
				decode_uint16(p); // air_resistance_hundreds
				decode_uint8(p);  // can_be_at_rear
				decode_uint16(p); // increase_maintenance_after_years
				decode_uint16(p); // increase_maintenance_by_percent
				decode_uint8(p);  // years_before_maintenance_max_reached
			}
			if (extended_version >= 5) { desc->livery_image_type = decode_uint8(p); } // livery_image_type
			if (extended_version >= 6) { decode_uint16(p); decode_uint16(p); } // loading_time_seconds
			if (extended_version >= 7) {
				decode_uint16(p); // rolling_resistance
				decode_uint16(p); // brake_force
				decode_uint16(p); // minimum_runway_length
			}
		}
		else {
			// OTRP v9: new: fixed_cost, loading_time, axle_load
			desc->price = decode_uint32(p);
			desc->capacity = decode_uint16(p);
			desc->loading_time = decode_uint16(p);
			desc->topspeed = decode_uint16(p);
			desc->weight = decode_uint16(p);
			desc->axle_load = decode_uint16(p);
			desc->power = decode_uint32(p);
			desc->running_cost = decode_uint16(p);
			desc->maintenance = decode_uint16(p);
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
		}
	}
	else if (version==10 || version==11) {
		// Extended v10/11 have different layout from OTRP v10/11
		if (extended) {
			desc->price = decode_uint32(p);
			// Extended v10/11 extended_version >= 4: has classes byte
			uint8 classes = 1;
			if (extended_version >= 4) {
				classes = decode_uint8(p);
			}
			// Read first class capacity, skip rest
			desc->capacity = decode_uint16(p);
			for (uint8 i = 1; i < classes; i++) { decode_uint16(p); }
			// Extended: no loading_time in standard position
			desc->topspeed = decode_uint16(p);
			desc->weight = decode_uint32(p); // already kg
			desc->axle_load = decode_uint16(p);
			desc->power = decode_uint32(p);
			desc->running_cost = decode_uint16(p);
			// Extended: no base_fixed_cost in standard position
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->gear = decode_uint16(p);
			desc->wtyp = decode_uint8(p);
			if (extended_version >= 3) {
				desc->sound = decode_sint16(p);
			} else {
				desc->sound = decode_sint8(p);
			}
			desc->engine_type = decode_uint8(p);
			desc->len = decode_uint8(p);
			desc->leader_count = decode_uint8(p);
			desc->trailer_count = decode_uint8(p);
			desc->freight_image_type = decode_uint8(p);
			// skip Extended trailing block
			if (extended_version >= 8) {
				dbg->fatal("vehicle_reader_t::read_node()", "Incompatible Extended pak version %i", extended_version);
			}
			decode_uint8(p);  // is_tilting
			decode_uint8(p);  // permissive
			decode_uint8(p);  // prohibitive
			decode_uint8(p);  // catering_level
			decode_uint8(p);  // bidirectional
			if (extended_version >= 5) {
				decode_uint8(p); // basic_constraint_prev
			} else {
				decode_uint8(p); // can_lead_from_rear
			}
			for (uint8 i = 0; i < classes; i++) { decode_uint8(p); } // comfort[i]
			decode_uint16(p); // overcrowded_capacity
			decode_uint16(p); // min/max_loading_time
			decode_uint8(p);  // upgrades
			decode_uint32(p); // base_upgrade_price
			decode_uint8(p);  // available_only_as_upgrade
			decode_uint32(p); // base_fixed_cost
			decode_uint16(p); // tractive_effort
			decode_uint16(p); // air_resistance_hundreds
			if (extended_version >= 5) {
				decode_uint8(p); // basic_constraint_next
			} else {
				decode_uint8(p); // can_be_at_rear
			}
			decode_uint16(p); // increase_maintenance_after_years
			decode_uint16(p); // increase_maintenance_by_percent
			decode_uint8(p);  // years_before_maintenance_max_reached
			desc->livery_image_type = decode_uint8(p); // livery_image_type
			decode_uint16(p); // min_loading_time_seconds
			decode_uint16(p); // max_loading_time_seconds
			decode_uint16(p); // rolling_resistance
			decode_uint16(p); // brake_force
			decode_uint16(p); // minimum_runway_length
			if (extended_version >= 1) {
				decode_uint16(p); // range
				decode_uint32(p); // way_wear_factor
			}
			if (extended_version >= 2) { decode_uint8(p); } // is_tall
			if (extended_version >= 5) { decode_uint8(p); } // mixed_load_prohibition
			if (extended_version >= 6) { decode_uint8(p); } // override_way_speed
		}
		else if (version == 10) {
			// OTRP v10: weight in kgs
			desc->price = decode_uint32(p);
			desc->capacity = decode_uint16(p);
			desc->loading_time = decode_uint16(p);
			desc->topspeed = decode_uint16(p);
			desc->weight = decode_uint32(p);
			desc->axle_load = decode_uint16(p);
			desc->power = decode_uint32(p);
			desc->running_cost = decode_uint16(p);
			desc->maintenance = decode_uint16(p);
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
		}
		else { // version == 11
			// OTRP v11: fix cost as uint32
			desc->price = decode_uint32(p);
			desc->capacity = decode_uint16(p);
			desc->loading_time = decode_uint16(p);
			desc->topspeed = decode_uint16(p);
			desc->weight = decode_uint32(p);
			desc->axle_load = decode_uint16(p);
			desc->power = decode_uint32(p);
			desc->running_cost = decode_uint16(p);
			desc->maintenance = decode_uint32(p);
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
		}
	}
	else if (version == 12) {
		// Cost values as sint64
		desc->price = decode_sint64(p);
		desc->capacity = decode_uint16(p);
		desc->loading_time = decode_uint16(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint32(p);
		desc->axle_load = decode_uint16(p);
		desc->power = decode_uint32(p);
		desc->running_cost = decode_sint64(p);
		desc->maintenance = decode_sint64(p);

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
	}
	else if (version == 13) {
		// loading time as uint32
		desc->price = decode_sint64(p);
		desc->capacity = decode_uint16(p);
		desc->loading_time = decode_uint32(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint32(p);
		desc->axle_load = decode_uint16(p);
		desc->power = decode_uint32(p);
		desc->running_cost = decode_sint64(p);
		desc->maintenance = decode_sint64(p);

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
	}
	else {
		if( version ) {
			dbg->fatal( "vehicle_reader_t::read_node()", "Cannot handle too new node version %i", version );
		}

		// old node, version 0
		desc->wtyp = (sint8)v;
		desc->capacity = decode_uint16(p);
		desc->price = decode_uint32(p);
		desc->topspeed = decode_uint16(p);
		desc->weight = decode_uint16(p);
		desc->power = decode_uint16(p);
		desc->running_cost = decode_uint16(p);
		desc->sound = (sint8)decode_sint16(p);
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
	if(version<8) {
		desc->freight_image_type=0;
	}

	// non-extended vehicles never have livery images
	if(!extended) {
		desc->livery_image_type = 0;
	}

	if (version < 9 && !extended) {
		desc->maintenance = 0;
		desc->axle_load = 0;
		desc->loading_time = 1000;
	}
	else if (extended && version < 9) {
		desc->maintenance = 0;
		desc->axle_load = desc->weight;
		desc->loading_time = 1000;
	}

	// old weights were tons (OTRP v<10, or Extended v<=9)
	if (version < 10 && !extended) {
		desc->weight *= 1000;
	}
	else if (extended && version <= 9) {
		desc->weight *= 1000;
	}

	if(desc->sound==LOAD_SOUND) {
		uint8 len=decode_sint8(p);
		char wavname[256];
		wavname[len] = 0;
		for(uint8 i=0; i<len; i++) {
			wavname[i] = decode_sint8(p);
		}
		desc->sound = (sint8)sound_desc_t::get_sound_id(wavname);
DBG_MESSAGE("vehicle_reader_t::register_obj()","sound %s to %i",wavname,desc->sound);
	}
	else if(desc->sound>=0  &&  desc->sound<=MAX_OLD_SOUNDS) {
		sint16 old_id = desc->sound;
		desc->sound = (sint8)sound_desc_t::get_compatible_sound_id((sint8)old_id);
DBG_MESSAGE("vehicle_reader_t::register_obj()","old sound %i to %i",old_id,desc->sound);
	}

	DBG_DEBUG("vehicle_reader_t::read_node()",
		"version=%d "
		"way=%d capacity=%d price=%d topspeed=%d weight=%g axle_load=%d power=%d "
		"running_cost=%d fixed_cost=%d sound=%d prev=%d next=%d loading_time=%d"
		"date=%d/%d retire=%d/%d gear=%d engine_type=%d freigh_imgs=%d len=%d",
		version,
		desc->wtyp,
		desc->capacity,
		desc->price,
		desc->topspeed,
		desc->weight/1000.0,
		desc->axle_load,
		desc->power,
		desc->running_cost,
		desc->maintenance,
		desc->sound,
		desc->leader_count,
		desc->trailer_count,
		desc->loading_time,
		(desc->intro_date%12)+1,
		desc->intro_date/12,
		(desc->retire_date%12)+1,
		desc->retire_date/12,
		desc->gear,
		desc->engine_type,
		desc->freight_image_type,
		desc->len);

	return desc;
}
