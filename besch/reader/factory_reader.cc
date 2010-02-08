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



obj_besch_t *factory_field_class_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	field_class_besch_t *besch = new field_class_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	uint16 v = decode_uint16(p);
	if(  v==0x8001  ) {
		// Knightly : field class specific data
		besch->snow_image = decode_uint8(p);
		besch->production_per_field = decode_uint16(p);
		besch->storage_capacity = decode_uint16(p);
		besch->spawn_weight = decode_uint16(p);

		DBG_DEBUG("factory_field_class_reader_t::read_node()", "has_snow %i, production %i, capacity %i, spawn_weight %i", besch->snow_image, besch->production_per_field, besch->storage_capacity, besch->spawn_weight);
	}
	else {
		dbg->fatal("factory_field_class_reader_t::read_node()","unknown version %i", v&0x00ff );
	}

	return besch;
}



void factory_field_class_reader_t::register_obj(obj_besch_t *&data)
{
	field_class_besch_t *const besch = static_cast<field_class_besch_t *>(data);

	// xref has not yet been resolved
	const char* name = static_cast<xref_besch_t*>( besch->get_child(0) )->get_name();
	field_t::register_besch(besch, name);
}




obj_besch_t *factory_field_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	field_besch_t *besch = new field_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	uint16 v = decode_uint16(p);
	if(  v==0x8002  ) {
		// Knightly : this version only store shared, common data
		besch->probability = decode_uint16(p);
		besch->max_fields = decode_uint16(p);
		besch->min_fields = decode_uint16(p);
		besch->field_classes = decode_uint16(p);

		DBG_DEBUG("factory_field_reader_t::read_node()", "probability %i, fields: max %i, min %i, field classes: %i", besch->probability, besch->max_fields, besch->min_fields, besch->field_classes);
	}
	else if(  v==0x8001  ) {
		/* Knightly :
		 *   leave shared, common data in field besch
		 *   field class specific data goes to field class besch
		 */
		field_class_besch_t *const field_class_besch = new field_class_besch_t();

		field_class_besch->snow_image = decode_uint8(p);
		besch->probability = decode_uint16(p);
		field_class_besch->production_per_field = decode_uint16(p);
		besch->max_fields = decode_uint16(p);
		besch->min_fields = decode_uint16(p);
		besch->field_classes = 1;
		field_class_besch->storage_capacity = 0;
		field_class_besch->spawn_weight = 1000;

		/* Knightly :
		 *   store it in a static variable for further processing
		 *   later in factory_field_reader_t::register_obj()
		 */
		incomplete_field_class_besch = field_class_besch;

		DBG_DEBUG("factory_field_reader_t::read_node()", "has_snow %i, probability %i, fields: max %i, min %i, production %i", field_class_besch->snow_image, besch->probability, besch->max_fields, besch->min_fields, field_class_besch->production_per_field);
	}
	else {
		dbg->fatal("factory_field_reader_t::read_node()","unknown version %i", v&0x00ff );
	}

	besch->probability = 10000 - min(10000, besch->probability);

	return besch;
}



void factory_field_reader_t::register_obj(obj_besch_t *&data)
{
	field_besch_t *const besch = static_cast<field_besch_t *>(data);

	// Knightly : check if we need to continue with the construction of field class besch
	if(  incomplete_field_class_besch  ) {
		field_class_besch_t *const field_class_besch = static_cast<field_class_besch_t *>(incomplete_field_class_besch);
		// we *must* transfer the obj_besch_t array and not just the besch object itself
		// as xref reader has already logged the address of the array element for xref resolution
		field_class_besch->node_info = besch->node_info;
		besch->node_info = new obj_besch_t*[1];
		besch->node_info[0] = field_class_besch;
		factory_field_class_reader_t::instance()->register_obj( besch->node_info[0] );
		incomplete_field_class_besch = NULL;
	}

	// initialize weighted vector for the field class indices
	besch->init_field_class_indices();
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
	}
	else {
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
	}
	else {
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
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 2) {
		// Versioned node, version 2
		besch->platzierung = (enum fabrik_besch_t::platzierung)decode_uint16(p);
		besch->produktivitaet = decode_uint16(p);
		besch->bereich = decode_uint16(p);
		besch->gewichtung = decode_uint16(p);
		besch->kennfarbe = decode_uint8(p);
		besch->fields = decode_uint8(p);
		besch->lieferanten = decode_uint16(p);
		besch->produkte = decode_uint16(p);
		besch->pax_level = decode_uint16(p);
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
	}
	else {
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
	return besch;
}


void factory_reader_t::register_obj(obj_besch_t *&data)
{
	fabrik_besch_t* besch = static_cast<fabrik_besch_t*>(data);
	size_t fab_name_len = strlen( besch->get_name() );
	besch->electricity_producer = ( fab_name_len>11   &&  (strcmp(besch->get_name()+fab_name_len-9, "kraftwerk")==0  ||  strcmp(besch->get_name()+fab_name_len-11, "Power Plant")==0) );
	fabrikbauer_t::register_besch(besch);
}
