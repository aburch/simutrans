//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  crossing_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: crossing_reader.cpp  $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2002/09/28 14:47:40 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: crossing_reader.cc,v $
//  Revision 1.3  2002/09/28 14:47:40  hajo
//  Hajo: restructurings
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

/////////////////////////////////////////////////////////////////////////////
//
//  static data
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "../../bauer/wegbauer.h"

#include "../kreuzung_besch.h"
#include "crossing_reader.h"

#include "../obj_node_info.h"

#include "../../simdebug.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      crossing_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      char *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void crossing_reader_t::register_obj(obj_besch_t *&data)
{
    kreuzung_besch_t *besch = static_cast<kreuzung_besch_t *>(data);

    wegbauer_t::register_besch(besch);
}



bool crossing_reader_t::successfully_loaded() const
{
    return wegbauer_t::alle_kreuzungen_geladen();
}




/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * crossing_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  kreuzung_besch_t *besch = new kreuzung_besch_t();
  besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

  // Hajo: Read data
  fread(besch_buf, node.size, 1, fp);
  char * p = besch_buf;

  // Hajo: old versions of PAK files have no version stamp.
  // But we know, the higher most bit was always cleared.

  const uint16 v = decode_uint16(p);
  const int version = v & 0x8000 ? v & 0x7FFF : 0;

  if(version == 0) {
    // old, nonversion node
    besch->wegtyp_ns = v;
    besch->wegtyp_ow = decode_uint16(p);
  }
  DBG_DEBUG("kreuzung_besch_t::read_node()","version=%i, ns=%d, ow=%d",v,besch->wegtyp_ns,besch->wegtyp_ow);
  return besch;
}
