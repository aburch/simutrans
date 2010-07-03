#include <string>
#include "../../dataobj/tabfile.h"
#include "../stadtauto_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "citycar_writer.h"


void citycar_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	stadtauto_besch_t besch;
	int i;

	obj_node_t node(this, 10, &parent);

	besch.gewichtung = obj.get_int("distributionweight", 1);

	besch.intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	besch.intro_date += obj.get_int("intro_month", 1) - 1;

	besch.obsolete_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	besch.obsolete_date += obj.get_int("retire_month", 1) - 1;

	besch.geschw  = obj.get_int("speed", 80) * 16;

	// new version with intro and obsolete dates
	node.write_uint16(fp, 0x8002,                    0); // version information
	node.write_uint16(fp, (uint16) besch.gewichtung, 2);
	node.write_uint16(fp, (uint16) besch.geschw,     4);
	node.write_uint16(fp, besch.intro_date,          6);
	node.write_uint16(fp, besch.obsolete_date,       8);

	write_head(fp, node, obj);

	static const char* const dir_codes[] = {
		"s", "w", "sw", "se", "n", "e", "ne", "nw"
	};
	slist_tpl<std::string> keys;
	std::string str;

	for (i = 0; i < 8; i++) {
		char buf[40];

		sprintf(buf, "image[%s]", dir_codes[i]);
		str = obj.get(buf);
		keys.append(str);
	}
	imagelist_writer_t::instance()->write_obj(fp, node, keys);

	node.write(fp);
}
