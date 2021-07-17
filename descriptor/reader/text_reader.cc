/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"

#include "../text_desc.h"

#include "text_reader.h"
#include "../obj_node_info.h"


obj_desc_t *text_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	text_desc_t *desc = new(node.size) text_desc_t();

	// Read data
	if (fread(desc->text, node.size, 1, fp) != 1) {
		delete desc;
		return NULL;
	}

//	DBG_DEBUG("text_reader_t::read_node()", "%s",desc->get_text() );

	return desc;
}
