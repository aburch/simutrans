#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../../dataobj/ribi.h"
#include "../tunnel_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "tunnel_writer.h"


void tunnel_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	tunnel_besch_t besch;
	int pos, i;

	obj_node_t node(this, 19, &parent, false);

	uint32 topspeed    = obj.get_int("topspeed",     999);
	uint32 preis       = obj.get_int("cost",           0);
	uint32 maintenance = obj.get_int("maintenance", 1000);
	uint8 wegtyp       = get_waytype(obj.get("waytype"));

	// prissi: timeline
	uint16 intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 obsolete_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	obsolete_date += obj.get_int("retire_month", 1) - 1;

	// Version uses always high bit set as trigger
	uint16 version = 0x8001;
	node.write_data_at(fp, &version,        0, 2);
	node.write_data_at(fp, &topspeed,       2, sizeof(uint32));
	node.write_data_at(fp, &preis,          6, sizeof(uint32));
	node.write_data_at(fp, &maintenance,   10, sizeof(uint32));
	node.write_data_at(fp, &wegtyp,        14, sizeof(uint8));
	node.write_data_at(fp, &intro_date,    15, sizeof(uint16));
	node.write_data_at(fp, &obsolete_date, 17, sizeof(uint16));
	write_head(fp, node, obj);

	static const char* const indices[] = { "n", "s", "e", "w" };
	slist_tpl<cstring_t> backkeys;
	slist_tpl<cstring_t> frontkeys;

	for (pos = 0; pos < 2; pos++) {
		for (i = 0; i < 4; i++) {
			char buf[40];
			sprintf(buf, "%simage[%s]", pos ? "back" : "front", indices[i]);
			cstring_t str = obj.get(buf);
			(pos ? &backkeys : &frontkeys)->append(str);
		}
	}

	slist_tpl<cstring_t> cursorkeys;
	cursorkeys.append(cstring_t(obj.get("cursor")));
	cursorkeys.append(cstring_t(obj.get("icon")));

	imagelist_writer_t::instance()->write_obj(fp, node, backkeys);
	imagelist_writer_t::instance()->write_obj(fp, node, frontkeys);
	cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);

	cursorkeys.clear();
	backkeys.clear();
	frontkeys.clear();

	node.write(fp);
}
