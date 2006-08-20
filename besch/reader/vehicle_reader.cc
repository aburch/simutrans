/*
 * vehicle_reader.cc
 *
 * Copyright (c) 2002 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "../../simdebug.h"
#include "../../bauer/vehikelbauer.h"
#include "../../boden/wege/weg.h"
#include "../sound_besch.h"
#include "../vehikel_besch.h"

#include "vehicle_reader.h"
#include "../obj_node_info.h"

/******************************************
 * This an attempt to move vehicles at the right positiion during load time

static offset_koord xy_road[8] =
{
	{ 28, 45 },
	{ 51, 45 },
	{ 28, 51 },
	{ 25, 38 },
	{ 20, 48 },
	{ 34, 33 },
	{ 41, 52 },
	{ 21, 44 }
};

static offset_koord xy_rail[8] =
{
	{ 32, 48 },
	{ 32, 48 },
	{ 28, 48 },
	{ 22, 48 },
};

*/


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      vehicle_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void vehicle_reader_t::register_obj(obj_besch_t *&data)
{
    vehikel_besch_t *besch = static_cast<vehikel_besch_t *>(data);

	// init the pixel lenght information
	if(besch->length[0]==0) {
		uint8 w1,w2,w3;
		const uint8	*p = besch->gib_bild_daten(3);
		bool is_128=(p[0]+p[2]>64);

		switch(besch->typ)
		{
			case weg_t::strasse:
				// diagonal length
				p = besch->gib_bild_daten(0);
				w1 = p[0]+p[2]-(is_128?55-4:28-2);
				// vertical length (==height)
				p = besch->gib_bild_daten(2);
				w2 = p[1]+p[3]-(is_128?88-4:40-2);
				// horizontal length (==height)
				p = besch->gib_bild_daten(3);
				w3 = p[2];
			break;

			case weg_t::schiene:
			case 5: // weg_t::monorail:
			case 6: // weg_t::schiene_maglev:
			case 7: // weg_t::schiene_strab:
				// diagonal length
				p = besch->gib_bild_daten(0);
				w1 = p[0]+p[2]-(is_128?58-4:32-2);
				// vertical length (==height)
				p = besch->gib_bild_daten(2);
				w2 = p[1]+p[3]-(is_128?80-4:41-2);
				// horizontal length (==height)
				p = besch->gib_bild_daten(3);
				w3 = p[2];
			break;

			default:
				// diagonal length
				p = besch->gib_bild_daten(0);
				w1 = p[2];
				// vertical length (==height)
				p = besch->gib_bild_daten(2);
				w2 = ((uint16)p[3]*5)/4;
				// horizontal length (==height)
				p = besch->gib_bild_daten(3);
				w3 = p[2];
			break;
		}
		// since we use normalized koordinates
		if(is_128) {
			w1 /= 2;
			w2 /= 2;
			w3 /= 2;
		}
		for( int i=0;  i<8;  i++ ) {
			if(i<4) {
				besch->length[i] = w1;
			} else if(i<6) {
				besch->length[i] = w2;
			} else {
				besch->length[i] = w3;
			}
		}
	}

    vehikelbauer_t::register_besch(besch);

    obj_for_xref(get_type(), besch->gib_name(), data);
}

bool vehicle_reader_t::successfully_loaded() const
{
    vehikelbauer_t::sort_lists();
    return true;
}


/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
#ifdef _MSC_VER /* no var array on the stack supported */
	char *besch_buf = static_cast<char *>(alloca(node.size));
#else
	// Hajo: reading buffer is better allocated on stack
	char besch_buf [node.size];
#endif

	char *info_buf = new char[sizeof(obj_besch_t) + node.children * sizeof(obj_besch_t *)];
	vehikel_besch_t *besch = new vehikel_besch_t();
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

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	} else if(version == 2) {
		// Versioned node, version 2

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	} else if (version==3   ||  version==4  ||  version==5) {
		// Versioned node, version 3 with retire date
		// version 4 identical, just other values for the waytype
		// version 5 just uses the new scheme for data calculation

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);
	} else if (version==6) {
		// version 5 just 32 bit for power and 16 Bit for gear

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	} else if (version==7) {
		// different length of cars ...

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->betriebskosten = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	} else {
		// old node, version 0

		besch->typ = v;
		besch->zuladung = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->betriebskosten = decode_uint16(p);
		besch->sound = decode_sint16(p);
		besch->vorgaenger = decode_uint16(p);
		besch->nachfolger = decode_uint16(p);

		besch->intro_date = DEFAULT_INTRO_DATE*16;
		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
		besch->gear = 64;
	}


	// correct the engine type for old vehicles
	if(version<2) {
		// steam eangines usually have a sound of 3
		// electric engines will be overridden further down ...
		besch->engine_type = (besch->sound==3) ? vehikel_besch_t::steam : vehikel_besch_t::diesel;
	}

	//change the vehicle type
	if(version<4) {
		if(besch->typ==4) {
			besch->engine_type = vehikel_besch_t::electric;
			besch->typ = 1;
		}
		// convert to new standard
		const weg_t::typ convert_from_old[8]={weg_t::strasse, weg_t::schiene, weg_t::wasser, weg_t::luft, weg_t::invalid, weg_t::monorail, weg_t::schiene_maglev, weg_t::schiene_strab };
		besch->typ = convert_from_old[besch->typ];
	}

	// before version 5 dates were based on base 12 ...
	if(version<5) {
		uint16 date=besch->intro_date;
		besch->intro_date = (date/16)*12 + (date%16);
		date=besch->obsolete_date;
		besch->obsolete_date = (date/16)*12 + (date%16);
	}

	// before the length was always 1/8 (=half a tile)
	if(version<7) {
		besch->len = 8;
	}

	if(besch->sound==LOAD_SOUND) {
		uint8 len=decode_sint8(p);
		char wavname[256];
		wavname[len] = 0;
		for(uint8 i=0; i<len; i++) {
			wavname[i] = decode_sint8(p);
		}
		besch->sound = sound_besch_t::gib_sound_id(wavname);
DBG_MESSAGE("vehicle_reader_t::register_obj()","sound %s to %i",wavname,besch->sound);
	}
	else if(besch->sound>=0  &&  besch->sound<=MAX_OLD_SOUNDS) {
		sint16 old_id = besch->sound;
		besch->sound = sound_besch_t::get_compatible_sound_id(old_id);
DBG_MESSAGE("vehicle_reader_t::register_obj()","old sound %i to %i",old_id,besch->sound);
	}

  DBG_DEBUG("vehicle_reader_t::read_node()",
	     "version=%d "
	     "typ=%d zuladung=%d preis=%d geschw=%d gewicht=%d leistung=%d "
	     "betrieb=%d sound=%d vor=%d nach=%d "
	     "date=%d/%d gear=%d engine_type=%d len=%d",
	     version,
	     besch->typ,
	     besch->zuladung,
	     besch->preis,
	     besch->geschw,
	     besch->gewicht,
	     besch->leistung,
	     besch->betriebskosten,
	     besch->sound,
	     besch->vorgaenger,
	     besch->nachfolger,
	     (besch->intro_date%12)+1,
	     besch->intro_date/12,
	     besch->gear,
	     besch->engine_type,
	     besch->len);

  return besch;
}
