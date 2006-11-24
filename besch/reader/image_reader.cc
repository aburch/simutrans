#include <stdio.h>
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

	if (besch->pic.len == 0) {
	delete_node(data);
	data = NULL;
    }
    else {
		register_image(&besch->pic);
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

	bild_besch_t* besch = new(node.size - 12) bild_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	besch->pic.x = decode_uint8(p);
	besch->pic.w = decode_uint8(p);
	besch->pic.y = decode_uint8(p);
	besch->pic.h = decode_uint8(p);
	besch->pic.len = decode_uint32(p);
	besch->pic.bild_nr = IMG_LEER;
	decode_uint16(p);	// dummys
	besch->pic.zoomable = decode_uint8(p);

//	DBG_DEBUG("bild_besch_t::read_node()","x,y=%d,%d  w,h=%d,%d",besch->x,besch->y,besch->w,besch->h);

	uint16* dest = besch->pic.data;
	p = besch_buf+12;
	if (besch->pic.h > 0) {
		for (uint i = 0; i < besch->pic.len; i++) {
			*dest++ = decode_uint16(p);
		}
//		DBG_DEBUG("bild_besch_t::read_node()","size %d, struct+2*len=%d, read=%d",node.size+4,sizeof(bild_besch_t)+2*besch->len,(int)(p-besch_buf));
	}

	return besch;
}
