/*
 * vehicle_reader.cc
 *
 * Copyright (c) 2002 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "../../simdebug.h"
#include "../../bauer/vehikelbauer.h"
#include "../vehikel_besch.h"

#include "vehicle_reader.h"
#include "../obj_node_info.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      vehicle_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void vehicle_reader_t::register_obj(obj_besch_t *&data)
{
    vehikel_besch_t *besch = static_cast<vehikel_besch_t *>(data);

    vehikelbauer_t::register_besch(besch);

    obj_for_xref(get_type(), besch->gib_name(), data);
//    printf("...Vehikel %s geladen\n", besch->gib_name());
}

bool vehicle_reader_t::successfully_loaded() const
{
    vehikelbauer_t::sort_lists();
    return true;
}


/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  vehikel_besch_t *besch = new vehikel_besch_t();

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

    besch->preis = decode_uint32(p);
    besch->zuladung = decode_uint16(p);
    besch->geschw = decode_uint16(p);
    besch->gewicht = decode_uint16(p);
    besch->leistung = decode_uint16(p);
    besch->betriebskosten = decode_uint16(p);

    besch->intro_date = decode_uint16(p);
    besch->gear = decode_uint8(p);

    besch->typ = decode_uint8(p);
    besch->sound = decode_sint8(p);
    besch->vorgaenger = decode_uint8(p);
    besch->nachfolger = decode_uint8(p);

    if(besch->typ == 4) {
      besch->engine_type = vehikel_besch_t::electric;
      besch->typ = vehikel_besch_t::schiene;
    } else {
      besch->engine_type = vehikel_besch_t::diesel;
    }

    besch->obsolete_date = (2999*16);
  } else if(version == 2) {
    // Versioned node, version 2

    besch->preis = decode_uint32(p);
    besch->zuladung = decode_uint16(p);
    besch->geschw = decode_uint16(p);
    besch->gewicht = decode_uint16(p);
    besch->leistung = decode_uint16(p);
    besch->betriebskosten = decode_uint16(p);

    besch->intro_date = decode_uint16(p);
    besch->gear = decode_uint8(p);

    besch->typ = decode_uint8(p);
    besch->sound = decode_sint8(p);
    besch->vorgaenger = decode_uint8(p);
    besch->nachfolger = decode_uint8(p);
    besch->engine_type = decode_uint8(p);

    // Hajo: compatibility for old dat file content
    if(besch->typ == 4) {
      besch->typ = vehikel_besch_t::schiene;
    }

    besch->obsolete_date = (2999*16);
} else if (version == 3 ) {
    // Versioned node, version 3 with retire date

    besch->preis = decode_uint32(p);
    besch->zuladung = decode_uint16(p);
    besch->geschw = decode_uint16(p);
    besch->gewicht = decode_uint16(p);
    besch->leistung = decode_uint16(p);
    besch->betriebskosten = decode_uint16(p);

    besch->intro_date = decode_uint16(p);
    besch->obsolete_date = decode_uint16(p);
    besch->gear = decode_uint8(p);

    besch->typ = decode_uint8(p);
    besch->sound = decode_sint8(p);
    besch->vorgaenger = decode_uint8(p);
    besch->nachfolger = decode_uint8(p);
    besch->engine_type = decode_uint8(p);
} else {
    // old node, version 0

    besch->typ = v;
    besch->zuladung = decode_uint16(p);
    besch->preis = decode_uint32(p);
    besch->geschw = decode_uint16(p);
    besch->gewicht = decode_uint16(p);
    besch->leistung = decode_uint16(p);
    besch->betriebskosten = decode_uint16(p);
    besch->sound = decode_sint16(p);
    besch->vorgaenger = decode_uint16(p);
    besch->nachfolger = decode_uint16(p);

    besch->intro_date = 0;
    besch->obsolete_date = (2999*16);
    besch->gear = 64;

    if(besch->typ == 4) {
      besch->engine_type = vehikel_besch_t::electric;
      besch->typ = vehikel_besch_t::schiene;
    } else {
      besch->engine_type = vehikel_besch_t::diesel;
    }
  }

  DBG_DEBUG("vehicle_reader_t::read_node()",
	     "version=%d "
	     "typ=%d zuladung=%d preis=%d geschw=%d gewicht=%d leistung=%d "
	     "betrieb=%d sound=%d vor=%d nach=%d "
	     "date=%d gear=%d engine_type=%d",
	     version,
	     besch->typ,
	     besch->zuladung,
	     besch->preis,
	     besch->geschw,
	     besch->gewicht,
	     besch->leistung,
	     besch->betriebskosten,
	     besch->sound,
	     besch->vorgaenger,
	     besch->nachfolger,
	     besch->intro_date,
	     besch->gear,
	     besch->engine_type);

  return besch;
}


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
