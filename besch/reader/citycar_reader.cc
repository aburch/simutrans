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

#include "../../simverkehr.h"
#include "../stadtauto_besch.h"

#include "citycar_reader.h"
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
void citycar_reader_t::register_obj(obj_besch_t *&data)
{
    stadtauto_besch_t *besch = static_cast<stadtauto_besch_t *>(data);

    stadtauto_t::register_besch(besch);
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
bool citycar_reader_t::successfully_loaded() const
{
    return stadtauto_t::laden_erfolgreich();
}



/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * citycar_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  stadtauto_besch_t *besch = new stadtauto_besch_t();

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

    besch->gewichtung = decode_uint16(p);
    besch->geschw = decode_uint16(p);
    besch->intro_date = decode_uint16(p);
    besch->obsolete_date = decode_uint16(p);
  }
  else {
    besch->gewichtung = v;
    besch->geschw = 80;
    besch->intro_date = 1900*16;
    besch->obsolete_date = 2999*16;
  }
DBG_DEBUG("citycar_reader_t::read_node()","version=%i, weight=%i, intro=%i.%i, retire=%i,%i",
	version,besch->gewichtung,besch->intro_date&15+1,besch->intro_date/16,besch->obsolete_date&15+1,besch->obsolete_date/16);
  return besch;
}
