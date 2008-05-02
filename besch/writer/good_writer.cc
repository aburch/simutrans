#include "../../dataobj/tabfile.h"

#include "../ware_besch.h"
#include "obj_node.h"
#include "text_writer.h"

#include "good_writer.h"


void good_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t	node(this, 10, &parent, false);

	write_head(fp, node, obj);
	text_writer_t::instance()->write_obj(fp, node, obj.get("metric"));

	uint16 value;
	uint8 val8;

	// Hajo: version number
	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	node.write_uint16(fp, 0x8003, 0);

	value = obj.get_int("value", 0);
	node.write_uint16(fp, value,  2);

	val8 = obj.get_int("catg", 0);
	node.write_uint8 (fp, val8,   4);

	value = obj.get_int("speed_bonus", 0);
	node.write_uint16(fp, value,  5);

	value = obj.get_int("weight_per_unit", 100);
	node.write_uint16(fp, value,  7);

	val8 = obj.get_int("mapcolor", 255);
	node.write_uint8 (fp, val8,   9);

	node.write(fp);
}
