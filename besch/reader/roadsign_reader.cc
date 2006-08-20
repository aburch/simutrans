/*
 *
 *  citycar_reader.cpp
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#include <stdio.h>

#include "../../tpl/stringhashtable_tpl.h"

#include "../../dings/roadsign.h"
#include "../../simvehikel.h"	// for khm to speed conversion
#include "../roadsign_besch.h"

#include "roadsign_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"

/*
 *  member function:
 *      citycar_reader_t::register_obj()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Argumente:
 *      obj_besch_t *&data
 */
void roadsign_reader_t::register_obj(obj_besch_t *&data)
{
    roadsign_besch_t *besch = static_cast<roadsign_besch_t *>(data);

    roadsign_t::register_besch(besch);
//    printf("...Stadtauto %s geladen\n", besch->gib_name());
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      citycar_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool roadsign_reader_t::successfully_loaded() const
{
    return roadsign_t::alles_geladen();
}



/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * roadsign_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif

  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  roadsign_besch_t *besch = new roadsign_besch_t();

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

    besch->min_speed = kmh_to_speed(decode_uint16(p));
    besch->flags = decode_uint8(p);
DBG_DEBUG("roadsign_reader_t::read_node()","min_speed=%i, flags=%x",besch->min_speed,besch->flags);
  }
  else {
  	dbg->fatal("roadsign_reader_t::read_node()","version 0 not supported. File corrupt?");
  }
  return besch;
}
