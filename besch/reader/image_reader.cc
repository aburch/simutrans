#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../simgraph.h"
#include "../../simmem.h"
#include "../../simdebug.h"
#include "../../simimg.h"

#include "../bild_besch.h"
#include "image_reader.h"
#include "../obj_node_info.h"


void image_reader_t::register_obj(obj_besch_t *&data)
{
    bild_besch_t *besch = static_cast<bild_besch_t *>(data);

    /*
    printf("image_reader_t::register_obj():\t"
	   "x=%02d y=%02d w=%02d h=%02d len=%d\n",
	   besch->x,
	   besch->y,
	   besch->w,
	   besch->h,
	   besch->len);
    */

    if(!besch->len) {
	delete_node(data);
	data = NULL;
    }
    else {
	register_image(besch);
    }
}


obj_besch_t *  image_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
	char *besch_buf = static_cast<char *>(alloca(node.size));
#else
	// Hajo: reading buffer is better allocated on stack
	char besch_buf [node.size];
#endif

	void* info_buf = malloc(sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t*));

	bild_besch_t* besch = reinterpret_cast<bild_besch_t*>(malloc(sizeof(bild_besch_t) + node.size - 12));
	besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	besch->x = decode_uint8(p);
	besch->w = decode_uint8(p);
	besch->y = decode_uint8(p);
	besch->h = decode_uint8(p);
	besch->len = decode_uint32(p);
	besch->bild_nr = IMG_LEER;
	decode_uint16(p);	// dummys
	besch->zoomable = decode_uint8(p);

//	DBG_DEBUG("bild_besch_t::read_node()","x,y=%d,%d  w,h=%d,%d",besch->x,besch->y,besch->w,besch->h);

	uint16 *dest =(uint16 *)(besch+1);
	p = besch_buf+12;
	if(besch->h>0) {
		for( int i=0;  i<besch->len;  i++ ) {
			*dest++ = decode_uint16(p);
		}
//		DBG_DEBUG("bild_besch_t::read_node()","size %d, struct+2*len=%d, read=%d",node.size+4,sizeof(bild_besch_t)+2*besch->len,(int)(p-besch_buf));
	}

	return besch;
}
