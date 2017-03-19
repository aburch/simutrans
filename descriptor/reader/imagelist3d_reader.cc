#include <stdio.h>
#include "../../simdebug.h"

#include "../bildliste3d_besch.h"

#include "imagelist3d_reader.h"
#include "../obj_node_info.h"


obj_besch_t * imagelist3d_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	bildliste3d_besch_t *besch = new bildliste3d_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	besch->anzahl = decode_uint16(p);

//	DBG_DEBUG("imagelist3d_reader_t::read_node()", "count=%d data read (node.size=%i)",besch->anzahl, node.size);

	return besch;
}
