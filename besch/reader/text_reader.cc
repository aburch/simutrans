#include <stdio.h>
#include "../../simdebug.h"

#include "../text_besch.h"

#include "text_reader.h"
#include "../obj_node_info.h"


obj_besch_t * text_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	text_besch_t* desc = new(node.size) text_besch_t();

	// Hajo: Read data
	fread(desc->text, node.size, 1, fp);

//	DBG_DEBUG("text_reader_t::read_node()", "%s",desc->get_text() );

	return desc;
}
