#include <cmath>
#include "../../utils/simstring.h"
#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../vehikel_besch.h"
#include "../sound_besch.h"
#include "../../boden/wege/weg.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"
#include "get_waytype.h"
#include "vehicle_writer.h"


/**
 * Calculate numeric engine type from engine type string
 */
static uint8 get_engine_type(const char* engine_type, tabfileobj_t& obj)
{
	uint8 uv8 = vehikel_besch_t::diesel;

	if (!STRICMP(engine_type, "diesel")) {
		uv8 = vehikel_besch_t::diesel;
	} else if (!STRICMP(engine_type, "electric")) {
		uv8 = vehikel_besch_t::electric;
	} else if (!STRICMP(engine_type, "steam")) {
		uv8 = vehikel_besch_t::steam;
	} else if (!STRICMP(engine_type, "bio")) {
		uv8 = vehikel_besch_t::bio;
	} else if (!STRICMP(engine_type, "sail")) {
		uv8 = vehikel_besch_t::sail;
	} else if (!STRICMP(engine_type, "fuel_cell")) {
		uv8 = vehikel_besch_t::fuel_cell;
	} else if (!STRICMP(engine_type, "hydrogene")) {
		uv8 = vehikel_besch_t::hydrogene;
	} else if (!STRICMP(engine_type, "battery")) {
		uv8 = vehikel_besch_t::battery;
	} else if (!STRICMP(engine_type, "unknown")) {
		uv8 = vehikel_besch_t::unknown;
	}

	// printf("Engine type %s -> %d\n", engine_type, uv8);

	return uv8;
}


/**
 * Writes vehicle node data to file
 *
 * NOTE: The data must be written in _exactly_
 * the same sequence as it is to be read in the
 * relevant reader file. The "total_len" field is
 * the length in bytes of the VHCL node of the
 * pak file. The VHCL node is the first node
 * beneath the header node, and contains all of
 * the _numerical_ information about the vehicle,
 * such as the introduction date, running costs,
 * etc.. Text (including filenames of sound files),
 * and graphics are _not_ part of the VHCL node,
 * and therefore do not count towards total length.
 * Furthermore, the third argument to the node.write
 * method must ascend sequentially with the number 
 * of bytes written so far (up 1 for a uint8, 2 for
 * a uint16, 4 for a uint32 and so forth). Failure
 * to observe these rules will result in data
 * corruption and errors when the pak file is read
 * by the main program.
 * @author of note: jamespetts
 */
void vehicle_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int i;
	uint8  uv8;

	int total_len = 35;

	// prissi: must be done here, since it may affect the length of the header!
	cstring_t sound_str = ltrim( obj.get("sound") );
	sint8 sound_id=NO_SOUND;
	if (sound_str.len() > 0) {
		// ok, there is some sound
		sound_id = atoi(sound_str);
		if (sound_id == 0 && sound_str[0] == '0') {
			sound_id = 0;
			sound_str = "";
		} else if (sound_id != 0) {
			// old style id
			sound_str = "";
		}
		if (sound_str.len() > 0) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.len() + 1;
		}
	}

	obj_node_t	node(this, total_len, &parent);

	write_head(fp, node, obj);


	// Hajo: version number
	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8008;
	node.write_uint16(fp, version, 0);


	// Hajodoc: Price of this vehicle in cent
	// Hajoval: int
	uint32 cost = obj.get_int("cost", 0);
	node.write_uint32(fp, cost, 2);


	// Hajodoc: Payload of this vehicle
	// Hajoval: int
	uint16 payload = obj.get_int("payload", 0);
	node.write_uint16(fp, payload, 6);


	// Hajodoc: Top speed of this vehicle. Must be greater than 0
	// Hajoval: int
	uint16 top_speed = obj.get_int("speed", 0);
	node.write_uint16(fp, top_speed, 8);


	// Hajodoc: Total weight of this vehicle in tons
	// Hajoval: int
	uint16 weight = obj.get_int("weight", 0);
	node.write_uint16(fp, weight, 10);


	// Hajodoc: Power of this vehicle in KW
	// Hajoval: int
	uint32 power = obj.get_int("power", 0);
	node.write_uint32(fp, power, 12);


	// Hajodoc: Running costs, given in cent per square
	// Hajoval: int
	uint16 runningcost = obj.get_int("runningcost", 0);
	node.write_uint16(fp, runningcost, 16);


	// Hajodoc: Introduction date (year * 12 + month)
	// Hajoval: int
	uint16 intro  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro += obj.get_int("intro_month", 1) - 1;
	node.write_uint16(fp, intro, 18);

	// Hajodoc: retire date (year * 12 + month)
	// Hajoval: int
	uint16 retire = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire += obj.get_int("retire_month", 1) - 1;
	node.write_uint16(fp, retire, 20);

	// Hajodoc: Engine gear (power multiplier)
	// Hajoval: int
	uint16 gear = (obj.get_int("gear", 100) * 64) / 100;
	node.write_uint16(fp, gear, 22);

	// Hajodoc: Type of way this vehicle drives on
	// Hajoval: road, track, electrified_track, monorail_track, maglev_track, water
	const char* waytype = obj.get("waytype");
	const char waytype_uint = get_waytype(waytype);
	uv8 = (waytype_uint == overheadlines_wt ? track_wt : waytype_uint);
	node.write_uint8 (fp, uv8, 24);

	// Hajodoc: The freight type
	// Hajoval: string
	const char* freight = obj.get("freight");
	if (!*freight) {
		freight = "None";
	}
	xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, true);
	xref_writer_t::instance()->write_obj(fp, node, obj_smoke, obj.get("smoke"), false);

	// Jetzt kommen die Bildlisten
	// "Now the picture lists" (Google)
	static const char* const dir_codes[] = {
		"s", "w", "sw", "se", "n", "e", "ne", "nw"
	};
	slist_tpl<cstring_t> emptykeys;
	slist_tpl<slist_tpl<cstring_t> > freightkeys;
	slist_tpl<cstring_t> freightkeys_old;
	cstring_t str;

	int  freight_max  = 0;
	bool has_8_images = false;

	// first: find out how many freight?
	for (i = 0; i < 127; i++) {
		char buf[40];
		sprintf(buf, "freightimage[%d][%s]", i, dir_codes[0]);
		str = obj.get(buf);
		if (str.len() == 0) {
			freight_max = i;
			break;
		}
	}

	// now load the images strings
	for (i = 0; i < 8; i++) {
		char buf[40];

		// Hajodoc: Empty vehicle image for direction, direction in "s", "w", "sw", "se", asymmetric vehicles need also "n", "e", "ne", "nw"
		sprintf(buf, "emptyimage[%s]", dir_codes[i]);
		str = obj.get(buf);
		if (str.len() > 0) {
			emptykeys.append(str);
			if (i >= 4) {
				has_8_images = true;
			}
		} else {
			// stop when empty string is found
			break;
		}

		if (freight_max == 0) {
			// a single freight image
			// old style definition - just [direction]
			sprintf(buf, "freightimage[%s]", dir_codes[i]);
			str = obj.get(buf);
			if (str.len() > 0) {
				freightkeys_old.append(str);
			}
		} else {
			freightkeys.append(slist_tpl<cstring_t>());
			for(int freight = 0; freight < freight_max; freight++) {
				sprintf(buf, "freightimage[%d][%s]", freight, dir_codes[i]);
				str = obj.get(buf);
				if (str.len() == 0) {
					printf("*** FATAL ***:\nMissing freightimage[%d][%s]!\n", freight, dir_codes[i]);
					fflush(NULL);
					exit(0);
				}
				freightkeys.at(i).append(str);
			}
		}
	}

	// prissi: added more error checks
	if (has_8_images && emptykeys.get_count() < 8) {
		printf("*** FATAL ***:\nMissing images (must be either 4 or 8 directions (but %i found)!)\n", emptykeys.get_count());
		exit(0);
	}
	if (!freightkeys_old.empty() && emptykeys.get_count() != freightkeys_old.get_count()) {
		printf("*** FATAL ***:\nMissing freigthimages (must be either 4 or 8 directions (but %i found)!)\n", freightkeys_old.get_count());
		exit(0);
	}

	imagelist_writer_t::instance()->write_obj(fp, node, emptykeys);
	if (freight_max > 0) {
		imagelist2d_writer_t::instance()->write_obj(fp, node, freightkeys);
	} else {
		if (freightkeys_old.get_count() == emptykeys.get_count()) {
			imagelist_writer_t::instance()->write_obj(fp, node, freightkeys_old);
		} else {
			// really empty list ...
			xref_writer_t::instance()->write_obj(fp, node, obj_imagelist, "", false);
		}
	}

	//
	// Vorgänger/Nachfolgerbedingungen
	// "Predecessor / Successor conditions" (Google)
	//
	uint8 besch_vorgaenger = 0;
	do {
		char buf[40];

		// Hajodoc: Constraints for previous vehicles
		// Hajoval: string, "none" means only suitable at front of an convoi
		sprintf(buf, "constraint[prev][%d]", besch_vorgaenger);

		str = obj.get(buf);
		if (str.len() > 0) {
			if (besch_vorgaenger == 0 && !STRICMP(str, "none")) {
				str = "";
			}
			xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str, false);
			besch_vorgaenger++;
		}
	} while (str.len() > 0);

	uint8 besch_nachfolger = 0;
	do {
		char buf[40];

		// Hajodoc: Constraints for successing vehicles
		// Hajoval: string, "none" to disallow any followers
		sprintf(buf, "constraint[next][%d]", besch_nachfolger);

		str = obj.get(buf);
		if (str.len() > 0) {
			if (besch_nachfolger == 0 && !STRICMP(str, "none")) {
				str = "";
			}
			xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str, false);
			besch_nachfolger++;
		}
	} while (str.len() > 0);

	// multiple freight image types - define what good uses each index
	// good without index will be an error
	for (i = 0; i <= freight_max; i++) {
		char buf[40];
		sprintf(buf, "freightimagetype[%d]", i);
		str = obj.get(buf);
		if (i == freight_max) {
			// check for supoerflous definitions
			if (str.len() > 0) {
				printf("WARNING: More freightimagetype (%i) than freight_images (%i)!\n", i, freight_max);
				fflush(NULL);
			}
			break;
		}
		if (str.len() == 0) {
			printf("*** FATAL ***:\nMissing freightimagetype[%i] for %i freight_images!\n", i, freight_max + 1);
			exit(0);
		}
		xref_writer_t::instance()->write_obj(fp, node, obj_good, str, false);
	}

	// if no index defined then add default as vehicle good
	// if not using freight images then store zero string
	if (freight_max > 0) {
		xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, false);
	}

	node.write_sint8(fp, sound_id, 25);

	if (waytype_uint == overheadlines_wt) {
		// Hajo: compatibility for old style DAT files
		uv8 = vehikel_besch_t::electric;
	} else {
		const char* engine_type = obj.get("engine_type");
		uv8 = get_engine_type(engine_type, obj);
	}
	node.write_uint8(fp, uv8, 26);

	// the length (default 8)
	uint8 length = obj.get_int("length", 8);
	node.write_uint8(fp, length, 27);

	node.write_sint8(fp, besch_vorgaenger, 28);
	node.write_sint8(fp, besch_nachfolger, 29);
	node.write_uint8(fp, (uint8) freight_max, 30);

	// Whether this is a tilting train
	// int
	//@author: jamespetts
	uint8 tilting = (obj.get_int("is_tilting", 0));
	node.write_uint8(fp, tilting, 31);

	// Way constraints
	// One byte for permissive, one byte for prohibitive.
	// Therefore, 8 possible constraints of each type.
	// Permissive: way allows vehicles with matching constraint:
	// vehicles not allowed on any other sort of way. Vehicles
	// without that constraint also allowed on the way.
	// Prohibitive: way allows only vehicles with matching constraint:
	// vehicles with matching constraint allowed on other sorts of way.
	// @author: jamespetts
	
	uint8 permissive_way_constraints = 0;
	uint8 prohibitive_way_constraints = 0;
	char buf_permissive[60];
	char buf_prohibitive[60];
	//Read the values from a file, and put them into an array.
	for(uint8 i = 0; i < 8; i++)
	{
		sprintf(buf_permissive, "way_constraint_permissive[%d]", i);
		sprintf(buf_prohibitive, "way_constraint_prohibitive[%d]", i);
		uint8 tmp_permissive = (obj.get_int(buf_permissive, 255));
		uint8 tmp_prohibitive = (obj.get_int(buf_prohibitive, 255));
		
		//Compress values into a single byte using bitwise OR.
		if(tmp_permissive < 8)
		{
			permissive_way_constraints = (tmp_permissive > 0) ? permissive_way_constraints | (uint8)pow(2, (double)tmp_permissive) : permissive_way_constraints | 1;
		}
		if(tmp_prohibitive < 8)
		{
			prohibitive_way_constraints = (tmp_prohibitive > 0) ? prohibitive_way_constraints | (uint8)pow(2, (double)tmp_prohibitive) : prohibitive_way_constraints | 1;
		}
	}
	node.write_uint8(fp, permissive_way_constraints, 32);
	node.write_uint8(fp, prohibitive_way_constraints, 33);

	// Catering level. 0 = no catering. 
	// Higher numbers, better catering.
	// Catering boosts passenger revenue.
	// @author: jamespetts
	uint8 catering_level = (obj.get_int("catering_level", 0));
	node.write_uint8(fp, catering_level, 34);		

	sint8 sound_str_len = sound_str.len();
	if (sound_str_len > 0) {
		node.write_sint8  (fp, sound_str_len, 35);
		node.write_data_at(fp, sound_str,     36, sound_str_len);
	}

	node.write(fp);
}
