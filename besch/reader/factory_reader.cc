#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "../../simfab.h"
#include "../../bauer/fabrikbauer.h"
#include "../../dings/wolke.h"
#include "../../simdebug.h"
#include "../obj_node_info.h"
#include "../fabrik_besch.h"
#include "../xref_besch.h"

#include "factory_reader.h"


obj_besch_t *
factory_smoke_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif

	rauch_besch_t *besch = new rauch_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

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


void factory_smoke_reader_t::register_obj(obj_besch_t *&data)
{
	rauch_besch_t *besch = static_cast<rauch_besch_t *>(data);
	// Xref ist hier noch nicht aufgelöst!
	const char* name = static_cast<xref_besch_t*>(besch->gib_kind(0))->get_name();

	wolke_t::register_besch(besch, name);
}


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

  fabrik_lieferant_besch_t *besch = new fabrik_lieferant_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

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

  fabrik_produkt_besch_t *besch = new fabrik_produkt_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

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

  DBG_DEBUG("factory_product_reader_t::read_node()", "version=%d capacity=%d factor=%x", version, besch->kapazitaet, besch->faktor);
  return besch;
}


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

  fabrik_besch_t *besch = new fabrik_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

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


void factory_reader_t::register_obj(obj_besch_t *&data)
{
	fabrik_besch_t* besch = static_cast<fabrik_besch_t*>(data);
	fabrikbauer_t::register_besch(besch);
}
