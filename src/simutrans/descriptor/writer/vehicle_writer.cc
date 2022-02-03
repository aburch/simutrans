/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdlib.h>
#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "../vehicle_desc.h"
#include "../sound_desc.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"
#include "get_waytype.h"
#include "vehicle_writer.h"

using std::string;

/**
 * Calculate numeric engine type from engine type string
 */
static vehicle_desc_t::engine_t get_engine_type(char const* const engine_type)
{
	vehicle_desc_t::engine_t uv8 = vehicle_desc_t::diesel;

	if (!STRICMP(engine_type, "diesel")) {
		uv8 = vehicle_desc_t::diesel;
	} else if (!STRICMP(engine_type, "electric")) {
		uv8 = vehicle_desc_t::electric;
	} else if (!STRICMP(engine_type, "steam")) {
		uv8 = vehicle_desc_t::steam;
	} else if (!STRICMP(engine_type, "bio")) {
		uv8 = vehicle_desc_t::bio;
	} else if (!STRICMP(engine_type, "sail")) {
		uv8 = vehicle_desc_t::sail;
	} else if (!STRICMP(engine_type, "fuel_cell")) {
		uv8 = vehicle_desc_t::fuel_cell;
	} else if (!STRICMP(engine_type, "hydrogene")) {
		uv8 = vehicle_desc_t::hydrogene;
	} else if (!STRICMP(engine_type, "battery")) {
		uv8 = vehicle_desc_t::battery;
	} else if (!STRICMP(engine_type, "unknown")) {
		uv8 = vehicle_desc_t::unknown;
	}

	// printf("Engine type %s -> %d\n", engine_type, uv8);

	return uv8;
}


/**
 * Writes vehicle node data to file
 */
void vehicle_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int i;
	uint8  uv8;

	int total_len = 41;

	// must be done here, since it may affect the len of the header!
	string sound_str = ltrim( obj.get("sound") );
	sint8 sound_id=NO_SOUND;
	if (!sound_str.empty()) {
		// ok, there is some sound
		sound_id = atoi(sound_str.c_str());
		if (sound_id == 0 && sound_str[0] == '0') {
			sound_id = 0;
			sound_str = "";
		}
		else if (sound_id != 0) {
			// old style id
			sound_str = "";
		}
		if (!sound_str.empty()) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.size() + 1;
		}
	}

	obj_node_t node(this, total_len, &parent);

	write_head(fp, node, obj);
	uint16 pos = 0;

	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversioned
	uint16 version = 0x800B;
	node.write_uint16(fp, version, pos);
	pos += sizeof(uint16);

	// Price of this vehicle in cent
	uint32 price = obj.get_int("cost", 0);
	node.write_uint32(fp, price, pos);
	pos += sizeof(uint32);


	// Maximum payload of this vehicle
	uint16 capacity = obj.get_int("payload", 0);
	node.write_uint16(fp, capacity, pos);
	pos += sizeof(uint16);

	// ms per loading/unloading everything
	uint16 loading_time = obj.get_int("loading_time", 1000 );
	node.write_uint16(fp, loading_time, pos);
	pos += sizeof(uint16);

	// Top speed of this vehicle. Must be greater than 0
	uint16 topspeed = obj.get_int("speed", 0);
	node.write_uint16(fp, topspeed, pos);
	pos += sizeof(uint16);

	// Total weight of this vehicle in tons
	const char *weight_str = obj.get("weight");
	uint32 weight = (uint32)(atof( weight_str )*1000.0 + 0.5);
	node.write_uint32(fp, weight, pos);
	pos += sizeof(uint32);

	// axle_load (determine ways usage)
	uint16 axle_load = obj.get_int("axle_load", 0);
	node.write_uint16(fp, axle_load, pos);
	pos += sizeof(uint16);

	// Power of this vehicle in KW
	uint32 power = obj.get_int("power", 0);
	node.write_uint32(fp, power, pos);
	pos += sizeof(uint32);

	// Running costs, given in cent per square
	uint16 running_cost = obj.get_int("runningcost", 0);
	node.write_uint16(fp, running_cost, pos);
	pos += sizeof(uint16);

	// monthly maintenance
	uint32 fixed_cost = obj.get_int("fixed_cost", 0xFFFFFFFFul );
	if(  fixed_cost == 0xFFFFFFFFul  ) {
		fixed_cost = obj.get_int("maintenance", 0);
	}
	node.write_uint32(fp, fixed_cost, pos);
	pos += sizeof(uint32);

	// Introduction date (year * 12 + month)
	uint16 intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;
	node.write_uint16(fp, intro_date, pos);
	pos += sizeof(uint16);

	// retire date (year * 12 + month)
	uint16 retire_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;
	node.write_uint16(fp, retire_date, pos);
	pos += sizeof(uint16);

	// Engine gear (power multiplier)
	uint16 gear = (obj.get_int("gear", 100) * 64) / 100;
	node.write_uint16(fp, gear, pos);
	pos += sizeof(uint16);

	// Type of way this vehicle drives on
	char const* const waytype_name = obj.get("waytype");
	waytype_t   const waytype      = get_waytype(waytype_name);
	uv8 = waytype != overheadlines_wt ? waytype : track_wt;
	node.write_uint8(fp, uv8, pos);
	pos += sizeof(uint8);

	// sound id byte
	node.write_sint8(fp, sound_id, pos);
	pos += sizeof(uint8);

	// engine
	if (waytype == overheadlines_wt) {
		// compatibility for old style DAT files
		uv8 = vehicle_desc_t::electric;
	}
	else {
		const char* engine_type = obj.get("engine_type");
		uv8 = get_engine_type(engine_type);
	}
	node.write_uint8(fp, uv8, pos);
	pos += sizeof(uint8);

	// the length (default 8)
	uint8 length = obj.get_int("length", 8);
	node.write_uint8(fp, length, pos);
	pos += sizeof(uint8);

	// The freight type
	const char* freight = obj.get("freight");
	if (!*freight) {
		freight = "None";
	}
	xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, true);
	xref_writer_t::instance()->write_obj(fp, node, obj_smoke, obj.get("smoke"), false);

	// Now comes the Image-list
	static const char* const dir_codes[] = {
		"s", "w", "sw", "se", "n", "e", "ne", "nw"
	};
	slist_tpl<string> emptykeys;
	slist_tpl<slist_tpl<string> > freightkeys;
	slist_tpl<string> freightkeys_old;
	string str;

	int  freight_image_type  = 0;
	bool has_8_images = false;

	// first: find out how many freight?
	for (i = 0; i < 127; i++) {
		char buf[40];
		sprintf(buf, "freightimage[%d][%s]", i, dir_codes[0]);
		str = obj.get(buf);
		if (str.empty()) {
			freight_image_type = i;
			break;
		}
	}

	// now load the images strings
	for (i = 0; i < 8; i++) {
		char buf[40];

		// Empty vehicle image for direction, direction in "s", "w", "sw", "se", unsymmetrical vehicles need also "n", "e", "ne", "nw"
		sprintf(buf, "emptyimage[%s]", dir_codes[i]);
		str = obj.get(buf);
		if (!str.empty()) {
			emptykeys.append(str);
			if (i >= 4) {
				has_8_images = true;
			}
		}
		else {
			// stop when empty string is found
			break;
		}

		if (freight_image_type == 0) {
			// a single freight image
			// old style definition - just [direction]
			sprintf(buf, "freightimage[%s]", dir_codes[i]);
			str = obj.get(buf);
			if (!str.empty()) {
				freightkeys_old.append(str);
			}
		}
		else {
			freightkeys.append();
			for(int freight = 0; freight < freight_image_type; freight++) {
				sprintf(buf, "freightimage[%d][%s]", freight, dir_codes[i]);
				str = obj.get(buf);
				if (str.empty()) {
					dbg->fatal( "Vehicle", "Missing freightimage[%d][%s]!", freight, dir_codes[i]);
				}
				freightkeys.at(i).append(str);
			}
		}
	}

	// added more error checks
	if (has_8_images && emptykeys.get_count() < 8) {
		dbg->fatal( "Vehicle", "Missing images (must be either 4 or 8 directions (but %i found)!)", emptykeys.get_count());
	}
	if (!freightkeys_old.empty() && emptykeys.get_count() != freightkeys_old.get_count()) {
		dbg->fatal( "Vehicle", "Missing freigthimages (must be either 4 or 8 directions (but %i found)!)", freightkeys_old.get_count());
	}

	imagelist_writer_t::instance()->write_obj(fp, node, emptykeys);
	if (freight_image_type > 0) {
		imagelist2d_writer_t::instance()->write_obj(fp, node, freightkeys);
	}
	else {
		if (freightkeys_old.get_count() == emptykeys.get_count()) {
			imagelist_writer_t::instance()->write_obj(fp, node, freightkeys_old);
		}
		else {
			// really empty list ...
			xref_writer_t::instance()->write_obj(fp, node, obj_imagelist, "", false);
		}
	}

	//
	// following/leader vehicle constrains
	//
	uint8 leader_count = 0;
	bool found;
	do {
		char buf[40];

		// Constraints for previous vehicles, "none" means only suitable at front of an convoi
		sprintf(buf, "constraint[prev][%d]", leader_count);

		str = obj.get(buf);
		found = !str.empty();
		if (found) {
			if (!STRICMP(str.c_str(), "none")) {
				str = "";
			}
			xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
			leader_count++;
		}
	} while (found);

	uint8 trailer_count = 0;
	do {
		char buf[40];

		// Constraints for next vehicle, "none" to disallow any followers
		sprintf(buf, "constraint[next][%d]", trailer_count);

		str = obj.get(buf);
		found = !str.empty();
		if (found) {
			if (!STRICMP(str.c_str(), "none")) {
				str = "";
			}
			xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.c_str(), false);
			trailer_count++;
		}
	} while (found);

	// multiple freight image types - define what good uses each index
	// good without index will be an error
	for (i = 0; i <= freight_image_type; i++) {
		char buf[40];
		sprintf(buf, "freightimagetype[%d]", i);
		str = obj.get(buf);
		if (i == freight_image_type) {
			// check for superfluous definitions
			if (str.size() > 0) {
				dbg->warning( obj_writer_t::last_name, "More freightimagetype (%i) than freight_images (%i)!", i, freight_image_type);
				fflush(NULL);
			}
			break;
		}
		if (str.size() == 0) {
			dbg->fatal( obj_writer_t::last_name, "Missing freightimagetype[%i] for %i freight_images!", i, freight_image_type + 1);
		}
		xref_writer_t::instance()->write_obj(fp, node, obj_good, str.c_str(), false);
	}

	// if no index defined then add default as vehicle good
	// if not using freight images then store zero string
	if (freight_image_type > 0) {
		xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, false);
	}

	node.write_sint8(fp, leader_count, pos);
	pos += sizeof(uint8);
	node.write_sint8(fp, trailer_count, pos);
	pos += sizeof(uint8);
	node.write_uint8(fp, (uint8) freight_image_type, pos);
	pos += sizeof(uint8);

	sint8 sound_str_len = sound_str.size();
	if (sound_str_len > 0) {
		node.write_sint8  (fp, sound_str_len, pos);
		pos += sizeof(uint8);
		node.write_data_at(fp, sound_str.c_str(), pos, sound_str_len);
		pos += sound_str_len;
	}

	node.write(fp);
}
