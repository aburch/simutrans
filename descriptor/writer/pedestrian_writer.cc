#include <string>
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "pedestrian_writer.h"


void pedestrian_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int i;

	obj_node_t node(this, 6, &parent);

	write_head(fp, node, obj);

	uint16 const dist_weight = obj.get_int("distributionweight", 1);

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

	uint16 intro_date = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro_date += obj.get_int("intro_month", 1) - 1;

	uint16 retire_date = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	retire_date += obj.get_int("retire_month", 1) - 1;

	node.write_uint16(fp, dist_weight,					 0);
	node.write_uint16(fp, intro_date,				     2); // Formerly dummy
	node.write_uint16(fp, retire_date,				 4); 

	node.write(fp);
}
