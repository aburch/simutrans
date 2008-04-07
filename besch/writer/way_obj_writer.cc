#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../way_obj_besch.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"
#include "get_waytype.h"
#include "way_obj_writer.h"


/**
 * Write a way object (lamps, overheadwires)
 */
void way_obj_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	static const char* const ribi_codes[16] = {
		"-", "n",  "e",  "ne",  "s",  "ns",  "se",  "nse",
		"w", "nw", "ew", "new", "sw", "nsw", "sew", "nsew"
	};
	int ribi, hang;

	// Hajo: node size is 24 bytes
	obj_node_t node(this, 20, &parent, false);


	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version     = 0x8001;
	uint32 price       = obj.get_int("cost",        100);
	uint32 maintenance = obj.get_int("maintenance", 100);
	uint32 topspeed    = obj.get_int("topspeed",    999);

	uint16 intro  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro +=        obj.get_int("intro_month", 1) - 1;

	uint16 retire  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire +=        obj.get_int("retire_month", 1) - 1;

	uint8 wtyp     =  get_waytype(obj.get("waytype"));
	uint8 own_wtyp =  get_waytype(obj.get("own_waytype"));

	node.write_data_at(outfp, &version,      0, 2);
	node.write_data_at(outfp, &price,        2, 4);
	node.write_data_at(outfp, &maintenance,  6, 4);
	node.write_data_at(outfp, &topspeed,    10, 4);
	node.write_data_at(outfp, &intro,       14, 2);
	node.write_data_at(outfp, &retire,      16, 2);
	node.write_data_at(outfp, &wtyp,        18, 1);
	node.write_data_at(outfp, &own_wtyp,    19, 1);

	write_head(outfp, node, obj);


	slist_tpl<cstring_t> front_list;
	slist_tpl<cstring_t> back_list;

	for (ribi = 0; ribi < 16; ribi++) {
		char buf[40];
		sprintf(buf, "frontimage[%s]", ribi_codes[ribi]);
		cstring_t str = obj.get(buf);
		front_list.append(str);
		sprintf(buf, "backimage[%s]", ribi_codes[ribi]);
		cstring_t str2 = obj.get(buf);
		back_list.append(str2);
	}
	imagelist_writer_t::instance()->write_obj(outfp, node, front_list);
	imagelist_writer_t::instance()->write_obj(outfp, node, back_list);

	front_list.clear();
	back_list.clear();

	for (hang = 3; hang <= 12; hang += 3) {
		char buf[40];
		sprintf(buf, "frontimageup[%d]", hang);
		cstring_t str = obj.get(buf);
		front_list.append(str);
		sprintf(buf, "backimageup[%d]", hang);
		cstring_t str2 = obj.get(buf);
		back_list.append(str2);
	}
	imagelist_writer_t::instance()->write_obj(outfp, node, front_list);
	imagelist_writer_t::instance()->write_obj(outfp, node, back_list);

	front_list.clear();
	back_list.clear();

	for (ribi = 3; ribi <= 12; ribi += 3) {
		char buf[40];
		sprintf(buf, "frontdiagonal[%s]", ribi_codes[ribi]);
		cstring_t str = obj.get(buf);
		front_list.append(str);
		sprintf(buf, "backdiagonal[%s]", ribi_codes[ribi]);
		cstring_t str2 = obj.get(buf);
		back_list.append(str2);
	}
	imagelist_writer_t::instance()->write_obj(outfp, node, front_list);
	imagelist_writer_t::instance()->write_obj(outfp, node, back_list);

	slist_tpl<cstring_t> cursorkeys;
	cursorkeys.append(cstring_t(obj.get("cursor")));
	cursorkeys.append(cstring_t(obj.get("icon")));
	cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);

	// node.write_data(fp, &besch);
	node.write(outfp);
}
