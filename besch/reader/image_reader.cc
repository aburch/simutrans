//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  image_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: image_reader.cpp     $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2003/02/02 10:15:41 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: image_reader.cc,v $
//  Revision 1.3  2003/02/02 10:15:41  hajo
//  Hajo: sync for 0.81.21exp
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>
#include <string.h>

#include "../../simgraph.h"
#include "../../simmem.h"
#include "../../simdebug.h"
#include "../../simimg.h"

#include "../bild_besch.h"
#include "image_reader.h"
#include "../obj_node_info.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      image_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      char *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
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


/**
 * Read an info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t *  image_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
	char *besch_buf = static_cast<char *>(alloca(node.size));
#else
	// Hajo: reading buffer is better allocated on stack
	char besch_buf [node.size];
#endif

	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

	bild_besch_t *besch = reinterpret_cast<bild_besch_t *>(new char[ sizeof(bild_besch_t)+node.size-12 ]);
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
