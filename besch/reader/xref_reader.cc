//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  xref_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: xref_reader.cpp      $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2003/06/29 10:33:38 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: xref_reader.cc,v $
//  Revision 1.3  2003/06/29 10:33:38  hajo
//  Hajo: added Volkers changes
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif
#include "../../simdebug.h"

#include "xref_reader.h"

#include "../obj_node_info.h"


/**
 * Read a node. Does version check and
 * compatibility transformations.
 * not really needed, since it is only a bytewise string, but there for future compatibility
 * @author Hj. Malthaner
 */
obj_besch_t * xref_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];
	obj_besch_t *besch =  (obj_besch_t*)malloc( sizeof(obj_besch_t *)+node.size );
	besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

	// Hajo: Read data
	fread(besch+1, node.size, 1, fp);

//	DBG_DEBUG("xref_reader_t::read_node()", "%s",besch->gib_text() );

	return besch;
}



//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      xref_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
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
