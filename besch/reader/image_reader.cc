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

#include "../../simgraph.h"
#include "../bild_besch.h"
#include "image_reader.h"


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
    else
	register_image(besch);
}
