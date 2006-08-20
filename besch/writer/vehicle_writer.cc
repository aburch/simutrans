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

#include "../../utils/simstring.h"
#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../vehikel_besch.h"
#include "../sound_besch.h"
#include "../../boden/wege/weg.h"

#include "obj_pak_exception.h"
#include "obj_node.h"
#include "text_writer.h"
#include "xref_writer.h"
#include "imagelist_writer.h"

#include "get_waytype.h"
#include "vehicle_writer.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif


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
  } else if(!STRICMP(engine_type, "sail")) {
    uv8 = vehikel_besch_t::sail;
  } else if(!STRICMP(engine_type, "fuel_cell")) {
    uv8 = vehikel_besch_t::fuel_cell;
  } else if(!STRICMP(engine_type, "hydrogene")) {
    uv8 = vehikel_besch_t::hydrogene;
  } else if(!STRICMP(engine_type, "unknown")) {
    uv8 = vehikel_besch_t::unknown;
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

	int total_len = 29;

	// prissi: must be done here, since it may affect the len of the header!
	cstring_t sound_str = ltrim( obj.get("sound") );
	sint8 sound_id=NO_SOUND;
	if(sound_str.len()>0) {
		// ok, there is some sound
		sound_id = atoi(sound_str.chars());
		if(sound_id==0  &&  sound_str.chars()[0]=='0') {
			sound_id = 0;
			sound_str = "";
		}
		else if(sound_id!=0) {
			// old style id
			sound_str = "";
		}
		if(sound_str.len()>0) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.len()+1;
		}
	}

    obj_node_t	node(this, total_len, &parent, false);

    write_head(fp, node, obj);


    // Hajo: version number
    // Hajo: Version needs high bit set as trigger -> this is required
    //       as marker because formerly nodes were unversionend
    uv16 = 0x8006;
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
    uv32 = obj.get_int("power", 0);
    node.write_data_at(fp, &uv32, 12, sizeof(uint32));


    // Hajodoc: Running costs, given in cent per square
    // Hajoval: int
    uv16 = obj.get_int("runningcost", 0);
    node.write_data_at(fp, &uv16, 16, sizeof(uint16));


    // Hajodoc: Introduction date (year*16+month)
    // Hajoval: int
    uv16  = obj.get_int("intro_year", DEFAULT_INTRO_DATE) * 12;
    uv16 += obj.get_int("intro_month", 1) - 1;
    node.write_data_at(fp, &uv16, 18, sizeof(uint16));

    // Hajodoc: retire date (year*16+month)
    // Hajoval: int
    uv16 = obj.get_int("retire_year", DEFAULT_RETIRE_DATE) * 12;
    uv16 += obj.get_int("retire_month", 1) - 1;
    node.write_data_at(fp, &uv16, 20, sizeof(uint16));

    // Hajodoc: Engine gear (power multiplier)
    // Hajoval: int
    uv16 = (obj.get_int("gear", 100) * 64) / 100;
    node.write_data_at(fp, &uv16, 22, sizeof(uint16));


    // Hajodoc: Type of way this vehicle drives on
    // Hajoval: road, track, electrified_track, monorail_track, maglev_track, water
    const char *waytype = obj.get("waytype");
    const char waytype_uint = get_waytype(waytype);
    uv8 = (waytype_uint==weg_t::overheadlines) ? weg_t::schiene : waytype_uint;
    node.write_data_at(fp, &uv8, 24, sizeof(uint8));

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

	node.write_data_at(fp, &sound_id, 25, sizeof(sint8));

    if(waytype_uint == weg_t::overheadlines) {
      // Hajo: compatibility for old style DAT files
      uv8 = vehikel_besch_t::electric;
    } else {
      const char *engine_type = obj.get("engine_type");
      uv8 = get_engine_type(engine_type, obj);
    }
    node.write_data_at(fp, &uv8, 26, sizeof(uint8));


    node.write_data_at(fp, &besch_vorgaenger, 27, sizeof(sint8));
    node.write_data_at(fp, &besch_nachfolger, 28, sizeof(sint8));

	if(sound_str.len()>0) {
		sv8 = sound_str.len();
		node.write_data_at(fp, &sv8, 29, sizeof(sint8));
		node.write_data_at(fp, sound_str.chars(), 30, sound_str.len());
	}

    node.write(fp);
}
