/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../dataobj/tabfile.h"
#include "../../dataobj/ribi.h"
#include "../tunnel_desc.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "tunnel_writer.h"

using std::string;

void tunnel_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 24, &parent);

	sint32 topspeed    = obj.get_int("topspeed",     999);
	uint32 price       = obj.get_int("cost",           0);
	uint32 maintenance = obj.get_int("maintenance", 1000);
	uint8 wtyp         = get_waytype(obj.get("waytype"));
	uint16 axle_load   = obj.get_int("axle_load",   9999);

	// timeline
	uint16 intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 retire_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;

	// predefined string for directions
	static const char* const indices[] = { "n", "s", "e", "w" };
	static const char* const add[] = { "", "l", "r", "m" };
	char buf[40];

	// Check for seasons
	sint8 number_of_seasons = 0;
	sprintf(buf, "%simage[%s][1]", "front", indices[0]);
	string str = obj.get(buf);
	if(  str.size() != 0  ) {
		// Snow images are present.
		number_of_seasons = 1;
	}

	// Check for broad portals
	uint8 number_portals = 1;
	sprintf(buf, "%simage[%s%s][0]", "front", indices[0], add[1]);
	str = obj.get(buf);
	if (str.empty()) {
		// Test short version
		sprintf(buf, "%simage[%s%s]", "front", indices[0], add[1]);
		str = obj.get(buf);
	}
	if(  str.size() != 0  ) {
		number_portals = 4;
	}

	// Version uses always high bit set as trigger
	// version 4: snow images + underground way image + broad portals
	uint16 version = 0x8005;
	node.write_uint16(fp, version,           0);
	node.write_sint32(fp, topspeed,          2);
	node.write_uint32(fp, price,             6);
	node.write_uint32(fp, maintenance,      10);
	node.write_uint8 (fp, wtyp,             14);
	node.write_uint16(fp, intro_date,       15);
	node.write_uint16(fp, retire_date,      17);
	node.write_uint16(fp, axle_load,        19);
	node.write_sint8(fp, number_of_seasons, 21);
	// has was (uint8) is here but filled later
	node.write_sint8(fp, (number_portals==4), 23);

	write_head(fp, node, obj);

	// and now the images
	slist_tpl<string> backkeys;
	slist_tpl<string> frontkeys;

	slist_tpl<string> cursorkeys;
	cursorkeys.append(string(obj.get("cursor")));
	cursorkeys.append(string(obj.get("icon")));
	for(  uint8 season = 0;  season <= number_of_seasons;  season++  ) {
		for(  uint8 pos = 0;  pos < 2;  pos++  ) {
			for(  uint8 j = 0;  j < number_portals;  j++  ) {
				for(  uint8 i = 0;  i < 4;  i++  ) {
					sprintf(buf, "%simage[%s%s][%d]", pos ? "back" : "front", indices[i], add[j], season);
					string str = obj.get(buf);
					if (str.empty() && season == 0) {
						// Test also the short version.
						sprintf(buf, "%simage[%s%s]", pos ? "back" : "front", indices[i], add[j]);
						str = obj.get(buf);
					}
					(pos ? &backkeys : &frontkeys)->append(str);
				}
			}
		}
		imagelist_writer_t::instance()->write_obj(fp, node, backkeys);
		imagelist_writer_t::instance()->write_obj(fp, node, frontkeys);
		backkeys.clear();
		frontkeys.clear();
		if(season == 0) {
			cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
		}
	}

	str = obj.get("way");
	if (!str.empty()) {
		xref_writer_t::instance()->write_obj(fp, node, obj_way, str.c_str(), false);
		node.write_sint8(fp, 1, 22);
	}
	else {
		node.write_sint8(fp, 0, 22);
	}

	cursorkeys.clear();

	node.write(fp);
}
