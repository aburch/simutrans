#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "../grund_besch.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"
#include "ground_writer.h"


void ground_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 0, &parent);

	write_head(fp, node, obj);

	slist_tpl<slist_tpl<std::string> > keys;
	// summer images
	for (int hangtyp = 0; hangtyp < 128; hangtyp++) {
		keys.append(slist_tpl<std::string>());

		for (int phase = 0; ; phase++) {
			char buf[40];

			sprintf(buf, "image[%d][%d]", hangtyp, phase);

			std::string str = obj.get(buf);
			if (str.size() == 0) {
				break;
			}
			keys.at(hangtyp).append(str);
		}
		// empty entries?
		if (keys.at(hangtyp).empty()) {
			break;
		}
	}
	imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

	node.write(fp);
}
