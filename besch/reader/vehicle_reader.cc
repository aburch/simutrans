#include <stdio.h>
#include "../../simdebug.h"
#include "../../simconst.h"
#include "../../bauer/vehikelbauer.h"
#include "../sound_besch.h"
#include "../vehikel_besch.h"
#include "../intro_dates.h"

#include "vehicle_reader.h"
#include "../obj_node_info.h"
#include "../../dataobj/pakset_info.h"



void vehicle_reader_t::register_obj(obj_besch_t *&data)
{
	vehikel_besch_t *besch = static_cast<vehikel_besch_t *>(data);
	vehikelbauer_t::register_besch(besch);
	obj_for_xref(get_type(), besch->get_name(), data);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool vehicle_reader_t::successfully_loaded() const
{
	return vehikelbauer_t::alles_geladen();
}


obj_besch_t *vehicle_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	vehikel_besch_t *besch = new vehikel_besch_t();
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

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if(version == 2) {
		// Versioned node, version 2

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);

		besch->obsolete_date = (DEFAULT_RETIRE_DATE*16);
	}
	else if (version==3   ||  version==4  ||  version==5) {
		// Versioned node, version 3 with retire date
		// version 4 identical, just other values for the waytype
		// version 5 just uses the new scheme for data calculation

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint8(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->engine_type = decode_uint8(p);
	}
	else if (version==6) {
		// version 5 just 32 bit for power and 16 Bit for gear

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	}
	else if (version==7) {
		// different length of cars ...

		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
	}
	else if (version==8) {
		// multiple freight images...
		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->freight_image_type = decode_uint8(p);
	}
	else if (version==9) {
		// new: fixed_cost, loading_time, axle_load
		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->loading_time = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->axle_load = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);
		besch->fixed_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->freight_image_type = decode_uint8(p);
	}
	else if (version==10) {
		// new: weight in kgs
		besch->preis = decode_uint32(p);
		besch->zuladung = decode_uint16(p);
		besch->loading_time = decode_uint16(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint32(p);
		besch->axle_load = decode_uint16(p);
		besch->leistung = decode_uint32(p);
		besch->running_cost = decode_uint16(p);
		besch->fixed_cost = decode_uint16(p);

		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->gear = decode_uint16(p);

		besch->typ = decode_uint8(p);
		besch->sound = decode_sint8(p);
		besch->engine_type = decode_uint8(p);
		besch->len = decode_uint8(p);
		besch->vorgaenger = decode_uint8(p);
		besch->nachfolger = decode_uint8(p);
		besch->freight_image_type = decode_uint8(p);
	}
	else {
		if(  version!=0  ) {
			dbg->fatal( "vehicle_reader_t::read_node()","Do not know how to handle version=%i", version );
		}
		// old node, version 0

		besch->typ = (sint8)v;
		besch->zuladung = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->geschw = decode_uint16(p);
		besch->gewicht = decode_uint16(p);
		besch->leistung = decode_uint16(p);
		besch->running_cost = decode_uint16(p);
		besch->sound = (sint8)decode_sint16(p);
		besch->vorgaenger = (sint8)decode_uint16(p);
		besch->nachfolger = (sint8)decode_uint16(p);

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
		static const waytype_t convert_from_old[8]={road_wt, track_wt, water_wt, air_wt, invalid_wt, monorail_wt, invalid_wt, tram_wt };
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
		besch->len = CARUNITS_PER_TILE/2;
	}

	// adjust length for different offset step sizes (which may arise in future)
	besch->len *= OBJECT_OFFSET_STEPS/CARUNITS_PER_TILE;

	// before version 8 vehicles could only have one freight image in each direction
	if(version<8) {
		besch->freight_image_type=0;
	}

	if(version<9) {
		besch->fixed_cost = 0;
		besch->axle_load = 0;
		besch->loading_time = 1800;
	}

	// old weights were tons
	if(version<10) {
		besch->gewicht *= 1000;
	}

	if(besch->sound==LOAD_SOUND) {
		uint8 len=decode_sint8(p);
		char wavname[256];
		wavname[len] = 0;
		for(uint8 i=0; i<len; i++) {
			wavname[i] = decode_sint8(p);
		}
		besch->sound = (sint8)sound_besch_t::get_sound_id(wavname);
DBG_MESSAGE("vehicle_reader_t::register_obj()","sound %s to %i",wavname,besch->sound);
	}
	else if(besch->sound>=0  &&  besch->sound<=MAX_OLD_SOUNDS) {
		sint16 old_id = besch->sound;
		besch->sound = (sint8)sound_besch_t::get_compatible_sound_id((sint8)old_id);
DBG_MESSAGE("vehicle_reader_t::register_obj()","old sound %i to %i",old_id,besch->sound);
	}

	DBG_DEBUG("vehicle_reader_t::read_node()",
		"version=%d "
		"way=%d zuladung=%d preis=%d geschw=%d gewicht=%d axle_load=%d leistung=%d "
		"betrieb=%d sound=%d vor=%d nach=%d "
		"date=%d/%d gear=%d engine_type=%d len=%d",
		version,
		besch->typ,
		besch->zuladung,
		besch->preis,
		besch->geschw,
		besch->gewicht,
		besch->axle_load,
		besch->leistung,
		besch->running_cost,
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
