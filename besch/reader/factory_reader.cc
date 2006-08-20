//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  factory_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: factory_reader.cpp   $       $Author: hajo $
//  $Revision: 1.4 $         $Date: 2003/10/29 22:00:39 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: factory_reader.cc,v $
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

#include "../../simfab.h"
#include "../../bauer/fabrikbauer.h"
#include "../../dings/raucher.h"
#include "../../simdebug.h"
#include "../obj_node_info.h"
#include "../fabrik_besch.h"
#include "xref_reader.h"

#include "factory_reader.h"



/**
 * Read a factory product node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t *
factory_smoke_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif

	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];
	rauch_besch_t *besch = new rauch_besch_t();
	besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	sint16 x = decode_sint16(p);
	sint16 y = decode_sint16(p);
	besch->pos_off = koord( x, y );

	x = decode_sint16(p);
	y = decode_sint16(p);
	besch->xy_off = koord( x, y );

	besch->zeitmaske = decode_sint16(p);

	DBG_DEBUG("factory_product_reader_t::read_node()","zeitmaske=%d (size %i)",node.size);

	return besch;
}




///@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      factory_smoke_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void factory_smoke_reader_t::register_obj(obj_besch_t *&data)
{
    rauch_besch_t *besch = static_cast<rauch_besch_t *>(data);
    // Xref ist hier noch nicht aufgelöst!
    const char *name = xref_reader_t::get_name(besch->gib_kind(0));

    raucher_t::register_besch(besch, name);
    //printf("...Fabrik %s geladen\n", besch->gib_name());
}


/**
 * Read a factory product node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t *
factory_supplier_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
  // DBG_DEBUG("factory_product_reader_t::read_node()", "called");

#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif

  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  fabrik_lieferant_besch_t *besch = new fabrik_lieferant_besch_t();

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

    // not there yet ...
  } else {
    // old node, version 0
	besch->kapazitaet = v;
	besch->anzahl = decode_uint16(p);
	besch->verbrauch = decode_uint16(p);
  }

  DBG_DEBUG("factory_product_reader_t::read_node()",  "capacity=%d anzahl=%d, verbrauch=%d", version, besch->kapazitaet, besch->anzahl,besch->verbrauch);


  return besch;
}




/**
 * Read a factory product node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t *
factory_product_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
  // DBG_DEBUG("factory_product_reader_t::read_node()", "called");

#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  fabrik_produkt_besch_t *besch = new fabrik_produkt_besch_t();

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
    besch->kapazitaet = decode_uint16(p);
    besch->faktor = decode_uint16(p);
  } else {
    // old node, version 0

    decode_uint16(p);

    besch->kapazitaet = v;
    besch->faktor = 256;
  }

  DBG_DEBUG("factory_product_reader_t::read_node()",
	     "version=%d capacity=%d factor=%x",
	     version, besch->kapazitaet, besch->faktor);


  return besch;
}




/**
 * Read a factory product. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t *
factory_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
  // DBG_DEBUG("factory_reader_t::read_node()", "called");

#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif

  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  fabrik_besch_t *besch = new fabrik_besch_t();

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
    besch->platzierung = (enum fabrik_besch_t::platzierung)decode_uint16(p);
    besch->produktivitaet = decode_uint16(p);
    besch->bereich = decode_uint16(p);
    besch->gewichtung = decode_uint16(p);
    besch->kennfarbe = (uint8)decode_uint16(p);
    besch->lieferanten = decode_uint16(p);
    besch->produkte = decode_uint16(p);
    besch->pax_level = decode_uint16(p);
DBG_DEBUG("factory_reader_t::read_node()","version=1, platz=%i, lieferanten=%i, pax=%i", besch->platzierung, besch->lieferanten, besch->pax_level);
  } else {
    // old node, version 0, without pax_level
DBG_DEBUG("factory_reader_t::read_node()","version=0");
    besch->platzierung = (enum fabrik_besch_t::platzierung)v;
    decode_uint16(p);	// alsways zero
    besch->produktivitaet = decode_uint16(p)|0x8000;
    besch->bereich = decode_uint16(p);
    besch->gewichtung = decode_uint16(p);
    besch->kennfarbe = (uint8)decode_uint16(p);
    besch->lieferanten = decode_uint16(p);
    besch->produkte = decode_uint16(p);
    besch->pax_level = 12;
  }

  return besch;
}




//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      factory_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void factory_reader_t::register_obj(obj_besch_t *&data)
{
    fabrik_besch_t *besch = reinterpret_cast<fabrik_besch_t *>(data);
    fabrikbauer_t::register_besch(besch);
//    printf("...Fabrik %s geladen\n", besch->gib_name());
}
