/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../tpl/slist_tpl.h"
#include "../image_array_3d.h"
#include "obj_node.h"
#include "imagelist_writer.h"
#include "imagelist3d_writer.h"
#include "imagelist2d_writer.h"

typedef slist_tpl<slist_tpl<slist_tpl<std::string> > > slist_3d;

void imagelist3d_writer_t::write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<slist_tpl<slist_tpl<std::string> > >& keys)
{
	image_array_3d_t desc;

	obj_node_t node(this, 4, &parent);

	desc.count = keys.get_count();

	FOR(slist_3d, const& i, keys)
	{
		imagelist2d_writer_t::instance()->write_obj(fp, node, i);
	}
	node.write_uint16(fp, desc.count, 0);
	node.write_uint16(fp, 0,            2);
	node.write(fp);
}
