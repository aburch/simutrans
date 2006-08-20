//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  bridge_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: bridge_writer.cpp    $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2004/10/30 09:20:49 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: bridge_writer.cc,v $
//  Revision 1.3  2004/10/30 09:20:49  hajo
//  sync for Dario
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../bruecke_besch.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"

#include "bridge_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif


/**
 * Convert waytype string to enum wegtyp
 * @author Hj. Malthaner
 */
static uint8 get_waytype (const char * waytype, const char * obj_name)
{
  uint8 wegtyp = weg_t::strasse;

  if(!STRICMP(waytype, "road")) {
    wegtyp = weg_t::strasse;
  } else if(!STRICMP(waytype, "track")) {
    wegtyp = weg_t::schiene;
  } else {
    cstring_t reason;

    reason.printf("invalid waytype %s for bridge %s\n",
		  waytype,
		  obj_name);

    throw new obj_pak_exception_t("way_writer_t", reason);
  }

  return wegtyp;
}


/**
 * Writes bridge node data to file
 * @author Hj. Malthaner
 */
void bridge_writer_t::write_obj(FILE *outfp, obj_node_t &parent, tabfileobj_t &obj)
{
    // Hajo: node size is 14 bytes
    obj_node_t	node(this, 19, &parent, false);

    uint8 wegtyp = get_waytype(obj.get("waytype"), obj.get("name"));

    uint16 topspeed = obj.get_int("topspeed", 999);
    uint32 preis = obj.get_int("cost", 0);
    uint32 maintenance= obj.get_int("maintenance", 1000);
    uint8 pillars_every = obj.get_int("pillar_distance",0);	// distance==0 is off
    uint8 max_lenght = obj.get_int("max_lenght",0);	// max_lenght==0: unlimited

    // prissi: timeline
    uint16 intro_date  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
    intro_date += obj.get_int("intro_month", 1) - 1;

    uint16 obsolete_date  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
    obsolete_date += obj.get_int("retire_month", 1) - 1;

    // Hajo: Version needs high bit set as trigger -> this is required
    //       as marker because formerly nodes were unversionend
    uint16 version = 0x8005;
    node.write_data_at(outfp, &version, 0, 2);

    node.write_data_at(outfp, &topspeed, 2, 2);
    node.write_data_at(outfp, &preis, 4, 4);
    node.write_data_at(outfp, &maintenance, 8, 4);
    node.write_data_at(outfp, &wegtyp, 12, 1);
    node.write_data_at(outfp, &pillars_every, 13, 1);
    node.write_data_at(outfp, &max_lenght, 14, 1);
    node.write_data_at(outfp, &intro_date, 15, sizeof(uint16));
    node.write_data_at(outfp, &obsolete_date, 17, sizeof(uint16));


    static const char * names[] = {
	"image",
	"ns", "ew", NULL,
	"start",
	"n", "s", "e", "w", NULL,
	"ramp",
	"n", "s", "e", "w", NULL,
	NULL
    };
    slist_tpl<cstring_t> backkeys;
    slist_tpl<cstring_t> frontkeys;

    const char **ptr = names;
    const char *keyname = *ptr++;

    do {
	const char *keyindex = *ptr++;
	do {
	    char keybuf[40];
	    cstring_t value;

	    sprintf(keybuf, "back%s[%s]", keyname, keyindex);
	    value = obj.get(keybuf);
	    backkeys.append(value);
	    //intf("BACK: %s -> %s\n", keybuf, value.chars());
	    sprintf(keybuf, "front%s[%s]", keyname, keyindex);
	    value = obj.get(keybuf);
	    if(value.len()>2) {
		    frontkeys.append(value);
		    //intf("FRNT: %s -> %s\n", keybuf, value.chars());
		}
		else {
printf("WARNING: not %s specified (but might be still working)\n",keybuf);
		}
	    keyindex = *ptr++;
	} while(keyindex);
	keyname = *ptr++;
    } while(keyname);

	if(pillars_every>0) {
		backkeys.append(cstring_t(obj.get("backpillar[s]")));
		backkeys.append(cstring_t(obj.get("backpillar[w]")));
	}

    slist_tpl<cstring_t> cursorkeys;
    cursorkeys.append(cstring_t(obj.get("cursor")));
    cursorkeys.append(cstring_t(obj.get("icon")));

    imagelist_writer_t::instance()->write_obj(outfp, node, backkeys);
    imagelist_writer_t::instance()->write_obj(outfp, node, frontkeys);
    cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);

    cursorkeys.clear();
    backkeys.clear();
    frontkeys.clear();

    // node.write_data(outfp, &besch);
    node.write(outfp);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
