#include <stdio.h>
#include "../../simdebug.h"
#include "../xref_besch.h"
#include "xref_reader.h"

#include "../obj_node_info.h"


obj_besch_t * xref_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	xref_besch_t* besch = new(node.size - 4 - 1) xref_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	char buf[4 + 1];
	fread(buf, 1, 5, fp);
	char* p = buf;
	besch->type = static_cast<obj_type>(decode_uint32(p));
	besch->fatal = (decode_uint8(p) != 0);
	fread(besch->name, 1, node.size - 4 - 1, fp);

//	DBG_DEBUG("xref_reader_t::read_node()", "%s",besch->gib_text() );

	return besch;
}


void xref_reader_t::register_obj(obj_besch_t *&data)
{
	xref_besch_t* besch = static_cast<xref_besch_t*>(data);

	if (besch->name[0] != '\0') {
		xref_to_resolve(besch->type, besch->name, &data, besch->fatal);
	} else if (besch->fatal) {
		xref_to_resolve(besch->type, "", &data, besch->fatal);
	} else {
		delete_node(data);
		data = NULL;
	}
}
