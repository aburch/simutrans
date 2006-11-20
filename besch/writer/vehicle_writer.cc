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
		uv8 = vehikel_besch_t::hydrogene;
	} else if (!STRICMP(engine_type, "unknown")) {
		uv8 = vehikel_besch_t::unknown;
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
	uint32 uv32;
	uint16 uv16;
	uint8  uv8;
	sint8  sv8;

	int total_len = 31;

	// prissi: must be done here, since it may affect the len of the header!
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

	obj_node_t	node(this, total_len, &parent, false);

	write_head(fp, node, obj);


	// Hajo: version number
	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uv16 = 0x8008;
	node.write_data_at(fp, &uv16, 0, sizeof(uint16));


	// Hajodoc: Price of this vehicle in cent
	// Hajoval: int
	uv32 = obj.get_int("cost", 0);
	node.write_data_at(fp, &uv32, 2, sizeof(uint32));


	// Hajodoc: Payload of this vehicle
	// Hajoval: int
	uv16 = obj.get_int("payload", 0);
	node.write_data_at(fp, &uv16, 6, sizeof(uint16));


	// Hajodoc: Top speed of this vehicle. Must be greater than 0
	// Hajoval: int
	uv16 = obj.get_int("speed", 0);
	node.write_data_at(fp, &uv16, 8, sizeof(uint16));


	// Hajodoc: Total weight of this vehicle in tons
	// Hajoval: int
	uv16 = obj.get_int("weight", 0);
	node.write_data_at(fp, &uv16, 10, sizeof(uint16));


	// Hajodoc: Power of this vehicle in KW
	// Hajoval: int
	uv32 = obj.get_int("power", 0);
	node.write_data_at(fp, &uv32, 12, sizeof(uint32));


	// Hajodoc: Running costs, given in cent per square
	// Hajoval: int
	uv16 = obj.get_int("runningcost", 0);
	node.write_data_at(fp, &uv16, 16, sizeof(uint16));


	// Hajodoc: Introduction date (year * 16 + month)
	// Hajoval: int
	uv16  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	uv16 += obj.get_int("intro_month", 1) - 1;
	node.write_data_at(fp, &uv16, 18, sizeof(uint16));

	// Hajodoc: retire date (year * 16 + month)
	// Hajoval: int
	uv16 = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	uv16 += obj.get_int("retire_month", 1) - 1;
	node.write_data_at(fp, &uv16, 20, sizeof(uint16));

	// Hajodoc: Engine gear (power multiplier)
	// Hajoval: int
	uv16 = (obj.get_int("gear", 100) * 64) / 100;
	node.write_data_at(fp, &uv16, 22, sizeof(uint16));


	// Hajodoc: Type of way this vehicle drives on
	// Hajoval: road, track, electrified_track, monorail_track, maglev_track, water
	const char* waytype = obj.get("waytype");
	const char waytype_uint = get_waytype(waytype);
	uv8 = (waytype_uint == overheadlines_wt ? track_wt : waytype_uint);
	node.write_data_at(fp, &uv8, 24, sizeof(uint8));

	// Hajodoc: The freight type
	// Hajoval: string
	const char* freight = obj.get("freight");
	if (!*freight) {
		freight = "None";
	}
	xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, true);
	xref_writer_t::instance()->write_obj(fp, node, obj_smoke, obj.get("smoke"), false);

	// Jetzt kommen die Bildlisten
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

		// Hajodoc: Empty vehicle image for direction, direction in "s", "w", "sw", "se", unsymmetric vehicles need also "n", "e", "ne", "nw"
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
	if (has_8_images && emptykeys.count() < 8) {
		printf("*** FATAL ***:\nMissing images (must be either 4 or 8 directions (but %i found)!)\n", emptykeys.count());
		exit(0);
	}
	if (!freightkeys_old.empty() && emptykeys.count() != freightkeys_old.count()) {
		printf("*** FATAL ***:\nMissing freigthimages (must be either 4 or 8 directions (but %i found)!)\n", freightkeys_old.count());
		exit(0);
	}

	imagelist_writer_t::instance()->write_obj(fp, node, emptykeys);
	if (freight_max > 0) {
		imagelist2d_writer_t::instance()->write_obj(fp, node, freightkeys);
	} else {
		if (freightkeys_old.count() == emptykeys.count()) {
			imagelist_writer_t::instance()->write_obj(fp, node, freightkeys_old);
		} else {
			// really empty list ...
			xref_writer_t::instance()->write_obj(fp, node, obj_imagelist, "", false);
		}
	}

	//
	// Vorgänger/Nachfolgerbedingungen
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

	node.write_data_at(fp, &sound_id, 25, sizeof(sint8));

	if (waytype_uint == overheadlines_wt) {
		// Hajo: compatibility for old style DAT files
		uv8 = vehikel_besch_t::electric;
	} else {
		const char* engine_type = obj.get("engine_type");
		uv8 = get_engine_type(engine_type, obj);
	}
	node.write_data_at(fp, &uv8, 26, sizeof(uint8));

	// the length (default 8)
	uv8 = obj.get_int("length", 8);
	node.write_data_at(fp, &uv8, 27, sizeof(uint8));

	node.write_data_at(fp, &besch_vorgaenger, 28, sizeof(sint8));
	node.write_data_at(fp, &besch_nachfolger, 29, sizeof(sint8));

	uv8 = freight_max;
	node.write_data_at(fp, &uv8, 30, sizeof(sint8));

	if (sound_str.len() > 0) {
		sv8 = sound_str.len();
		node.write_data_at(fp, &sv8, 31, sizeof(sint8));
		node.write_data_at(fp, sound_str, 32, sound_str.len());
	}

	node.write(fp);
}
