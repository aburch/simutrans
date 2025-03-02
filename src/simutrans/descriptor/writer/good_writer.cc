/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../dataobj/tabfile.h"

#include "../goods_desc.h"
#include "obj_node.h"
#include "text_writer.h"

#include "good_writer.h"


void goods_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 16, &parent);

	write_name_and_copyright(fp, node, obj);
	text_writer_t::instance()->write_obj(fp, node, obj.get("metric"));

	node.write_version(fp, 4);

	node.write_sint64(fp, obj.get_int64("value", 0));
	node.write_uint8 (fp, obj.get_int("catg", 0));
	node.write_uint16(fp, obj.get_int("speed_bonus", 0));
	node.write_uint16(fp, obj.get_int("weight_per_unit", 100));
	node.write_uint8 (fp, obj.get_int("mapcolor", 255));

	node.check_and_write_header(fp);
}
