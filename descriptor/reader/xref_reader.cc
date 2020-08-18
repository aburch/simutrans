#include <stdio.h>
#include "../../simdebug.h"
#include "../xref_desc.h"
#include "xref_reader.h"

#include "../obj_node_info.h"


obj_desc_t * xref_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	xref_desc_t* desc = new(node.size - 4 - 1) xref_desc_t();

	char buf[4 + 1];
	fread(buf, 1, 5, fp);
	char* p = buf;
	desc->type = static_cast<obj_type>(decode_uint32(p));
	desc->fatal = (decode_uint8(p) != 0);
	fread(desc->name, 1, node.size - 4 - 1, fp);

//	DBG_DEBUG("xref_reader_t::read_node()", "%s",desc->get_text() );

	return desc;
}


void xref_reader_t::register_obj(obj_desc_t *&data)
{
	xref_desc_t* desc = static_cast<xref_desc_t*>(data);

	if (desc->name[0] != '\0' || desc->fatal) {
		xref_to_resolve(desc->type, desc->name, &data, desc->fatal);
	} else {
		delete data;
		data = NULL;
	}
}
