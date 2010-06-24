#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "../skin_besch.h"
#include "../fabrik_besch.h"
#include "text_writer.h"
#include "building_writer.h"
#include "factory_writer.h"
#include "xref_writer.h"


void factory_field_class_writer_t::write_obj(FILE* outfp, obj_node_t& parent, const char* field_name, int snow_image, int production, int capacity, int weight)
{
	field_class_besch_t besch;
	obj_node_t node(this, 9, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_field, field_name, true);

	// Knightly : data specific to each field class
	besch.snow_image = snow_image;
	besch.production_per_field = production;
	besch.storage_capacity = capacity;
	besch.spawn_weight = weight;

	node.write_uint16(outfp, 0x8001,                        0); // version
	node.write_uint8 (outfp, besch.snow_image,              2);
	node.write_uint16(outfp, besch.production_per_field,    3);
	node.write_uint16(outfp, besch.storage_capacity,        5);
	node.write_uint16(outfp, besch.spawn_weight,            7);

	node.write(outfp);
}



void factory_field_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	field_besch_t besch;
	obj_node_t node(this, 10, &parent);

	if(  *obj.get("fields")  ) {
		// old format with no square-bracketed subscripts
		besch.field_classes = 1;
		const char *field_name = obj.get("fields");
		int snow_image = obj.get_int("has_snow", 1);
		int production = obj.get_int("production_per_field", 16);
		int capacity = obj.get_int("storage_capacity", 0);		// default is 0 to avoid breaking the balance of existing pakset objects
		int weight = obj.get_int("spawn_weight", 1000);

		factory_field_class_writer_t::instance()->write_obj(outfp, node, field_name, snow_image, production, capacity, weight);
	}
	else {
		// Knightly : for each field class, retrieve its data and write a field class node
		for(  besch.field_classes=0  ;  ;  besch.field_classes++  ) {
			char buf[64];

			sprintf(buf, "fields[%d]", besch.field_classes);
			const char *field_name = obj.get(buf);
			if(  !field_name  ||  !*field_name  ) {
				break;
			}

			sprintf(buf, "has_snow[%d]", besch.field_classes);
			int snow_image = obj.get_int(buf, 1);
			sprintf(buf, "production_per_field[%d]", besch.field_classes);
			int production = obj.get_int(buf, 16);
			sprintf(buf, "storage_capacity[%d]", besch.field_classes);
			int capacity = obj.get_int(buf, 0);		// default is 0 to avoid breaking the balance of existing pakset objects
			sprintf(buf, "spawn_weight[%d]", besch.field_classes);
			int weight = obj.get_int(buf, 1000);

			factory_field_class_writer_t::instance()->write_obj(outfp, node, field_name, snow_image, production, capacity, weight);
		}
	}

	// common, shared field data
	besch.probability = obj.get_int("probability_to_spawn", 10); // 0,1 %
	besch.max_fields = obj.get_int("max_fields", 25);
	besch.min_fields = obj.get_int("min_fields", 5);

	node.write_uint16(outfp, 0x8002,                     0); // version
	node.write_uint16(outfp, besch.probability,          2);
	node.write_uint16(outfp, besch.max_fields,           4);
	node.write_uint16(outfp, besch.min_fields,           6);
	node.write_uint16(outfp, besch.field_classes,        8);

	node.write(outfp);
}



void factory_smoke_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	rauch_besch_t besch;
	MEMZERO(besch);
	obj_node_t node(this, 10, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_smoke, obj.get("smoke"), true);
	besch.pos_off   = obj.get_koord("smoketile",   koord(0, 0));
	besch.xy_off    = obj.get_koord("smokeoffset", koord(0, 0));
	besch.zeitmaske = obj.get_int(  "smokespeed",  0);

	node.write_sint16(outfp, besch.pos_off.x, 0);
	node.write_sint16(outfp, besch.pos_off.y, 2);
	node.write_sint16(outfp, besch.xy_off.x,  4);
	node.write_sint16(outfp, besch.xy_off.y,  6);
	node.write_sint16(outfp, besch.zeitmaske, 8);

	node.write(outfp);
}


void factory_product_writer_t::write_obj(FILE* outfp, obj_node_t& parent, int capacity, int factor, const char* warename)
{
	obj_node_t node(this, sizeof(uint16) * 3, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_good, warename, true);

	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	// new version 2: pax-level added
	node.write_uint16(outfp, 0x8001,   0);

	node.write_uint16(outfp, capacity, 2);
	node.write_uint16(outfp, factor,   4);

	node.write(outfp);
}


void factory_supplier_writer_t::write_obj(FILE* outfp, obj_node_t& parent, int capacity, int count, int verbrauch, const char* warename)
{
	fabrik_lieferant_besch_t besch;

	obj_node_t node(this, 8, &parent);

	besch.anzahl = count;
	besch.kapazitaet = capacity;
	besch.verbrauch = verbrauch;

	xref_writer_t::instance()->write_obj(outfp, node, obj_good, warename, true);

	node.write_uint16(outfp, besch.kapazitaet, 0);
	node.write_uint16(outfp, besch.anzahl,     2);
	node.write_uint16(outfp, besch.verbrauch,  4);
	node.write_uint16(outfp, 0,                6); //dummy, unused (and uninitialized in past versions)

	node.write(outfp);
}


void factory_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	fabrik_besch_t besch;

	const char* placing = obj.get("location");

	if (!STRICMP(placing, "land")) {
		besch.platzierung = fabrik_besch_t::Land;
	} else if (!STRICMP(placing, "water")) {
		besch.platzierung = fabrik_besch_t::Wasser;
	} else if (!STRICMP(placing, "city")) {
		besch.platzierung = fabrik_besch_t::Stadt;
	}
	besch.produktivitaet = obj.get_int("productivity", 10);
	besch.bereich        = obj.get_int("range", 10);
	besch.gewichtung     = obj.get_int("distributionweight", 1);
	besch.kennfarbe      = obj.get_int("mapcolor", 255);
	if(besch.kennfarbe==255) {
		printf("ERROR:\nmissing an indentification color!\n");
		exit(0);
	}
	besch.pax_level      = obj.get_int("pax_level", 12);

	obj_node_t node(this, 18, &parent);

	obj.put("type", "fac");

	// name/copyright is in building - write_head(fp, node, obj);
	building_writer_t::instance()->write_obj(fp, node, obj);
	if (*obj.get("smoke")) {
		factory_smoke_writer_t::instance()->write_obj(fp, node, obj);
	} else {
		xref_writer_t::instance()->write_obj(fp, node, obj_smoke, "", false);
	}
	for (besch.lieferanten = 0; ; besch.lieferanten++) {
		char buf[40];

		sprintf(buf, "inputgood[%d]", besch.lieferanten);
		const char* good = obj.get(buf);

		if (!good || !*good) {
			break;
		}
		sprintf(buf, "inputsupplier[%d]", besch.lieferanten);
		int supp = obj.get_int(buf, 0);
		sprintf(buf, "inputcapacity[%d]", besch.lieferanten);
		int cap = obj.get_int(buf, 0);
		sprintf(buf, "inputfactor[%d]", besch.lieferanten);
		int verbrauch = (obj.get_int(buf, 100) * 256) / 100;

		factory_supplier_writer_t::instance()->write_obj(fp, node, cap, supp, verbrauch, good);
	}
	for (besch.produkte = 0; ; besch.produkte++) {
		char buf[40];

		sprintf(buf, "outputgood[%d]", besch.produkte);
		const char* good = obj.get(buf);

		if (!good || !*good) {
			break;
		}
		sprintf(buf, "outputcapacity[%d]", besch.produkte);
		int cap = obj.get_int(buf, 0);

		sprintf(buf, "outputfactor[%d]", besch.produkte);
		int fac = (obj.get_int(buf, 100) * 256) / 100;

		factory_product_writer_t::instance()->write_obj(fp, node, cap, fac, good);
	}
	// fields (careful, are xref'ed)
	besch.fields = 0;
	if(  *obj.get("fields")  ||  *obj.get("fields[0]")  ) {
		// Knightly : at least one field class available
		besch.fields = 1;
		factory_field_writer_t::instance()->write_obj(fp, node, obj);
	}

	// new version with pax_level
	node.write_uint16(fp, 0x8002,                      0); // version

	node.write_uint16(fp, (uint16) besch.platzierung,  2);
	node.write_uint16(fp, besch.produktivitaet,        4);
	node.write_uint16(fp, besch.bereich,               6);
	node.write_uint16(fp, besch.gewichtung,            8);
	node.write_uint8 (fp, besch.kennfarbe,            10);
	node.write_uint8 (fp, besch.fields,               11);
	node.write_uint16(fp, besch.lieferanten,          12);
	node.write_uint16(fp, besch.produkte,             14);
	node.write_uint16(fp, besch.pax_level,            16);

	node.write(fp);
}


std::string factory_writer_t::get_node_name(FILE* fp) const
{
	obj_node_info_t node; // Gebäude - wehe nicht

	fread(&node, sizeof(node), 1, fp);
	if (node.type != obj_building) return ""; // ???

	fseek(fp, node.size, SEEK_CUR);

	return building_writer_t::instance()->get_node_name(fp);
}
