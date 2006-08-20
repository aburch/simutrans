//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  building_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: building_reader.cpp  $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2003/11/22 16:53:50 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: building_reader.cc,v $
//  Revision 1.3  2003/11/22 16:53:50  hajo
//  Hajo: integrated Hendriks changes
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>

#include "../../simdebug.h"
#include "../haus_besch.h"
#include "../obj_node_info.h"
#include "building_reader.h"



void building_reader_t::register_obj(obj_besch_t *&data)
{
    haus_besch_t *besch = static_cast<haus_besch_t *>(data);

    hausbauer_t::register_besch(besch);
    dbg->debug("building_reader_t::register_obj", "Loaded '%s'\n", besch->gib_name());
}



//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      building_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool building_reader_t::successfully_loaded() const
{
    return hausbauer_t::alles_geladen();
}


/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * building_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
  // dbg->debug("good_reader_t::read_node()", "called");

  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  haus_besch_t *besch = new haus_besch_t();

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

    besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
    besch->utyp      = (enum hausbauer_t::utyp)decode_uint8(p);
    besch->level     = decode_uint16(p);
    besch->bauzeit   = decode_uint32(p);
    besch->groesse.x = decode_uint16(p);
    besch->groesse.y = decode_uint16(p);
    besch->layouts   = decode_uint8(p);
    besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
    besch->chance    = decode_uint8(p);


  } else {
    // old node, version 0
    besch->gtyp      = (enum gebaeude_t::typ)v;
    decode_uint16(p);
    besch->utyp      = (enum hausbauer_t::utyp)decode_uint32(p);
    besch->level     = decode_uint32(p);
    besch->bauzeit   = decode_uint32(p);
    besch->groesse.x = decode_uint16(p);
    besch->groesse.y = decode_uint16(p);
    besch->layouts   = decode_uint32(p);
    besch->flags     = (enum haus_besch_t::flag_t)decode_uint32(p);
    besch->chance    = 100;
  }

  dbg->debug("building_reader_t::read_node()",
	     "version=%d"
	     " gtyp=%d"
	     " utyp=%d"
	     " level=%d"
	     " bauzeit=%d"
	     " groesse.x=%d"
	     " groesse.y=%d"
	     " layouts=%d"
	     " flags=%d"
	     " chance=%d",
 	     version,
	     besch->gtyp,
	     besch->utyp,
	     besch->level,
	     besch->bauzeit,
	     besch->groesse.x,
	     besch->groesse.y,
	     besch->layouts,
	     besch->flags,
	     besch->chance
	     );

  return besch;

}



/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
