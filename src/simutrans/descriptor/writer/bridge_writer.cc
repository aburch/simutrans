/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../bridge_desc.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "bridge_writer.h"

using std::string;

void write_bridge_images(FILE* outfp, obj_node_t& node, tabfileobj_t& obj, int season)
{
	slist_tpl<string> backkeys;
	slist_tpl<string> frontkeys;

	static const char* const names[] = {
		"image",
		"ns", "ew", NULL,
		"start",
		"n", "s", "e", "w", NULL,
		"ramp",
		"n", "s", "e", "w", NULL,
		"pillar",
		"s", "w", NULL,
		"image2",
		"ns", "ew", NULL,
		"start2",
		"n", "s", "e", "w", NULL,
		"ramp2",
		"n", "s", "e", "w", NULL,
		"pillar2",
		"s", "w", NULL,
		NULL
	};

	const char* const * ptr = names;
	const char* keyname = *ptr++;
	char keybuf[40];

	do {
		const char* keyindex = *ptr++;
		do {
			string value;

			if(  season < 0  ) {
				sprintf( keybuf, "back%s[%s]", keyname, keyindex );
				value = obj.get( keybuf );
				backkeys.append( value );
				//intf("BACK: %s -> %s\n", keybuf, value.chars());
				sprintf( keybuf, "front%s[%s]", keyname, keyindex );
			}
			else {
				sprintf( keybuf, "back%s[%s][%d]", keyname, keyindex, season );
				value = obj.get( keybuf );
				backkeys.append( value );
				//intf("BACK: %s -> %s\n", keybuf, value.chars());
				sprintf( keybuf, "front%s[%s][%d]", keyname, keyindex, season );
			}

			// must append to front keys even if empty to keep order correct (but warn anyway)
			value = obj.get( keybuf );
			frontkeys.append( value );
			//intf("FRNT: %s -> %s\n", keybuf, value.chars());
			if(  value.size() <= 2  ) {
				dbg->warning( obj_writer_t::last_name, "No %s specified (might still work)", keybuf );
			}

			keyindex = *ptr++;
		} while(  keyindex  );
		keyname = *ptr++;
	} while(  keyname  );

	imagelist_writer_t::instance()->write_obj( outfp, node, backkeys );
	imagelist_writer_t::instance()->write_obj( outfp, node, frontkeys );
	if(  season <= 0  ) {
		slist_tpl<string> cursorkeys;
		cursorkeys.append( string( obj.get("cursor") ) );
		cursorkeys.append( string( obj.get("icon") ) );

		cursorskin_writer_t::instance()->write_obj( outfp, node, obj, cursorkeys );

		cursorkeys.clear();
	}
	backkeys.clear();
	frontkeys.clear();
}

void bridge_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 24, &parent);

	uint8  wegtyp        = get_waytype(obj.get("waytype"));
	uint16 topspeed      = obj.get_int("topspeed", 999);
	uint32 price         = obj.get_int("cost", 0);
	uint32 maintenance   = obj.get_int("maintenance", 1000);
	uint8  pillars_every = obj.get_int("pillar_distance",0); // distance==0 is off
	uint8  pillar_asymmetric = obj.get_int("pillar_asymmetric",0); // middle of tile
	uint8  max_length    = obj.get_int("max_lenght",0); // max_lenght==0: unlimited
	max_length    = obj.get_int("max_length",max_length); // with correct spelling
	uint8  max_height    = obj.get_int("max_height",0); // max_height==0: unlimited
	uint16 axle_load = obj.get_int("axle_load",    9999);

	// timeline
	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 retire_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;

	sint8 number_of_seasons = 0;

	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversionend
	uint16 version = 0x8009;
	node.write_uint16(outfp, version,            0);
	node.write_uint16(outfp, topspeed,           2);
	node.write_uint32(outfp, price,              4);
	node.write_uint32(outfp, maintenance,        8);
	node.write_uint8 (outfp, wegtyp,            12);
	node.write_uint8 (outfp, pillars_every,     13);
	node.write_uint8 (outfp, max_length,        14);
	node.write_uint16(outfp, intro_date,        15);
	node.write_uint16(outfp, retire_date,       17);
	node.write_uint8 (outfp, pillar_asymmetric, 19);
	node.write_uint16(outfp, axle_load,         20);
	node.write_uint8 (outfp, max_height,        22);

	char keybuf[40];

	string str = obj.get("backimage[ns][0]");
	if (str.empty()) {
		node.write_data_at(outfp, &number_of_seasons, 23, sizeof(uint8));
		write_head(outfp, node, obj);
		write_bridge_images( outfp, node, obj, -1 );

	}
	else {
		while(number_of_seasons < 2) {
			sprintf(keybuf, "backimage[ns][%d]", number_of_seasons+1);
			string str = obj.get(keybuf);
			if (!str.empty()) {
				number_of_seasons++;
			}
			else {
				break;
			}
		}

		node.write_data_at(outfp, &number_of_seasons, 23, sizeof(uint8));
		write_head(outfp, node, obj);

		for(uint8 season = 0 ; season <= number_of_seasons ; season++) {
			write_bridge_images( outfp, node, obj, season );
		}
	}

	// node.write_data(outfp, &desc);
	node.write(outfp);
}
