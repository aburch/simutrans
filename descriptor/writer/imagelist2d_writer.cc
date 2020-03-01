/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../tpl/slist_tpl.h"
#include "obj_node.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"


void imagelist2d_writer_t::write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<slist_tpl<std::string> >& keys)
{
	obj_node_t node(this, 4, &parent);

	uint16 const count = keys.get_count();

	FOR(slist_tpl<slist_tpl<std::string> >, const& s, keys) {
		imagelist_writer_t::instance()->write_obj(fp, node, s);
	}
	node.write_uint16(fp, count, 0);
	node.write_uint16(fp, 0,     2);
	node.write(fp);
}
