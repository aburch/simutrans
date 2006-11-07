#include <stdio.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif
#include "../../simdebug.h"

#include "xref_reader.h"

#include "../obj_node_info.h"


obj_besch_t * xref_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	obj_besch_t *besch =  (obj_besch_t*)malloc( sizeof(obj_besch_t *)+node.size );
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch+1, node.size, 1, fp);

//	DBG_DEBUG("xref_reader_t::read_node()", "%s",besch->gib_text() );

	return besch;
}


void xref_reader_t::register_obj(obj_besch_t *&data)
{
    char *rest = reinterpret_cast<char *>(data + 1);
    obj_type *typ = (obj_type *)rest;
    bool fatal = rest[sizeof(obj_type)] != 0;
    char *text = rest + sizeof(obj_type) + 1;

    if(*text) {
	xref_to_resolve(*typ, text, &data, fatal);
    }
    else if(fatal) {
	xref_to_resolve(*typ, "", &data, fatal);
    } else {
	delete_node(data);
	data = NULL;
    }
}
