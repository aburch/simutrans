#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../roadsign_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"

#include "roadsign_writer.h"
#include "get_waytype.h"
#include "skin_writer.h"

void roadsign_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
	obj_node_t	node(this, 14, &parent, false);	/* false, because we write this ourselves */

	// Hajodoc: Preferred height of this tree type
	// Hajoval: int (useful range: 0-14)
	roadsign_besch_t besch;
	besch.cost = obj.get_int("cost", 500)*100;
	besch.min_speed = obj.get_int("min_speed", 0);
	besch.flags = (obj.get_int("single_way", 0)>0) + (obj.get_int("free_route", 0)>0)*2 + (obj.get_int("is_private", 0)>0)*4 +
						(obj.get_int("is_signal", 0)>0)*8 + (obj.get_int("is_presignal", 0)>0)*16 + (obj.get_int("no_foreground", 0)>0)*32;
	besch.wtyp =  get_waytype(obj.get("waytype"));

	// Hajo: temp vars of appropriate size
	uint32 v32;
	uint16 v16;
	uint8 v8;

	// Hajo: write version data
	v16 = 0x8003;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	v16 = (uint16) besch.min_speed;
	node.write_data_at(fp, &v16, 2, sizeof(uint16));

	v32 = (uint32) besch.cost;
	node.write_data_at(fp, &v32, 4, sizeof(uint32));

	v8 = (uint8)besch.flags;
	node.write_data_at(fp, &v8, 8, sizeof(uint8));

	v8 = (uint8)besch.wtyp;
	node.write_data_at(fp, &v8, 9, sizeof(uint8));

	uint16 intro  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro += obj.get_int("intro_month", 1) - 1;
	node.write_data_at(fp, &intro, 10, sizeof(uint16));

	uint16 retire  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire += obj.get_int("retire_month", 1) - 1;
	node.write_data_at(fp, &retire, 12, sizeof(uint16));

	write_head(fp, node, obj);

	// add the images
	slist_tpl<cstring_t> keys;
	cstring_t str;

	for(int i=0; i<24; i++) {
		char buf[40];

		sprintf(buf, "image[%i]", i);
		str = obj.get(buf);
		// make sure, there are always 4, 8, 12, ... images (for all directions)
		if(str.len()==0  &&  (i%4)==0) {
			break;
		}
		keys.append(str);
	}
	imagelist_writer_t::instance()->write_obj(fp, node, keys);

	// probably add some icons, if defined
	slist_tpl<cstring_t> cursorkeys;

	cstring_t c=cstring_t(obj.get("cursor")), i=cstring_t(obj.get("icon"));
	cursorkeys.append(c);
	cursorkeys.append(i);
	if(c.len()>0  ||  i.len()>0) {
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	}

	node.write(fp);
}
