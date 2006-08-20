//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  imagelist_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: imagelist_reader.cpp $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:21 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: imagelist_reader.cc,v $
//  Revision 1.1  2002/09/18 19:13:21  hajo
//  Volker: new config system
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

#include "../text_besch.h"

#include "text_reader.h"
#include "../obj_node_info.h"


/////////////////////////////////////////////////////////////////////////////
//
//  static data
//
/////////////////////////////////////////////////////////////////////////////

/**
 * Read a node. Does version check and
 * compatibility transformations.
 * not really needed, since it is only a bytewise string, but there for future compatibility
 * @author Hj. Malthaner
 */
obj_besch_t * text_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];
	text_besch_t *besch =  (text_besch_t*)malloc( sizeof(text_besch_t*)+node.size );
	besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

	// Hajo: Read data
	fread(besch+1, node.size, 1, fp);

//	DBG_DEBUG("text_reader_t::read_node()", "%s",besch->gib_text() );

	return besch;
}
