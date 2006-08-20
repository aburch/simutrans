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
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "../../simdebug.h"
#include "../haus_besch.h"
#include "../obj_node_info.h"
#include "building_reader.h"



/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * tile_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
	char *besch_buf = static_cast<char *>(alloca(node.size));
#else
	// Hajo: reading buffer is better allocated on stack
	char besch_buf [node.size];
#endif

	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

	haus_tile_besch_t *besch = new haus_tile_besch_t();

	besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = (v & 0x8000)!=0 ? v&0x7FFF : 0;

	if(version == 1) {
//  DBG_DEBUG("tile_reader_t::read_node()","version=1");
		// Versioned node, version 1
		besch->phasen = decode_uint16(p);
		besch->index = decode_uint16(p);
		besch->haus = NULL;
	}
	else {
		// skip the pointer ...
		p += 2;
		besch->phasen = decode_uint16(p);
		besch->index = decode_uint16(p);
		besch->haus = NULL;
	}
//  DBG_DEBUG("tile_reader_t::read_node()","phasen=%i index=%i", besch->phasen, besch->index );

	return besch;
}




void building_reader_t::register_obj(obj_besch_t *&data)
{
    haus_besch_t *besch = static_cast<haus_besch_t *>(data);

	if(besch->utyp==hausbauer_t::fabrik) {
		// this stuff is just for compatibility
		if(  strcmp("Oelbohrinsel",besch->gib_name())==0  ) {
			besch->enables = 1|2|4;
		}
		else if(  strcmp("fish_swarm",besch->gib_name())==0  ) {
			besch->enables = 4;
		}
	}

	if(besch->utyp==hausbauer_t::weitere  &&  besch->enables==0x80) {
		// this stuff is just for compatibility
		long checkpos=strlen(besch->gib_name());
		besch->enables = 0;
		// before station buildings were identified by their name ...
		if(  strcmp("BusStop",besch->gib_name()+checkpos-7)==0  ) {
			besch->utyp = hausbauer_t::bushalt;
			besch->enables = 1;
		}
		if(  strcmp("CarStop",besch->gib_name()+checkpos-7)==0  ) {
			besch->utyp = hausbauer_t::ladebucht;
			besch->enables = 4;
		}
		else if(  strcmp("TrainStop",besch->gib_name()+checkpos-9)==0  ) {
			besch->utyp = hausbauer_t::bahnhof;
			besch->enables = 1|4;
		}
		else if(  strcmp("ShipStop",besch->gib_name()+checkpos-8)==0  ) {
			besch->utyp = hausbauer_t::hafen;
			besch->enables = 1|4;
		}
		else if(  strcmp("ChannelStop",besch->gib_name()+checkpos-11)==0  ) {
			besch->utyp = hausbauer_t::binnenhafen;
			besch->enables = 1|4;
		}
		else if(  strcmp("PostOffice",besch->gib_name()+checkpos-10)==0  ) {
			besch->utyp = hausbauer_t::post;
			besch->enables = 2;
		}
		else if(  strcmp("StationBlg",besch->gib_name()+checkpos-10)==0  ) {
			besch->utyp = hausbauer_t::wartehalle;
			besch->enables = 1|4;
		}
	}

    hausbauer_t::register_besch(besch);
    DBG_DEBUG("building_reader_t::register_obj", "Loaded '%s'", besch->gib_name());
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
  // DBG_DEBUG("good_reader_t::read_node()", "called");

#ifdef _MSC_VER /* no var array on the stack supported */
    char *besch_buf = static_cast<char *>(alloca(node.size));
#else
  // Hajo: reading buffer is better allocated on stack
  char besch_buf [node.size];
#endif


  char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];

  haus_besch_t *besch = new haus_besch_t();

  besch->node_info = reinterpret_cast<obj_besch_info_t *>(info_buf);

  // Hajo: Read data
  fread(besch_buf, node.size, 1, fp);

  char * p = besch_buf;

  // Hajo: old versions of PAK files have no version stamp.
  // But we know, the highest bit was always cleared.

  const uint16 v = decode_uint16(p);
  const int version = (v & 0x8000)!=0 ? v&0x7FFF : 0;

  if(version == 3) {
    // Versioned node, version 3

    besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
    besch->utyp      = (enum hausbauer_t::utyp)decode_uint8(p);
    besch->level     = decode_uint16(p);
    besch->bauzeit   = decode_uint32(p);
    besch->groesse.x = decode_uint16(p);
    besch->groesse.y = decode_uint16(p);
    besch->layouts   = decode_uint8(p);
    besch->enables   = decode_uint8(p);
    besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
    besch->chance    = decode_uint8(p);

    besch->intro_date    = decode_uint16(p);
    besch->obsolete_date = decode_uint16(p);
  }
  else if(version == 2) {
    // Versioned node, version 2

    besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
    besch->utyp      = (enum hausbauer_t::utyp)decode_uint8(p);
    besch->level     = decode_uint16(p);
    besch->bauzeit   = decode_uint32(p);
    besch->groesse.x = decode_uint16(p);
    besch->groesse.y = decode_uint16(p);
    besch->layouts   = decode_uint8(p);
    besch->enables   = 0x80;
    besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
    besch->chance    = decode_uint8(p);

    besch->intro_date    = decode_uint16(p);
    besch->obsolete_date = decode_uint16(p);

  }
  else if(version == 1) {
    // Versioned node, version 1

    besch->gtyp      = (enum gebaeude_t::typ)decode_uint8(p);
    besch->utyp      = (enum hausbauer_t::utyp)decode_uint8(p);
    besch->level     = decode_uint16(p);
    besch->bauzeit   = decode_uint32(p);
    besch->groesse.x = decode_uint16(p);
    besch->groesse.y = decode_uint16(p);
    besch->layouts   = decode_uint8(p);
    besch->enables   = 0x80;
    besch->flags     = (enum haus_besch_t::flag_t)decode_uint8(p);
    besch->chance    = decode_uint8(p);

    besch->intro_date    = DEFAULT_INTRO_DATE*12;
    besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
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
    besch->enables   = 0x80;
    besch->flags     = (enum haus_besch_t::flag_t)decode_uint32(p);
    besch->chance    = 100;

    besch->intro_date    = DEFAULT_INTRO_DATE*12;
    besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
  }

	// correct old station buildings ...
	if(besch->level<=0  &&  (besch->utyp>=hausbauer_t::bahnhof  ||  besch->utyp==hausbauer_t::fabrik)) {
		DBG_DEBUG("building_reader_t::read_node()","old station building -> set level to 4");
		besch->level = 4;
	}

  DBG_DEBUG("building_reader_t::read_node()",
	     "version=%d"
	     " gtyp=%d"
	     " utyp=%d"
	     " level=%d"
	     " bauzeit=%d"
	     " groesse.x=%d"
	     " groesse.y=%d"
	     " layouts=%d"
	     " enables=%x"
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
	     besch->enables,
	     besch->flags,
	     besch->chance
	     );

  return besch;

}
