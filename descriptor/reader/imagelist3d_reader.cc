#include <stdio.h>
#include "../../simdebug.h"

#include "../image_array_3d.h"

#include "imagelist3d_reader.h"
#include "../obj_node_info.h"


obj_desc_t * imagelist3d_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	image_array_3d_t *desc = new image_array_3d_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	desc->count = decode_uint16(p);

	return desc;
}
