/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"

#include "../image_list.h"

#include "imagelist_reader.h"
#include "../obj_node_info.h"


obj_desc_t * imagelist_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	node_body_t p(fp, node.size, get_type_name());
	if (!p) return NULL;

	image_list_t *desc = new image_list_t();
	desc->count = decode_uint16(p);

	return desc;
}
