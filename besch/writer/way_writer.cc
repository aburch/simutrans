//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  way_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: way_writer.cpp       $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2004/10/30 09:20:49 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: way_writer.cc,v $
//  Revision 1.3  2004/10/30 09:20:49  hajo
//  sync for Dario
//
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../weg_besch.h"
#include "../../boden/wege/weg.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"

#include "way_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

/**
 * Convert waytype string to enum wegtyp
 * @author Hj. Malthaner
 */
static uint8 get_waytype(const char * waytype, tabfileobj_t &obj)
{
	uint8 uv8 = weg_t::strasse;

	if(!STRICMP(waytype, "road")) {
		uv8 = weg_t::strasse;
	} else if(!STRICMP(waytype, "track")) {
		uv8 = weg_t::schiene;
	} else if(!STRICMP(waytype, "electrified_track")) {
		uv8 = weg_t::overheadlines;
	} else if(!STRICMP(waytype, "monorail_track")) {
		uv8 = weg_t::schiene_monorail;
	} else if(!STRICMP(waytype, "maglev_track")) {
		uv8 = weg_t::schiene_maglev;
	} else if(!STRICMP(waytype, "water")) {
		uv8 = weg_t::wasser;
	} else if(!STRICMP(waytype, "air")) {
		uv8 = weg_t::luft;
	} else if(!STRICMP(waytype, "schiene_tram")) {
		uv8 = weg_t::schiene_strab;
	} else if(!STRICMP(waytype, "tram_track")) {
		uv8 = weg_t::schiene_strab;
	} else if(!STRICMP(waytype, "power")) {
		uv8 = weg_t::powerline;
	} else {
		cstring_t reason;
		reason.printf("invalid waytype %s for way %s\n", waytype, obj.get("name"));
		throw new obj_pak_exception_t("way_writer_t", reason);
	}

	return uv8;
}



/**
 * Write a waytype description node
 * @author Hj. Malthaner
 */
void way_writer_t::write_obj(FILE *outfp, obj_node_t &parent, tabfileobj_t &obj)
{
	static char *ribi_codes[16] = {
		"-", "n",  "e",  "ne",  "s",  "ns",  "se",  "nse",
		"w", "nw", "ew", "new", "sw", "nsw", "sew", "nsew"
	};
	int ribi, hang;

	// Hajo: node size is 24 bytes
	obj_node_t	node(this, 24, &parent, false);


	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8002;
	uint32 price =      obj.get_int("cost", 100);
	uint32 maintenance= obj.get_int("maintenance", 100);
	uint32 topspeed =   obj.get_int("topspeed", 999);
	uint32 max_weight = obj.get_int("max_weight", 999);

	uint16 intro  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
	intro +=        obj.get_int("intro_month", 1) - 1;

	uint16 retire  = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
	intro +=        obj.get_int("retire_month", 1) - 1;

	uint8 wtyp =    get_waytype(obj.get("waytype"), obj);
	uint8 styp =    obj.get_int("system_type", 0);
	if(wtyp==weg_t::schiene  &&  styp==1) {
//		styp = 1;
		wtyp = weg_t::schiene_monorail;
	} else if(wtyp==weg_t::schiene  &&  styp==7) {
//		styp = 7;
		wtyp = weg_t::schiene_strab;
	}

	node.write_data_at(outfp, &version, 0, 2);
	node.write_data_at(outfp, &price, 2, 4);
	node.write_data_at(outfp, &maintenance, 6, 4);
	node.write_data_at(outfp, &topspeed, 10, 4);
	node.write_data_at(outfp, &max_weight, 14, 4);
	node.write_data_at(outfp, &intro, 18, 4);
	node.write_data_at(outfp, &retire, 20, 2);
	node.write_data_at(outfp, &wtyp, 22, 1);
	node.write_data_at(outfp, &styp, 23, 1);


	write_head(outfp, node, obj);


	slist_tpl<cstring_t> keys;

	for(ribi = 0; ribi < 16; ribi++) {
	char buf[40];


	sprintf(buf, "image[%s]", ribi_codes[ribi]);

	cstring_t str = obj.get(buf);
	keys.append(str);
	}
	imagelist_writer_t::instance()->write_obj(outfp, node, keys);

	keys.clear();
	for(hang = 3; hang <= 12; hang += 3) {
	char buf[40];

	sprintf(buf, "imageup[%d]", hang);

	cstring_t str = obj.get(buf);
	keys.append(str);
	}
	imagelist_writer_t::instance()->write_obj(outfp, node, keys);

	keys.clear();
	for(ribi = 3; ribi <= 12; ribi += 3) {
	char buf[40];

	sprintf(buf, "diagonal[%s]", ribi_codes[ribi]);

	cstring_t str = obj.get(buf);
	keys.append(str);
	}
	imagelist_writer_t::instance()->write_obj(outfp, node, keys);

	slist_tpl<cstring_t> cursorkeys;

	cursorkeys.append(cstring_t(obj.get("cursor")));
	cursorkeys.append(cstring_t(obj.get("icon")));

	cursorskin_writer_t::instance()->write_obj(outfp, node, obj, cursorkeys);

	// node.write_data(fp, &besch);
	node.write(outfp);
}
