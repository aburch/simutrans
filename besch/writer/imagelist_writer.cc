#include <string>
#include "../../tpl/slist_tpl.h"
#include "obj_node.h"
#include "image_writer.h"
#include "obj_pak_exception.h"
#include "imagelist_writer.h"


void imagelist_writer_t::write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<std::string>& keys)
{
	obj_node_t node(this, 4, &parent);

	unsigned int count = 0;
	FOR(slist_tpl<std::string>, const& s, keys) {
		image_writer_t::instance()->write_obj(fp, node, s);
		count ++;
	}
	if (count < keys.get_count()) {
		printf("WARNING: Expected %i images, but found only %i (but might be still correct)!\n", keys.get_count(), count);
		fflush(NULL);
	}

	node.write_uint16(fp, count, 0);
	node.write_uint16(fp, 0,     2);
	node.write(fp);
}
