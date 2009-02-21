#include "../../utils/cstring_t.h"
#include "../../tpl/slist_tpl.h"
#include "../bildliste2d_besch.h"
#include "obj_node.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"


void imagelist2d_writer_t::write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<slist_tpl<cstring_t> >& keys)
{
	bildliste2d_besch_t besch;

	obj_node_t node(this, 4, &parent);

	slist_iterator_tpl<slist_tpl<cstring_t> > iter(keys);

	besch.anzahl = keys.get_count();

	while (iter.next()) {
		imagelist_writer_t::instance()->write_obj(fp, node, iter.get_current());
	}
	node.write_uint16(fp, besch.anzahl, 0);
	node.write_uint16(fp, 0,            2);
	node.write(fp);
}
