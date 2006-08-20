//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  intlist_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: intlist_reader.cpp   $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:21 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: intlist_reader.cc,v $
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

#include "../intliste_besch.h"

#include "intlist_reader.h"
#include "../obj_node_info.h"

/////////////////////////////////////////////////////////////////////////////
//
//  static data
//
/////////////////////////////////////////////////////////////////////////////

/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
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
