/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>
#include "../../simdebug.h"
#include "../xref_desc.h"
#include "xref_reader.h"

#include "../obj_node_info.h"


obj_desc_t *xref_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	if (node.size < 5) {
		dbg->error("xref_reader_t::read_node", "node.size %u < 5", node.size);
		return NULL;
	}
	node_body_t p(fp, node.size, get_type_name());
	if (!p) return NULL;

	const uint32 name_len = node.size - 4 - 1;
	xref_desc_t* desc = new(name_len) xref_desc_t();
	desc->type = static_cast<obj_type>(decode_uint32(p));
	desc->fatal = (decode_uint8(p) != 0);
	memcpy(desc->name, p.read_bytes(name_len), name_len);

//	PAKSET_INFO("xref_reader_t::read_node()", "%s",desc->get_text() );

	return desc;
}


void xref_reader_t::register_obj(obj_desc_t *&data)
{
	xref_desc_t* desc = static_cast<xref_desc_t*>(data);

	if (desc->name[0] != '\0' || desc->fatal) {
		pakset_manager_t::xref_to_resolve(desc->type, desc->name, &data, desc->fatal);
	}
	else {
		delete data;
		data = NULL;
	}
}
