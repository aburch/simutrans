#include <stdio.h>
#include "../../simdebug.h"

#include "../bildliste2d_besch.h"

#include "imagelist2d_reader.h"
#include "../obj_node_info.h"


obj_besch_t * imagelist2d_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	bildliste2d_besch_t *besch = new bildliste2d_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	besch->anzahl = decode_uint16(p);

//	DBG_DEBUG("imagelist2d_reader_t::read_node()", "count=%d data read (node.size=%i)",besch->anzahl, node.size);

	return besch;
}
