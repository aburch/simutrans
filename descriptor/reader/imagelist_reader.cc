/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"

#include "../image_list.h"

#include "imagelist_reader.h"
#include "../obj_node_info.h"
#include "../../tpl/array_tpl.h"


obj_desc_t * imagelist_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	image_list_t *desc = new image_list_t();
	desc->count = decode_uint16(p);

	return desc;
}
