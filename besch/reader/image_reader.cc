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
	ALLOCA(char, besch_buf, node.size);
	bild_besch_t* besch=NULL;

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf+6;

	// always zero in old version, since length was always less than 65535
	// because a node could not hold more data
	uint8 version = decode_uint8(p);
	p = besch_buf;
	//DBG_MESSAGE("bild_besch_t::read_node()","version %i",version);

	if(version==0) {
		besch = new(node.size - 12) bild_besch_t();
		besch->node_info = new obj_besch_t*[node.children];

		besch->pic.x = decode_uint8(p);
		besch->pic.w = decode_uint8(p);
		besch->pic.y = decode_uint8(p);
		besch->pic.h = decode_uint8(p);
		besch->pic.len = (uint16)decode_uint32(p);
		besch->pic.bild_nr = IMG_LEER;
		p += 2;	// dummys
		besch->pic.zoomable = decode_uint8(p);

		//DBG_DEBUG("bild_besch_t::read_node()","x,y=%d,%d  w,h=%d,%d, len=%i",besch->pic.x,besch->pic.y,besch->pic.w,besch->pic.h, besch->pic.len);

		uint16* dest = besch->pic.data;
		p = besch_buf+12;

		if (besch->pic.h > 0) {
			for (uint i = 0; i < besch->pic.len; i++) {
				uint16 data = decode_uint16(p);
				if(data>=0x8000u  &&  data<=0x800Fu) {
					// player color offset changed
					data ++;
				}
				*dest++ = data;
			}
		}
	}
	else if(version==1  ||  version==2) {
		besch = new(node.size - 10) bild_besch_t();
		besch->node_info = new obj_besch_t*[node.children];

		besch->pic.x = decode_uint16(p);
		besch->pic.y = decode_uint16(p);
		besch->pic.w = decode_uint8(p);
		besch->pic.h = decode_uint8(p);
		p++; // skip version information
		besch->pic.len = decode_uint16(p);
		besch->pic.zoomable = decode_uint8(p);
		besch->pic.bild_nr = IMG_LEER;

		uint16* dest = besch->pic.data;
		if (besch->pic.h > 0) {
			for (uint i = 0; i < besch->pic.len; i++) {
				*dest++ = decode_uint16(p);
			}
		}
	}
	else {
		dbg->fatal("image_reader_t::read_node()","illegal versions %d", version );
	}

	// check for left corner
	if(version<2) {
		// corect left border
		uint16 *dest = besch->pic.data;
		for( uint8 y=0;  y<besch->pic.h;  y++  ) {
			uint16 runlen = *dest;
			*dest++ -= besch->pic.x;
			assert(runlen>=besch->pic.x);
			// skip rest of the line
			do {
				runlen = *dest++;
				dest += runlen;
				runlen = *dest++;
			} while(runlen);
		}
	}

	return besch;
}
