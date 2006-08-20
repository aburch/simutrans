//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  building_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: building_writer.cpp  $       $Author: hajo $
//  $Revision: 1.5 $         $Date: 2004/01/01 11:34:43 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: building_writer.cc,v $
//  Revision 1.5  2004/01/01 11:34:43  hajo
//  Hajo: merge with Hendriks update
//
//  Revision 1.4  2003/11/22 16:53:50  hajo
//  Hajo: integrated Hendriks changes
//
//  Revision 1.3  2003/01/08 19:54:02  hajo
//  Hajo: preparation for delivery to volker
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../haus_besch.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"

#include "building_writer.h"
#include "skin_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      tile_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      int index
//      const slist_tpl< slist_tpl<cstring_t> > &backkeys
//      const slist_tpl< slist_tpl<cstring_t> > &frontkeys
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void tile_writer_t::write_obj(FILE *fp, obj_node_t &parent,
    int index, const slist_tpl< slist_tpl<cstring_t> > &backkeys,
    const slist_tpl< slist_tpl<cstring_t> > &frontkeys)
{
	haus_tile_besch_t besch;

	obj_node_t node(this, 6, &parent, false);

	besch.phasen = 0;

	slist_iterator_tpl< slist_tpl<cstring_t> > iter(backkeys);
	while(iter.next()) {
		if(iter.get_current().count() > besch.phasen) {
			besch.phasen = iter.get_current().count();
		}
	}
	iter = slist_iterator_tpl< slist_tpl<cstring_t> > (frontkeys);
	while(iter.next()) {
		if(iter.get_current().count() > besch.phasen) {
			besch.phasen = iter.get_current().count();
		}
	}
	besch.index = index;

	imagelist2d_writer_t::instance()->write_obj(fp, node, backkeys);
	imagelist2d_writer_t::instance()->write_obj(fp, node, frontkeys);

	// Hajo: temp vars of appropriate size
	uint16 v16;

	// Hajo: write version data
	v16 = 0x8001;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	v16 = besch.phasen;
	node.write_data_at(fp, &v16, 2, sizeof(uint16));

	v16 = besch.index;
	node.write_data_at(fp, &v16, 4, sizeof(uint16));

	node.write(fp);
}


/**
 * 01-Nov-03: Hajo: changed this method to write versioned
 * nodes
 */
void building_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    haus_besch_t besch;

    // Hajo: take care, hardocded size of node on disc here!
    obj_node_t	node(this, 21, &parent, false);

    write_head(fp, node, obj);

    besch.groesse = koord(1, 1);
    besch.layouts = 0;

    int *ints = obj.get_ints("dims");

    switch(ints[0]) {
    default:
    case 3:
	besch.layouts = ints[3];
    case 2:
	besch.groesse.y = ints[2];
    case 1:
	besch.groesse.x = ints[1];
    case 0:
	break;
    }
    if(besch.layouts == 0) {
	besch.layouts = besch.groesse.x == besch.groesse.y ? 1 : 2;
    }
    delete [] ints;

    besch.gtyp = gebaeude_t::unbekannt;
    besch.utyp = hausbauer_t::unbekannt;
    besch.bauzeit = 0;
    besch.level = 0;
    besch.flags = haus_besch_t::flag_t(
	(obj.get_int("noinfo", 0) > 0 ? haus_besch_t::FLAG_KEINE_INFO : 0) |
	(obj.get_int("noconstruction", 0) > 0 ? haus_besch_t::FLAG_KEINE_GRUBE : 0));

    const char *type_name = obj.get("type");

    if(!STRICMP(type_name, "res")) {
	besch.gtyp = gebaeude_t::wohnung;
	besch.level = obj.get_int("level", 1) - 1;
    } else if(!STRICMP(type_name, "com")) {
	besch.gtyp = gebaeude_t::gewerbe;
        besch.level = obj.get_int("level", 1) - 1;
    } else if(!STRICMP(type_name, "ind")) {
	besch.gtyp = gebaeude_t::industrie;
	besch.level = obj.get_int("level", 1) - 1;
    } else if(!STRICMP(type_name, "cur")) {
	besch.bauzeit = obj.get_int("build_time", 0);
	besch.level = obj.get_int("passengers", 0);
	besch.utyp = besch.bauzeit == 0 ? hausbauer_t::sehenswuerdigkeit : hausbauer_t::special;
    } else if(!STRICMP(type_name, "mon")) {
	besch.utyp = hausbauer_t::denkmal;
	besch.level = obj.get_int("passengers", 0);
    } else if(!STRICMP(type_name, "tow")) {
	besch.level = obj.get_int("passengers", 0);
	besch.bauzeit = obj.get_int("build_time", 0);
	besch.utyp = hausbauer_t::rathaus;
    } else if(!STRICMP(type_name, "hq")) {
	besch.level = obj.get_int("passengers", 0);
	besch.bauzeit = obj.get_int("build_time", 0);
	besch.utyp = hausbauer_t::firmensitz;
    } else if(!STRICMP(type_name, "fac")) {
	besch.utyp = hausbauer_t::fabrik;
    } else if(!STRICMP(type_name, "any") || *type_name == '\0') {
	besch.level = obj.get_int("level", 1) ;
	besch.utyp = hausbauer_t::weitere;
    } else {
	cstring_t reason;

	reason.printf("invalid type %s for building %s\n", type_name, obj.get("name"));
	throw new obj_pak_exception_t("building_writer_t", reason);
    }

    // Hajo: read chance - default is 100% chance to be built
    besch.chance = obj.get_int("chance", 100) ;

    // prissi: timeline for buildings
    besch.intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
    besch.intro_date += obj.get_int("intro_month", 1) - 1;

    besch.obsolete_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
    besch.obsolete_date += obj.get_int("retire_month", 1) - 1;

    int tile_index = 0;
    for(int l = 0; l < besch.layouts; l++)  {   // each layout
	for(int y = 0; y < besch.gib_h(l); y++)  {
    	    for(int x = 0; x < besch.gib_b(l); x++)  {	// each tile
		slist_tpl< slist_tpl<cstring_t> > backkeys;
		slist_tpl< slist_tpl<cstring_t> > frontkeys;

		for(int pos = 0; pos < 2; pos++) {
		    slist_tpl< slist_tpl<cstring_t> > &keys = pos ? backkeys : frontkeys;

   		    for(unsigned int h = 0; ; h++)  {	// each height
   			for(int phase = 0; ; phase++)  {	// each animation
			    char buf[40];

			    sprintf(buf, "%simage[%d][%d][%d][%d][%d]", pos ? "back" : "front", l, y, x, h, phase);

			    cstring_t str = obj.get(buf);
			    if(str.len() == 0) {
				break;
    			    }
			    if(phase == 0) {
				keys.append( slist_tpl<cstring_t>() );
			    }
			    keys.at(h).append(str);
			}
			if(keys.count() <= h) {
			    break;
			}
		    }
		}
		tile_writer_t::instance()->write_obj(fp, node, tile_index++, backkeys, frontkeys);
	    }
	}
    }
    // Hajo: old code
    // node.write_data(fp, &besch);

    // Hajo: temp vars of appropriate size
    uint32 v32;
    uint16 v16;
    uint8  v8;

    // Hajo: write version data
    v16 = 0x8002;
    node.write_data_at(fp, &v16, 0, sizeof(uint16));

    // Hajo: write besch data

    v8 = (uint8) besch.gtyp;
    node.write_data_at(fp, &v8, 2, sizeof(uint8));

    v8 = (uint8)besch.utyp;
    node.write_data_at(fp, &v8, 3, sizeof(uint8));

    v16 = (uint16)besch.level;
    node.write_data_at(fp, &v16, 4, sizeof(uint16));

    v32 = (uint32)besch.bauzeit;
    node.write_data_at(fp, &v32, 6, sizeof(uint32));

    v16 = besch.groesse.x;
    node.write_data_at(fp, &v16, 10, sizeof(uint16));

    v16 = besch.groesse.y;
    node.write_data_at(fp, &v16, 12, sizeof(uint16));

    v8 = (uint8)besch.layouts;
    node.write_data_at(fp, &v8, 14, sizeof(uint8));

    v8 = (uint8)besch.flags;
    node.write_data_at(fp, &v8, 15, sizeof(uint8));

    v8 = (uint8)besch.chance;
    node.write_data_at(fp, &v8, 16, sizeof(uint8));

    v16 = besch.intro_date;
    node.write_data_at(fp, &v16, 17, sizeof(uint16));

    v16 = besch.obsolete_date;
    node.write_data_at(fp, &v16, 19, sizeof(uint16));

    // probably add some icons, if defined
	slist_tpl<cstring_t> cursorkeys;

	cstring_t c=cstring_t(obj.get("cursor")), i=cstring_t(obj.get("icon"));
	cursorkeys.append(c);
	cursorkeys.append(i);
	if(c.len()>0  ||  i.len()>0) {
		cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);
	}
    node.write(fp);
}


/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
