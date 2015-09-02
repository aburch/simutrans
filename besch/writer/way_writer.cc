#include <cmath>
#include <string>
#include "../../dataobj/tabfile.h"
#include "../../utils/simstring.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../weg_besch.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "way_writer.h"

using std::string;

/**
 * Write a waytype description node
 */
void way_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	static const char* const ribi_codes[26] = {
		"-", "n",  "e",  "ne",  "s",  "ns",  "se",  "nse",
		"w", "nw", "ew", "new", "sw", "nsw", "sew", "nsew",
		"nse1", "new1", "nsw1", "sew1", "nsew1",	// different crossings: northwest/southeast is straight
		"nse2", "new2", "nsw2", "sew2", "nsew2",
	};
	int ribi, hang;

	// node size is 46 bytes
	obj_node_t node(this, 45, &parent);


	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version     = 0x8006;

	// This is the overlay flag for Simutrans-Experimental
	// This sets the *second* highest bit to 1. 
	version |= EXP_VER;

	// Finally, this is the experimental version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	// 0x200 - 12.x - added max. speeds for different gradients.
	version += 0x200;

	uint32 price				= obj.get_int("cost",        100);
	uint32 maintenance			= obj.get_int("maintenance", 100);
	sint32 topspeed				= obj.get_int("topspeed",    999);
	sint32 topspeed_gradient_1  = obj.get_int("topspeed_gradient_1",    topspeed);
	sint32 topspeed_gradient_2  = obj.get_int("topspeed_gradient_2",    topspeed_gradient_1);
	uint16 axle_load			= obj.get_int("max_weight",  9999);
	axle_load					= obj.get_int("axle_load",  axle_load);
	sint8 max_altitude			= obj.get_int("max_altitude",			0);
	uint8 max_vehicles_on_tile	= obj.get_int("max_vehicles_on_tile",	251);
	uint32 way_only_cost		= obj.get_int("way_only_cost", price);
	uint8 upgrade_group			= obj.get_int("upgrade_group", 0); 

	uint16 intro  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro += obj.get_int("intro_month", 1) - 1;

	uint16 retire  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire += obj.get_int("retire_month", 1) - 1;

	uint8 wtyp = get_waytype(obj.get("waytype"));
	uint8 styp = obj.get_int("system_type", 0);
	// compatibility conversions
	if (wtyp == track_wt && styp == 5) {
		wtyp = monorail_wt;
	}
	else if (wtyp == track_wt && styp == 7) {
		wtyp = tram_wt;
	}

	// Must use get_int64 here, as get_int is limited to the length of a *signed* 32-bit integer.
	uint32 wear_capacity		= obj.get_int64("wear_capacity", wtyp == road_wt ? 100000000 : 4000000000);

	// true to draw as foregrund and not much earlier (default)
	uint8 draw_as_obj = (obj.get_int("draw_as_ding", 0) == 1);
	draw_as_obj = (obj.get_int("draw_as_obj", draw_as_obj) == 1);
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

	node.write_uint16(outfp, version,					0);
	node.write_uint32(outfp, price,						2);
	node.write_uint32(outfp, maintenance,				6);
	node.write_sint32(outfp, topspeed,					10);
	node.write_uint16(outfp, intro,						14);
	node.write_uint16(outfp, retire,					16);
	node.write_uint16(outfp, axle_load,                 18);
	node.write_uint8 (outfp, wtyp,						20);
	node.write_uint8 (outfp, styp,						21);
	node.write_uint8 (outfp, draw_as_obj,				22);
	// Seasons											23
	node.write_uint8(outfp, permissive_way_constraints,	24);
	node.write_uint8(outfp, prohibitive_way_constraints,25);
	node.write_sint32(outfp, topspeed_gradient_1,		26);
	node.write_sint32(outfp, topspeed_gradient_2,		30);
	node.write_sint8(outfp, max_altitude,				34);
	node.write_uint8(outfp, max_vehicles_on_tile,		35);
	node.write_uint32(outfp, wear_capacity,				36);
	node.write_uint32(outfp, way_only_cost,				40);
	node.write_uint8(outfp, upgrade_group,				44); 

	static const char* const image_type[] = { "", "front" };

	slist_tpl<string> keys;
	char buf[40];
	sprintf(buf, "image[%s][0]", ribi_codes[0]);
	string str = obj.get(buf);
	if (str.empty()) {
		node.write_data_at(outfp, &number_seasons, 23, 1);
		write_head(outfp, node, obj);

		sprintf(buf, "image[%s]", ribi_codes[0]);
		string str = obj.get(buf);
		if (str.empty()) {
			dbg->fatal("way_writer_t::write_obj", "image with label %s missing", buf);
		}
		for(int backtofront = 0; backtofront<2; backtofront++) {
			// way images defined without seasons
			char buf[40];
			sprintf(buf, "%simage[new2][0]", image_type[backtofront]);
			// test for switch images
			const uint8 ribinr = *(obj.get(buf))==0 ? 16 : 26;
			for (ribi = 0; ribi < ribinr; ribi++) {
				char buf[40];

				sprintf(buf, "%simage[%s]", image_type[backtofront], ribi_codes[ribi]);
				string str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);

			keys.clear();
			for(  hang = 3;  hang <= 12;  hang += 3  ) {
				char buf[40];

				sprintf( buf, "%simageup[%d]", image_type[backtofront], hang );
				string str = obj.get(buf);
				keys.append(str);
			}
			for(  hang = 3;  hang <= 12;  hang += 3  ) {
				char buf[40];

				sprintf( buf, "%simageup2[%d]", image_type[backtofront], hang );
				string str = obj.get(buf);
				if(  !str.empty()  ) {
					keys.append(str);
				}
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);

			keys.clear();
			for (ribi = 3; ribi <= 12; ribi += 3) {
				char buf[40];

				sprintf(buf, "%sdiagonal[%s]", image_type[backtofront], ribi_codes[ribi]);
				string str = obj.get(buf);
				keys.append(str);
			}
			imagelist_writer_t::instance()->write_obj(outfp, node, keys);
			keys.clear();


			if(backtofront == 0) {
				slist_tpl<string> cursorkeys;

				cursorkeys.append(string(obj.get("cursor")));
				cursorkeys.append(string(obj.get("icon")));

				cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
			}
		}
	} else {
		sprintf(buf, "image[%s][%d]", ribi_codes[0], number_seasons+1);
		if (!strempty(obj.get(buf))) {
			number_seasons++;
		}

		node.write_data_at(outfp, &number_seasons, 23, 1);
		write_head(outfp, node, obj);

		// has switch images for both directions?
		const uint8 ribinr = *(obj.get("image[new2][0]"))==0 ? 16 : 26;

		for(int backtofront = 0; backtofront<2; backtofront++) {
			for (uint8 season = 0; season <= number_seasons ; season++) {
				for (ribi = 0; ribi < ribinr; ribi++) {
					char buf[40];

					sprintf(buf, "%simage[%s][%d]", image_type[backtofront], ribi_codes[ribi], season);
					string str = obj.get(buf);
					keys.append(str);
				}
				imagelist_writer_t::instance()->write_obj(outfp, node, keys);

				keys.clear();
				for(  hang = 3;  hang <= 12;  hang += 3  ) {
					char buf[40];

					sprintf( buf, "%simageup[%d][%d]", image_type[backtofront], hang, season );
					string str = obj.get(buf);
					keys.append(str);
				}
				for(  hang = 3;  hang <= 12;  hang += 3  ) {
					char buf[40];

					sprintf( buf, "%simageup2[%d][%d]", image_type[backtofront], hang, season );
					string str = obj.get(buf);
					if(  !str.empty()  ) {
						keys.append(str);
					}
				}
				imagelist_writer_t::instance()->write_obj(outfp, node, keys);

				keys.clear();
				for (ribi = 3; ribi <= 12; ribi += 3) {
					char buf[40];

					sprintf(buf, "%sdiagonal[%s][%d]", image_type[backtofront], ribi_codes[ribi], season);
					string str = obj.get(buf);
					keys.append(str);
				}
				imagelist_writer_t::instance()->write_obj(outfp, node, keys);

				keys.clear();
				if(season == 0  &&  backtofront == 0) {
					slist_tpl<string> cursorkeys;

					cursorkeys.append(string(obj.get("cursor")));
					cursorkeys.append(string(obj.get("icon")));

					cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);
				}
			}
		}
	}

	node.write(outfp);
}
