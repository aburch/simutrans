//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  tree_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: tree_reader.cpp      $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: tree_reader.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>

#include "../../simdings.h"
#include "../../simdebug.h"
#include "../../dings/baum.h"

#include "../baum_besch.h"
#include "../obj_node_info.h"
#include "tree_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      tree_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void tree_reader_t::register_obj(obj_besch_t *&data)
{
    baum_besch_t *besch = static_cast<baum_besch_t *>(data);

    baum_t::register_besch(besch);
//    printf("...Baum %s geladen\n", besch->gib_name());
}


bool tree_reader_t::successfully_loaded() const
{
    return baum_t::alles_geladen();
}


/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * tree_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  baum_besch_t *besch = new baum_besch_t();

  besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

  // Hajo: Read data
  fread(besch_buf, node.size, 1, fp);

  char * p = besch_buf;

  // Hajo: old versions of PAK files have no version stamp.
  // But we know, the highest bit was always cleared.

  const uint16 v = decode_uint16(p);
  const int version = v & 0x8000 ? v & 0x7FFF : 0;

  if(version == 1) {
    // Versioned node, version 1

    besch->hoehenlage = (uint8)decode_uint8(p);
    besch->distribution_weight = (uint8)decode_uint8(p);
  } else {
    // old node, version 0
    besch->hoehenlage = (uint16)v;
    besch->distribution_weight = 3;
  }
DBG_DEBUG("tree_reader_t::read_node()", "hoehe=%i and weight=%i (ver=%i, node.size=%i)",besch->hoehenlage,besch->distribution_weight, version, node.size);

  return besch;
}


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
