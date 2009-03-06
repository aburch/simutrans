#include <cmath>
#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../bruecke_besch.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "bridge_writer.h"


void bridge_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 28, &parent);

	uint8  wegtyp        = get_waytype(obj.get("waytype"));
	uint16 topspeed      = obj.get_int("topspeed", 999);
	uint32 preis         = obj.get_int("cost", 0);
	uint32 maintenance   = obj.get_int("maintenance", 1000);
	uint8  pillars_every = obj.get_int("pillar_distance",0); // distance==0 is off
	uint8  pillar_asymmetric = obj.get_int("pillar_asymmetric",0); // middle of tile
	uint8  max_length    = obj.get_int("max_lenght",0); // max_lenght==0: unlimited
	max_length    = obj.get_int("max_length",max_length); // with correct spelling
	uint8  max_height    = obj.get_int("max_height",0); // max_height==0: unlimited
	uint32 max_weight	 = obj.get_int("max_weight",999);

	// prissi: timeline
	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 obsolete_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	obsolete_date += obj.get_int("retire_month", 1) - 1;

	sint8 number_seasons = 0;

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
		
		// Values must fit into one byte
		// Must therefore be 0-7
		if(tmp_permissive > 7 || tmp_prohibitive > 7)
		{
			continue;
		}
		
		//Compress values into a single byte using bitwise OR.
		permissive_way_constraints = (tmp_permissive > 0) ? permissive_way_constraints | (uint8)pow(2, (double)tmp_permissive) : permissive_way_constraints | 1;
		prohibitive_way_constraints = (tmp_prohibitive > 0) ? prohibitive_way_constraints | (uint8)pow(2, (double)tmp_prohibitive) : prohibitive_way_constraints | 1;
	}

	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8008;
	
	// This is the overlay flag for Simutrans-Experimental
	// This sets the *second* highest bit to 1. 
	version |= EXP_VER;

	// Finally, this is the experimental version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	version += 0x100;

	node.write_uint16(outfp, version,					0);
	node.write_uint16(outfp, topspeed,					2);
	node.write_uint32(outfp, preis,						4);
	node.write_uint32(outfp, maintenance,				8);
	node.write_uint8 (outfp, wegtyp,					12);
	node.write_uint8 (outfp, pillars_every,				13);
	node.write_uint8 (outfp, max_length,				14);
	node.write_uint16(outfp, intro_date,				15);
	node.write_uint16(outfp, obsolete_date,				17);
	node.write_uint8 (outfp, pillar_asymmetric,			19);
	node.write_uint8 (outfp, max_height,				20);
	node.write_uint32(outfp, max_weight,				22);
	node.write_uint8(outfp, permissive_way_constraints,	26);
	node.write_uint8(outfp, prohibitive_way_constraints,27);

	static const char* const names[] = {
		"image",
		"ns", "ew", NULL,
		"start",
		"n", "s", "e", "w", NULL,
		"ramp",
		"n", "s", "e", "w", NULL,
		NULL
	};
	slist_tpl<cstring_t> backkeys;
	slist_tpl<cstring_t> frontkeys;

	slist_tpl<cstring_t> cursorkeys;
	cursorkeys.append(cstring_t(obj.get("cursor")));
	cursorkeys.append(cstring_t(obj.get("icon")));

	char keybuf[40];

	cstring_t str = obj.get("backimage[ns][0]");
	if(strlen(str) == 0) {
		node.write_data_at(outfp, &number_seasons, 21, sizeof(uint8));
		write_head(outfp, node, obj);
		const char* const * ptr = names;
		const char* keyname = *ptr++;

		do {
			const char* keyindex = *ptr++;
			do {
				cstring_t value;

				sprintf(keybuf, "back%s[%s]", keyname, keyindex);
				value = obj.get(keybuf);
				backkeys.append(value);
				//intf("BACK: %s -> %s\n", keybuf, value.chars());
				sprintf(keybuf, "front%s[%s]", keyname, keyindex);
				value = obj.get(keybuf);
				if (value.len() > 2) {
					frontkeys.append(value);
					//intf("FRNT: %s -> %s\n", keybuf, value.chars());
				} else {
					printf("WARNING: not %s specified (but might be still working)\n",keybuf);
				}
				keyindex = *ptr++;
			} while (keyindex);
			keyname = *ptr++;
		} while (keyname);

		if (pillars_every > 0) {
			backkeys.append(cstring_t(obj.get("backpillar[s]")));
			backkeys.append(cstring_t(obj.get("backpillar[w]")));
		}
		imagelist_writer_t::instance()->write_obj(outfp, node, backkeys);
		imagelist_writer_t::instance()->write_obj(outfp, node, frontkeys);
		cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
		backkeys.clear();
		frontkeys.clear();
	} else {
		while(number_seasons < 2) {
			sprintf(keybuf, "backimage[ns][%d]", number_seasons+1);
			cstring_t str = obj.get(keybuf);
			if(strlen(str) > 0) {
				number_seasons++;
			} else {
				break;
			}
		}

		node.write_data_at(outfp, &number_seasons, 21, sizeof(uint8));
		write_head(outfp, node, obj);

		for(uint8 season = 0 ; season <= number_seasons ; season++) {
			const char* const * ptr = names;
			const char* keyname = *ptr++;

			do {
				const char* keyindex = *ptr++;
				do {
					cstring_t value;

					sprintf(keybuf, "back%s[%s][%d]", keyname, keyindex, season);
					value = obj.get(keybuf);
					backkeys.append(value);
					//intf("BACK: %s -> %s\n", keybuf, value.chars());
					sprintf(keybuf, "front%s[%s][%d]", keyname, keyindex, season);
					value = obj.get(keybuf);
					if (value.len() > 2) {
						frontkeys.append(value);
						//intf("FRNT: %s -> %s\n", keybuf, value.chars());
					} else {
						printf("WARNING: not %s specified (but might be still working)\n",keybuf);
					}
					keyindex = *ptr++;
				} while (keyindex);
				keyname = *ptr++;
			} while (keyname);

			if (pillars_every > 0) {
				sprintf(keybuf, "backpillar[s][%d]",season);
				backkeys.append(cstring_t(obj.get(keybuf)));
				sprintf(keybuf, "backpillar[w][%d]",season);
				backkeys.append(cstring_t(obj.get(keybuf)));
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, backkeys);
			imagelist_writer_t::instance()->write_obj(outfp, node, frontkeys);
			if(season == 0 ) {
				cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
			}
			backkeys.clear();
			frontkeys.clear();
		}
	}

	cursorkeys.clear();

	// node.write_data(outfp, &besch);
	node.write(outfp);
}
