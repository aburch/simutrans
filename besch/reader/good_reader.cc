//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  good_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: good_reader.cpp      $       $Author: hajo $
//  $Revision: 1.6 $         $Date: 2004/10/30 09:20:49 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: good_reader.cc,v $
//  Revision 1.6  2004/10/30 09:20:49  hajo
//  sync for Dario
//
//  Revision 1.5  2003/12/10 19:41:45  hajo
//  Hajo: sync for Hendrik
//
//  Revision 1.4  2003/10/29 22:00:39  hajo
//  Hajo: sync for Hendrik Siegeln
//
//  Revision 1.3  2002/09/28 14:47:40  hajo
//  Hajo: restructurings
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "../../simdebug.h"
#include "../../simware.h"
#include "../../bauer/warenbauer.h"

#include "good_reader.h"
#include "../obj_node_info.h"
#include "../ware_besch.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      good_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void good_reader_t::register_obj(obj_besch_t *&data)
{
  ware_besch_t *besch = static_cast<ware_besch_t *>(data);

  warenbauer_t::register_besch(besch);
  DBG_DEBUG("good_reader_t::register_obj()",
	     "loaded good '%s'", besch->gib_name());

  obj_for_xref(get_type(), besch->gib_name(), data);
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      good_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool good_reader_t::successfully_loaded() const
{
    return warenbauer_t::alles_geladen();
}



/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * good_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
  // DBG_DEBUG("good_reader_t::read_node()", "called");

#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  ware_besch_t *besch = new ware_besch_t();

  besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

  // Hajo: Read data
  fread(besch_buf, node.size, 1, fp);

  char * p = besch_buf;

  // Hajo: old versions of PAK files have no version stamp.
  // But we know, the higher most bit was always cleared.

  const uint16 v = decode_uint16(p);
  const int version = v & 0x8000 ? v & 0x7FFF : 0;

  if(version == 1) {
    // Versioned node, version 1

    besch->value = decode_uint16(p);
    besch->catg = decode_uint16(p);
    besch->speed_bonus = decode_uint16(p);
    besch->weight_per_unit = 100;

  } else if(version == 2) {
    // Versioned node, version 2

    besch->value = decode_uint16(p);
    besch->catg = decode_uint16(p);
    besch->speed_bonus = decode_uint16(p);
    besch->weight_per_unit = decode_uint16(p);

  } else {
    // old node, version 0

    besch->value = v;
    besch->catg = decode_uint16(p);
    besch->speed_bonus = 0;
    besch->weight_per_unit = 100;
  }

  DBG_DEBUG("good_reader_t::read_node()",
	     "version=%d value=%d catg=%d bonus=%d",
	     version, besch->value, besch->catg, besch->speed_bonus);


  return besch;
}
