
#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

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
  // DBG_DEBUG("way_reader_t::read_node()", "called");

#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  weg_besch_t *besch = new weg_besch_t();

  besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

  // DBG_DEBUG("way_reader_t::read_node()", "node size = %d", node.size);

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
    besch->intro_date = DEFAULT_INTRO_DATE*12;
    besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
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

    if(version == 2) {
      // Versioned node, version 2

      besch->price = decode_uint32(p);
      besch->maintenance = decode_uint32(p);
      besch->topspeed = decode_uint32(p);
      besch->max_weight = decode_uint32(p);
      besch->intro_date = decode_uint16(p);
      besch->obsolete_date = decode_uint16(p);
      besch->wtyp = decode_uint8(p);
      besch->styp = decode_uint8(p);

    } else if(version == 1) {
      // Versioned node, version 1

      besch->price = decode_uint32(p);
      besch->maintenance = decode_uint32(p);
      besch->topspeed = decode_uint32(p);
      besch->max_weight = decode_uint32(p);
      uint32 intro_date= decode_uint32(p);
      besch->intro_date = (intro_date/16)*12 + (intro_date%16);
      besch->wtyp = decode_uint8(p);
      besch->styp = decode_uint8(p);
      besch->obsolete_date = DEFAULT_RETIRE_DATE*12;

    } else {
      dbg->fatal("way_reader_t::read_node()","Invalid version %d", version);
    }
  }
  if(besch->wtyp==weg_t::schiene_strab) {
  	besch->styp = 7;
  	besch->wtyp = weg_t::schiene;
  }
  else if(besch->styp==5  &&  besch->wtyp==weg_t::schiene) {
  	besch->wtyp = weg_t::monorail;
  	besch->styp = 0;
  }
  if(version<=2  &&  besch->wtyp==weg_t::luft  &&  besch->topspeed>=250) {
  	// runway!
  	besch->styp = 1;
  }

  DBG_DEBUG("way_reader_t::read_node()",
	     "version=%d price=%d maintenance=%d topspeed=%d max_weight=%d "
	     "wtype=%d styp=%d intro_year=%i",
	     version,
	     besch->price,
	     besch->maintenance,
	     besch->topspeed,
	     besch->max_weight,
	     besch->wtyp,
	     besch->styp,
	     besch->intro_date/12);

  return besch;
}
