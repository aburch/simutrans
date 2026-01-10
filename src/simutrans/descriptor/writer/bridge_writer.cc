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

#include <string>


static void write_bridge_images(FILE* outfp, obj_node_t& node, tabfileobj_t& obj, int season)
{
	slist_tpl<std::string> backkeys;
	slist_tpl<std::string> frontkeys;

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
			std::string value;

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
		slist_tpl<std::string> cursorkeys;
		cursorkeys.append( std::string( obj.get("cursor") ) );
		cursorkeys.append( std::string( obj.get("icon") ) );

		cursorskin_writer_t::instance()->write_obj( outfp, node, obj, cursorkeys );

		cursorkeys.clear();
	}

	backkeys.clear();
	frontkeys.clear();
}


void bridge_writer_t::write_obj(FILE *outfp, obj_node_t &parent, tabfileobj_t &obj)
{
	const waytype_t waytype        = get_waytype(obj.get("waytype"));
	const uint16 topspeed          = obj.get_int("topspeed", 999);
	const sint64 price             = obj.get_int64("cost", 0);
	const sint64 maintenance       = obj.get_int64("maintenance", 1000);
	const uint8  pillars_every     = obj.get_int_clamped("pillar_distance",     0, 0, UINT8_MAX); // distance==0 is off
	const uint8  pillar_asymmetric = obj.get_int_clamped("pillar_asymmetric",   0, 0, 1);         // middle of tile
	      uint8  max_length        = obj.get_int_clamped("max_lenght",          0, 0, UINT8_MAX); // max_lenght==0: unlimited
	             max_length        = obj.get_int_clamped("max_length", max_length, 0, UINT8_MAX); // with correct spelling
	const uint8  max_height        = obj.get_int_clamped("max_height",          0, 0, UINT8_MAX); // max_height==0: unlimited
	const uint16 axle_load         = obj.get_int_clamped("axle_load",        9999, 0, UINT16_MAX);
	const uint8  clip_below        = obj.get_int_clamped("clip_below",          1, 0, 1);         // clip ground below

	// timeline
	const uint16 intro_date  = 12*obj.get_int_clamped("intro_year",  DEFAULT_INTRO_YEAR,  0, INT32_MAX) + obj.get_int_clamped("intro_month",  1, 1, 12) - 1;
	const uint16 retire_date = 12*obj.get_int_clamped("retire_year", DEFAULT_RETIRE_YEAR, 0, INT32_MAX) + obj.get_int_clamped("retire_month", 1, 1, 12) - 1;

	obj_node_t node(this, 33, &parent);

	node.write_version(outfp, 11);
	node.write_uint16(outfp, topspeed);
	node.write_sint64(outfp, price);
	node.write_sint64(outfp, maintenance);
	node.write_uint8 (outfp, waytype);
	node.write_uint8 (outfp, pillars_every);
	node.write_uint8 (outfp, max_length);
	node.write_uint16(outfp, intro_date);
	node.write_uint16(outfp, retire_date);
	node.write_uint8 (outfp, pillar_asymmetric);
	node.write_uint16(outfp, axle_load);
	node.write_uint8 (outfp, max_height);
	node.write_uint8 (outfp, clip_below);

	char keybuf[40];
	std::string str = obj.get("backimage[ns][0]");
	sint8 number_of_seasons = 0;

	if (str.empty()) {
		node.write_uint8(outfp, number_of_seasons);
		write_name_and_copyright(outfp, node, obj);
		write_bridge_images( outfp, node, obj, -1 );
	}
	else {
		while(number_of_seasons < 2) {
			sprintf(keybuf, "backimage[ns][%d]", number_of_seasons+1);
			std::string str = obj.get(keybuf);
			if (!str.empty()) {
				number_of_seasons++;
			}
			else {
				break;
			}
		}

		node.write_uint8(outfp, number_of_seasons);
		write_name_and_copyright(outfp, node, obj);

		for(uint8 season = 0 ; season <= number_of_seasons ; season++) {
			write_bridge_images( outfp, node, obj, season );
		}
	}

	node.check_and_write_header(outfp);
}
