#include <stdio.h>
#include "../../simdebug.h"

#include "../bildliste_besch.h"

#include "imagelist_reader.h"
#include "../obj_node_info.h"


obj_besch_t * imagelist_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	image_list_t *besch = new image_list_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	besch->count = decode_uint16(p);

//	DBG_DEBUG("imagelist_reader_t::read_node()", "count=%d data read (node.size=%i)",besch->count, node.size);

	return besch;
}
