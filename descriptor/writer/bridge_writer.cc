/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <cmath>
#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../bridge_desc.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "bridge_writer.h"
#include "xref_writer.h"

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
	obj_node_t node(this, 38, &parent);

	uint8  waytype_t				= get_waytype(obj.get("waytype"));
	uint16 topspeed					= obj.get_int("topspeed", 999);
	uint16 topspeed_gradient_1      = obj.get_int("topspeed_gradient_1", topspeed);
	uint16 topspeed_gradient_2      = obj.get_int("topspeed_gradient_2", topspeed_gradient_1);
	uint32 price					= obj.get_int("cost", 0);
	uint32 maintenance				= obj.get_int("maintenance", 1000);
	uint8  pillars_every			= obj.get_int("pillar_distance",0); // distance==0 is off
	uint8  pillar_asymmetric		= obj.get_int("pillar_asymmetric",0); // middle of tile
	uint8  max_length				= obj.get_int("max_lenght",0); // max_lenght==0: unlimited
	max_length						= obj.get_int("max_length",max_length); // with correct spelling
	uint8  max_height				= obj.get_int("max_height",0); // max_height==0: unlimited
	uint16 axle_load				= obj.get_int("axle_load", 9999);
	uint32 max_weight				= obj.get_int("max_weight", 250);
	sint8 max_altitude				= obj.get_int("max_altitude", 0);
	uint8 max_vehicles_on_tile		= obj.get_int("max_vehicles_on_tile", 251);
	uint8 has_own_way_graphics		= obj.get_int("has_own_way_graphics", 1); // Traditionally, bridges had their own way graphics, hence the default of 1.

	// timeline
	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 retire_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;

	sint8 number_of_seasons = 0;

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
			permissive_way_constraints = (tmp_permissive > 0) ? permissive_way_constraints | (uint8)pow(2.0, (double)tmp_permissive) : permissive_way_constraints | 1;
		}
		if(tmp_prohibitive < 8)
		{
			prohibitive_way_constraints = (tmp_prohibitive > 0) ? prohibitive_way_constraints | (uint8)pow(2.0, (double)tmp_prohibitive) : prohibitive_way_constraints | 1;
		}
	}

	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversionend
	uint16 version = 0x8009;

	// This is the overlay flag for Simutrans-Extended
	// This sets the *second* highest bit to 1.
	version |= EX_VER;

	// Finally, this is the extended version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	version += 0x200;

	node.write_uint16(outfp, version,					0);
	node.write_uint16(outfp, topspeed,					2);
	node.write_uint32(outfp, price,						4);
	node.write_uint32(outfp, maintenance,				8);
	node.write_uint8 (outfp, waytype_t,					12);
	node.write_uint8 (outfp, pillars_every,				13);
	node.write_uint8 (outfp, max_length,				14);
	node.write_uint16(outfp, intro_date,				15);
	node.write_uint16(outfp, retire_date,				17);
	node.write_uint8 (outfp, pillar_asymmetric,			19);
	node.write_uint8 (outfp, max_height,				20);
	node.write_uint16(outfp, axle_load,					21);
	node.write_uint32(outfp, max_weight,				23);
	node.write_uint8(outfp, permissive_way_constraints,	27);
	node.write_uint8(outfp, prohibitive_way_constraints,28);
	node.write_uint16(outfp, topspeed_gradient_1,		29);
	node.write_uint16(outfp, topspeed_gradient_2,		31);
	node.write_sint8(outfp, max_altitude,				33);
	node.write_uint8(outfp, max_vehicles_on_tile,		34);
	node.write_uint8(outfp, has_own_way_graphics,		35);

	char keybuf[40];

	string str = obj.get("backimage[ns][0]");
	if (str.empty()) {
		node.write_data_at(outfp, &number_of_seasons, 37, sizeof(uint8));
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

		node.write_data_at(outfp, &number_of_seasons, 37, sizeof(uint8));
		write_head(outfp, node, obj);

		for(uint8 season = 0 ; season <= number_of_seasons ; season++) {
			write_bridge_images( outfp, node, obj, season );
		}
	}

	str = obj.get("way");
	if (!str.empty()) {
		xref_writer_t::instance()->write_obj(outfp, node, obj_way, str.c_str(), false);
		node.write_sint8(outfp, 1, 36);
	}
	else {
		node.write_sint8(outfp, 0, 36);
	}

	node.write(outfp);
}
