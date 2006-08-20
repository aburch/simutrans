//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  vehicle_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: vehicle_writer.cpp   $       $Author: hajo $
//  $Revision: 1.7 $         $Date: 2004/10/30 09:20:49 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: vehicle_writer.cc,v $
//  Revision 1.7  2004/10/30 09:20:49  hajo
//  sync for Dario
//
//  Revision 1.6  2004/01/16 21:35:20  hajo
//  Hajo: sync with Hendrik
//
//  Revision 1.5  2003/11/22 16:53:50  hajo
//  Hajo: integrated Hendriks changes
//
//  Revision 1.4  2003/06/29 10:33:38  hajo
//  Hajo: added Volkers changes
//
//  Revision 1.3  2003/01/08 19:54:02  hajo
//  Hajo: preparation for delivery to volker
//
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../vehikel_besch.h"
#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"

#include "vehicle_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif


/**
 * Calculate numeric waytype from waytype string
 * @author Hj. Malthaner
 */
static uint8 get_waytype(const char * waytype, tabfileobj_t &obj)
{
  uint8 uv8 = vehikel_besch_t::strasse;

  if(!STRICMP(waytype, "road")) {
    uv8 = vehikel_besch_t::strasse;
  } else if(!STRICMP(waytype, "track")) {
    uv8 = vehikel_besch_t::schiene;
  } else if(!STRICMP(waytype, "electrified_track")) {
    uv8 = 4;
  } else if(!STRICMP(waytype, "monorail_track")) {
    uv8 = vehikel_besch_t::schiene_monorail;
  } else if(!STRICMP(waytype, "maglev_track")) {
    uv8 = vehikel_besch_t::schiene_maglev;
  } else if(!STRICMP(waytype, "water")) {
	uv8 = vehikel_besch_t::wasser;
  } else if(!STRICMP(waytype, "schiene_tram")) {
		uv8 = vehikel_besch_t::schiene_strab;
	} else {
    cstring_t reason;

    reason.printf("invalid waytype %s for vehicle %s\n", waytype, obj.get("name"));

    throw new obj_pak_exception_t("vehicle_writer_t", reason);
  }

  return uv8;
}


/**
 * Calculate numeric engine type from engine type string
 * @author Hj. Malthaner
 */
static uint8 get_engine_type(const char * engine_type, tabfileobj_t &obj)
{
  uint8 uv8 = vehikel_besch_t::diesel;

  if(!STRICMP(engine_type, "diesel")) {
    uv8 = vehikel_besch_t::diesel;
  } else if(!STRICMP(engine_type, "electric")) {
    uv8 = vehikel_besch_t::electric;
  } else if(!STRICMP(engine_type, "steam")) {
    uv8 = vehikel_besch_t::steam;
  } else if(!STRICMP(engine_type, "bio")) {
    uv8 = vehikel_besch_t::bio;
  } else if(!STRICMP(engine_type, "fuel_cell")) {
    uv8 = vehikel_besch_t::fuel_cell;
  } else if(!STRICMP(engine_type, "hydrogene")) {
    uv8 = vehikel_besch_t::hydrogene;
  }

  // printf("Engine type %s -> %d\n", engine_type, uv8);

  return uv8;
}


/**
 * Writes vehicle node data to file
 * @author Hj. Malthaner
 */
void vehicle_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    int i;
    uint32 uv32;
    uint16 uv16;
    uint8  uv8;
    sint8  sv8;

    obj_node_t	node(this, 24, &parent, false);

    write_head(fp, node, obj);


    // Hajo: version number
    // Hajo: Version needs high bit set as trigger -> this is required
    //       as marker because formerly nodes were unversionend
    uv16 = 0x8002;
    node.write_data_at(fp, &uv16, 0, sizeof(uint16));


    // Hajodoc: Price of this vehicle in cent
    // Hajoval: int
    uv32 = obj.get_int("cost", 0);
    node.write_data_at(fp, &uv32, 2, sizeof(uint32));


    // Hajodoc: Payload of this vehicle
    // Hajoval: int
    uv16 = obj.get_int("payload", 0);
    node.write_data_at(fp, &uv16, 6, sizeof(uint16));


    // Hajodoc: Top speed of this vehicle. Must be greater than 0
    // Hajoval: int
    uv16 = obj.get_int("speed", 0);
    node.write_data_at(fp, &uv16, 8, sizeof(uint16));


    // Hajodoc: Total weight of this vehicle in tons
    // Hajoval: int
    uv16 = obj.get_int("weight", 0);
    node.write_data_at(fp, &uv16, 10, sizeof(uint16));


    // Hajodoc: Power of this vehicle in KW
    // Hajoval: int
    uv16 = obj.get_int("power", 0);
    node.write_data_at(fp, &uv16, 12, sizeof(uint16));


    // Hajodoc: Running costs, given in cent per square
    // Hajoval: int
    uv16 = obj.get_int("runningcost", 0);
    node.write_data_at(fp, &uv16, 14, sizeof(uint16));


    // Hajodoc: Introduction date (year*16+month)
    // Hajoval: int
    uv16  = obj.get_int("intro_year", 1900) * 16;
    uv16 += obj.get_int("intro_month", 1) - 1;
    node.write_data_at(fp, &uv16, 16, sizeof(uint16));


    // Hajodoc: Engine gear (power multiplier)
    // Hajoval: int
    uv8 = (obj.get_int("gear", 100) * 64) / 100;
    node.write_data_at(fp, &uv8, 18, sizeof(uint8));


    // Hajodoc: Type of way this vehicle drives on
    // Hajoval: road, track, electrified_track, monorail_track, maglev_track, water
    const char *waytype = obj.get("waytype");
    uv8 = get_waytype(waytype, obj);

    node.write_data_at(fp, &uv8, 19, sizeof(uint8));


    // Hajodoc: The sound to play on start, -1 for no sound
    // Hajoval: int
    sv8 = obj.get_int("sound", -1);
    node.write_data_at(fp, &sv8, 20, sizeof(sint8));


    if(uv8 == 4) {
      // Hajo: compatibility for old style DAT files
      uv8 = vehikel_besch_t::electric;
    } else {
      const char *engine_type = obj.get("engine_type");
      uv8 = get_engine_type(engine_type, obj);
    }
    node.write_data_at(fp, &uv8, 23, sizeof(uint8));



    // Hajodoc: The freight type
    // Hajoval: string
    const char *freight = obj.get("freight");
    if(!*freight) {
	freight = "None";
    }
    xref_writer_t::instance()->write_obj(fp, node, obj_good, freight, true);
    xref_writer_t::instance()->write_obj(fp, node, obj_smoke, obj.get("smoke"), false);

    //
    // Jetzt kommen die Bildlisten
    //
    static const char * dir_codes[] = {
	"s", "w", "sw", "se", "n", "e", "ne", "nw"
    };
    slist_tpl<cstring_t> emptykeys;
    slist_tpl<cstring_t> freightkeys;
    cstring_t str;
    bool    has_freight = false;
    bool    has_8_images = false;

    for(i = 0; i < 8; i++) {
	char buf[40];

	// Hajodoc: Empty vehicle image for direction, direction in "s", "w", "sw", "se", unsymmetric vehicles need also "n", "e", "ne", "nw"
	// Hajoval: string
	sprintf(buf, "emptyimage[%s]", dir_codes[i]);
	str = obj.get(buf);
	emptykeys.append(str);
	if(str.len() > 0) {
	    if(i >= 4) {
		has_8_images = true;
	    }
	}

	// Hajodoc: Loaded vehicle image for direction, direction in "s", "w", "sw", "se", unsymmetric vehicles need also "n", "e", "ne", "nw"
	// Hajoval: string
	sprintf(buf, "freightimage[%s]", dir_codes[i]);
	str = obj.get(buf);
	freightkeys.append(str);
	if(str.len() > 0) {
	    has_freight = true;
	    if(i >= 4) {
		has_8_images = true;
	    }
	}
    }
    if(!has_8_images) {
	while(emptykeys.count() > 4) {
	    emptykeys.remove_at(emptykeys.count() - 1);
	}
	while(freightkeys.count() > 4) {
	    freightkeys.remove_at(freightkeys.count() - 1);
	}
    }
    imagelist_writer_t::instance()->write_obj(fp, node, emptykeys);
    if(has_freight) {
	imagelist_writer_t::instance()->write_obj(fp, node, freightkeys);
    } else {
	xref_writer_t::instance()->write_obj(fp, node, obj_imagelist, "", false);
    }

    //
    // Vorgänger/Nachfolgerbedingungen
    //
    uint8 besch_vorgaenger = 0;
    do {
	char buf[40];

	// Hajodoc: Constraints for previous vehicles
	// Hajoval: string, "none" means only suitable at front of an convoi
	sprintf(buf, "constraint[prev][%d]", besch_vorgaenger);

	str = obj.get(buf);
	if(str.len() > 0) {
	    if(besch_vorgaenger == 0 && !STRICMP(str.chars(), "none")) {
		str = "";
	    }
	    xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.chars(), false);
	    besch_vorgaenger++;
	}
    } while (str.len() > 0);

    uint8 besch_nachfolger = 0;
    do {
	char buf[40];

	// Hajodoc: Constraints for successing vehicles
	// Hajoval: string, "none" to disallow any followers
	sprintf(buf, "constraint[next][%d]", besch_nachfolger);

	str = obj.get(buf);
	if(str.len() > 0) {
	    if(besch_nachfolger == 0 && !STRICMP(str.chars(), "none")) {
		str = "";
	    }
	    xref_writer_t::instance()->write_obj(fp, node, obj_vehicle, str.chars(), false);
	    besch_nachfolger++;
	}
    } while (str.len() > 0);

    node.write_data_at(fp, &besch_vorgaenger, 21, sizeof(sint8));
    node.write_data_at(fp, &besch_nachfolger, 22, sizeof(sint8));

    node.write(fp);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
