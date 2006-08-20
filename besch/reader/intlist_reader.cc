#include <stdio.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif
#include "../../simdebug.h"

#include "../intliste_besch.h"

#include "intlist_reader.h"
#include "../obj_node_info.h"


obj_besch_t * intlist_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif

	// if there are any children, we need space for them ...
	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	uint16 anzahl = decode_uint16(p);
	intliste_besch_t *besch = (intliste_besch_t *)malloc( (anzahl+1)*sizeof(anzahl)+sizeof(intliste_besch_t *) );
	besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

	// convert data
	uint16 *daten = &(besch->anzahl);
	for( int i=1;  i<=anzahl;  i++ ) {
		daten[i] = decode_uint16(p);
	}

	DBG_DEBUG("intlist_reader_t::read_node()", "count=%d data read (node.size=%i)",besch->anzahl, node.size);

	return besch;
}
