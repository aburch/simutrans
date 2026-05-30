/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"

#include "../image_array.h"

#include "imagelist2d_reader.h"
#include "../obj_node_info.h"


obj_desc_t * imagelist2d_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	node_body_t p(fp, node.size, get_type_name());
	if (!p) return NULL;

	image_array_t *desc = new image_array_t();
	desc->count = decode_uint16(p);

//	PAKSET_INFO("imagelist2d_reader_t::read_node()", "count=%d data read (node.size=%i)",desc->count, node.size);

	return desc;
}
