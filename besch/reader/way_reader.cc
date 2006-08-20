
#include <stdio.h>

#include "../../simdebug.h"
#include "../../utils/simstring.h"

#include "../weg_besch.h"
#include "../../bauer/wegbauer.h"

#include "way_reader.h"
#include "../obj_node_info.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      way_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void way_reader_t::register_obj(obj_besch_t *&data)
{
    weg_besch_t *besch = static_cast<weg_besch_t *>(data);

    wegbauer_t::register_besch(besch);
//    printf("...Weg %s geladen\n", besch->gib_name());
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      way_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool way_reader_t::successfully_loaded() const
{
    return wegbauer_t::alle_wege_geladen();
}


/**
 * Read a way info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * way_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
  // dbg->debug("way_reader_t::read_node()", "called");

  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  weg_besch_t *besch = new weg_besch_t();

  besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

  // dbg->debug("way_reader_t::read_node()", "node size = %d", node.size);

  // Hajo: Read data
  fread(besch_buf, node.size, 1, fp);

  char * p = besch_buf;

  // Hajo: old versions of PAK files have no version stamp.
  // But we know, the higher most bit was always cleared.

  int version = 0;

  if(node.size == 0) {
    // old node, version 0, compatibility code

    besch->price = 10000;
    besch->maintenance = 800;
    besch->topspeed = 999;
    besch->max_weight = 999;
    besch->intro_date = 1;
    besch->wtyp = weg_t::strasse;
    besch->styp = 0;

    /*
    if(tstrequ(besch->gib_name(), "road")) {
      besch->wtyp = weg_t::strasse;
      besch->max_speed = 130;
    } else {
      besch->wtyp = weg_t::schiene;
      besch->max_speed = 450;
    }
    */

  } else {

    const uint16 v = decode_uint16(p);
    version = v & 0x7FFF;

    if(version == 1) {
      // Versioned node, version 1

      besch->price = decode_uint32(p);
      besch->maintenance = decode_uint32(p);
      besch->topspeed = decode_uint32(p);
      besch->max_weight = decode_uint32(p);
      besch->intro_date = decode_uint32(p);
      besch->wtyp = decode_uint8(p);
      besch->styp = decode_uint8(p);

    } else {
      dbg->fatal("way_reader_t::read_node()",
		 "Invalid version %d",
		 version);
    }
  }

  dbg->debug("way_reader_t::read_node()",
	     "version=%d price=%d maintenance=%d topspeed=%d max_weight=%d "
	     "wtype=%d styp=%d",
	     version,
	     besch->price,
	     besch->maintenance,
	     besch->topspeed,
	     besch->max_weight,
	     besch->wtyp,
	     besch->styp);

  return besch;
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
