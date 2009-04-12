#include <cmath>
#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../../dataobj/ribi.h"
#include "../tunnel_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "tunnel_writer.h"


void tunnel_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int pos, i;

	obj_node_t node(this, 27, &parent);

	uint32 topspeed    = obj.get_int("topspeed",    999);
	uint32 preis       = obj.get_int("cost",          0);
	uint32 maintenance = obj.get_int("maintenance",1000);
	uint8 wegtyp       = get_waytype(obj.get("waytype"));
	uint32 max_weight = obj.get_int("max_weight",  999);

	// prissi: timeline
	uint16 intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 obsolete_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	obsolete_date += obj.get_int("retire_month", 1) - 1;

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

	// Version uses always high bit set as trigger
	// version 2: snow images
	// Version 3: pre-defined ways
	uint16 version = 0x8003;

	// This is the overlay flag for Simutrans-Experimental
	// This sets the *second* highest bit to 1. 
	version |= EXP_VER;

	// Finally, this is the experimental version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).
	version += 0x100;

	node.write_uint16(fp, version,						0);
	node.write_uint32(fp, topspeed,						2);
	node.write_uint32(fp, preis,						6);
	node.write_uint32(fp, maintenance,					10);
	node.write_uint8 (fp, wegtyp,						14);
	node.write_uint16(fp, intro_date,					15);
	node.write_uint16(fp, obsolete_date,				17);
	node.write_uint32(fp, max_weight,					20);
	node.write_uint8(fp, permissive_way_constraints,	24);
	node.write_uint8(fp, prohibitive_way_constraints,	25);
	str = obj.get("way");
	if (str.len() > 0) 
	{
		xref_writer_t::instance()->write_obj(fp, node, obj_way, str, true);
		node.write_sint8(fp, 1, 26);
	}
	else 
	{
		node.write_sint8(fp, 0, 26);
	}

	sint8 number_seasons = 0;

	static const char* const indices[] = { "n", "s", "e", "w" };
	slist_tpl<cstring_t> backkeys;
	slist_tpl<cstring_t> frontkeys;

	slist_tpl<cstring_t> cursorkeys;
	cursorkeys.append(cstring_t(obj.get("cursor")));
	cursorkeys.append(cstring_t(obj.get("icon")));

	char buf[40];
	sprintf(buf, "%simage[%s][0]", "back", indices[0]);

	cstring_t str = obj.get(buf);
	if (strlen(str) == 0) {
		node.write_sint8(fp, number_seasons, 19);
		write_head(fp, node, obj);

		for (pos = 0; pos < 2; pos++) {
			for (i = 0; i < 4; i++) {
				sprintf(buf, "%simage[%s]", pos ? "back" : "front", indices[i]);
				cstring_t str = obj.get(buf);
				(pos ? &backkeys : &frontkeys)->append(str);
			}
		}
		imagelist_writer_t::instance()->write_obj(fp, node, backkeys);
		imagelist_writer_t::instance()->write_obj(fp, node, frontkeys);
		backkeys.clear();
		frontkeys.clear();
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	} else {
		while(number_seasons < 2) {
			sprintf(buf, "%simage[%s][%d]", "back", indices[0], number_seasons+1);
			cstring_t str = obj.get(buf);
			if(strlen(str) > 0) {
				number_seasons++;
			} else {
				break;
			}
		}
		node.write_sint8(fp, number_seasons, 19);
		write_head(fp, node, obj);

		for (uint8 season = 0; season <= number_seasons ; season++) {
			for (pos = 0; pos < 2; pos++) {
				for (i = 0; i < 4; i++) {
					sprintf(buf, "%simage[%s][%d]", pos ? "back" : "front", indices[i], season);
					cstring_t str = obj.get(buf);
					(pos ? &backkeys : &frontkeys)->append(str);
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
	}
	
	cursorkeys.clear();

	node.write(fp);
}
