/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

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

	if(  !keys.empty() &&  debuglevel>2  ) {
		dbg->debug("", "Source                                     image                X     Y     Off X Off Y Width Height Zoom\n");
		dbg->debug("", "------------------------------------------ -------------------- ----- ----- ----- ----- ----- ------ ----\n");
	}

	for(std::string const& s : keys) {
		image_writer_t::instance()->write_obj(fp, node, s, count);
		count ++;
	}
	if (count < keys.get_count()) {
		dbg->warning( obj_writer_t::last_name, "Expected %i but found %i images (might be correct)!\n", keys.get_count(), count);
	}

	node.write_uint16(fp, count, 0);
	node.write_uint16(fp, 0,     2);
	node.write(fp);
}
