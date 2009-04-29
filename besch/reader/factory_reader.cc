#include <stdio.h>
#include "../../simfab.h"
#include "../../bauer/fabrikbauer.h"
#include "../../dings/wolke.h"
#include "../../dings/field.h"
#include "../../simdebug.h"
#include "../obj_node_info.h"
#include "../fabrik_besch.h"
#include "../xref_besch.h"

#include "factory_reader.h"



obj_besch_t *
factory_field_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	field_besch_t *besch = new field_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	uint16 v = decode_uint16(p);
	if(v==0x8001) {
		besch->has_winter = decode_uint8(p);
		besch->probability = decode_uint16(p);
		besch->production_per_field = decode_uint16(p);
		besch->max_fields = decode_uint16(p);
		besch->min_fields = decode_uint16(p);
	}
	else {
		dbg->fatal("factory_field_reader_t::read_node()","unknown version %i",v&0x00ff );
	}

	DBG_DEBUG("factory_field_reader_t::read_node()", "has_snow %i, probability %i, fields: max %i, min %i, production %i",besch->has_winter, besch->probability, besch->max_fields, besch->min_fields, besch->production_per_field );
	besch->probability = 10000 - min(10000, besch->probability);
	return besch;
}



void
factory_field_reader_t::register_obj(obj_besch_t *&data)
{
	field_besch_t *besch = static_cast<field_besch_t *>(data);
	// Xref ist hier noch nicht aufgelöst!
	const char* name = static_cast<xref_besch_t*>(besch->get_child(0))->get_name();
	field_t::register_besch(besch, name);
}





obj_besch_t *
factory_smoke_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	rauch_besch_t *besch = new rauch_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	sint16 x = decode_sint16(p);
	sint16 y = decode_sint16(p);
	besch->pos_off = koord( x, y );

	x = decode_sint16(p);
	y = decode_sint16(p);

	besch->xy_off = koord( x, y );
	besch->zeitmaske = decode_sint16(p);

	DBG_DEBUG("factory_product_reader_t::read_node()","zeitmaske=%d (size %i)",node.size);

	return besch;
}


obj_besch_t *
factory_supplier_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_product_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	fabrik_lieferant_besch_t *besch = new fabrik_lieferant_besch_t();
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

		// not there yet ...
	} else {
		// old node, version 0
		besch->kapazitaet = v;
		besch->anzahl = decode_uint16(p);
		besch->verbrauch = decode_uint16(p);
	}
	DBG_DEBUG("factory_product_reader_t::read_node()",  "capacity=%d anzahl=%d, verbrauch=%d", version, besch->kapazitaet, besch->anzahl,besch->verbrauch);

	return besch;
}


obj_besch_t *
factory_product_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_product_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	fabrik_produkt_besch_t *besch = new fabrik_produkt_besch_t();
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
		besch->kapazitaet = decode_uint16(p);
		besch->faktor = decode_uint16(p);
	} else {
		// old node, version 0
		decode_uint16(p);
		besch->kapazitaet = v;
		besch->faktor = 256;
	}

	DBG_DEBUG("factory_product_reader_t::read_node()", "version=%d capacity=%d factor=%x", version, besch->kapazitaet, besch->faktor);
	return besch;
}


obj_besch_t *
factory_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	fabrik_besch_t *besch = new fabrik_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Experimental
	//@author: jamespetts

	const bool experimental = version > 0 ? v & EXP_VER : false;
	uint16 experimental_version = 0;
	if(experimental)
	{
		// Experimental version to start at 0 and increment.
		version = version & EXP_VER ? version & 0x3FFF : 0;
		while(version > 0x100)
		{
			version -= 0x100;
			experimental_version ++;
		}
		experimental_version -=1;
	}

	if(version == 2) {
		// Versioned node, version 2
		besch->platzierung = (enum fabrik_besch_t::platzierung)decode_uint16(p); //"placement" (Babelfish)
		besch->produktivitaet = decode_uint16(p); //"productivity" (Babelfish)
		besch->bereich = decode_uint16(p); //"range" (Babelfish)
		besch->gewichtung = decode_uint16(p); //"weighting" (Babelfish)
		besch->kennfarbe = decode_uint8(p); //"identification colour code" (Babelfish)
		besch->fields = decode_uint8(p); //"fields" (Babelfish)
		besch->lieferanten = decode_uint16(p); //"supplier" (Babelfish)
		besch->produkte = decode_uint16(p); //"products" (Babelfish)
		besch->pax_level = decode_uint16(p);
		if(experimental)
		{
			if(experimental_version == 1)
			{
				besch->electricity_proportion = ((float)decode_uint16(p) / 100.0);
				besch->inverse_electricity_proportion = 1 / besch->electricity_proportion;
			}
			else
			{
				dbg->fatal( "factory_reader_t::read_node()","Incompatible pak file version for Simutrans-E, number %i", experimental_version );
			}
		}
		DBG_DEBUG("factory_reader_t::read_node()","version=2, platz=%i, lieferanten=%i, pax=%i", besch->platzierung, besch->lieferanten, besch->pax_level );
	} else if(version == 1) {
		// Versioned node, version 1
		besch->platzierung = (enum fabrik_besch_t::platzierung)decode_uint16(p);
		besch->produktivitaet = decode_uint16(p);
		besch->bereich = decode_uint16(p);
		besch->gewichtung = decode_uint16(p);
		besch->kennfarbe = (uint8)decode_uint16(p);
		besch->lieferanten = decode_uint16(p);
		besch->produkte = decode_uint16(p);
		besch->pax_level = decode_uint16(p);
		besch->fields = 0;
		DBG_DEBUG("factory_reader_t::read_node()","version=1, platz=%i, lieferanten=%i, pax=%i", besch->platzierung, besch->lieferanten, besch->pax_level);
	} else {
		// old node, version 0, without pax_level
		DBG_DEBUG("factory_reader_t::read_node()","version=0");
		besch->platzierung = (enum fabrik_besch_t::platzierung)v;
		decode_uint16(p);	// alsways zero
		besch->produktivitaet = decode_uint16(p)|0x8000;
		besch->bereich = decode_uint16(p);
		besch->gewichtung = decode_uint16(p);
		besch->kennfarbe = (uint8)decode_uint16(p);
		besch->lieferanten = decode_uint16(p);
		besch->produkte = decode_uint16(p);
		besch->pax_level = 12;
		besch->fields = 0;
	}

	if(!experimental)
	{
		besch->electricity_proportion = 0.17;
		besch->inverse_electricity_proportion = 1 / besch->electricity_proportion;
	}

	return besch;
}


void factory_reader_t::register_obj(obj_besch_t *&data)
{
	fabrik_besch_t* besch = static_cast<fabrik_besch_t*>(data);
	size_t fab_name_len = strlen( besch->get_name() );
	besch->electricity_producer = ( fab_name_len>11   &&  (strcmp(besch->get_name()+fab_name_len-9, "kraftwerk")==0  ||  strcmp(besch->get_name()+fab_name_len-11, "Power Plant")==0) );
	fabrikbauer_t::register_besch(besch);
}
