#include <string>
#include <stdlib.h>
#include "../../utils/simstring.h"
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "../skin_desc.h"
#include "../factory_desc.h"
#include "../sound_desc.h"
#include "text_writer.h"
#include "building_writer.h"
#include "factory_writer.h"
#include "xref_writer.h"

using std::string;

void factory_field_class_writer_t::write_obj(FILE* outfp, obj_node_t& parent, const char* field_name, int snow_image, int production, int capacity, int weight)
{
	obj_node_t node(this, 9, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_field, field_name, true);

	// Knightly : data specific to each field class
	node.write_uint16(outfp, 0x8001,     0); // version
	node.write_uint8 (outfp, snow_image, 2);
	node.write_uint16(outfp, production, 3);
	node.write_uint16(outfp, capacity,   5);
	node.write_uint16(outfp, weight,     7);

	node.write(outfp);
}



void factory_field_group_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 12, &parent);

	uint16 field_classes;
	if(  *obj.get("fields")  ) {
		// format with no square-bracketed subscripts
		field_classes = 1;
		const char *field_name = obj.get("fields");
		int snow_image = obj.get_int("has_snow", 1);
		int production = obj.get_int("production_per_field", 16);
		int capacity = obj.get_int("storage_capacity", 0);		// default is 0 to avoid breaking the balance of existing pakset objects
		int weight = obj.get_int("spawn_weight", 1000);

		factory_field_class_writer_t::instance()->write_obj(outfp, node, field_name, snow_image, production, capacity, weight);
	}
	else {
		// Knightly : for each field class, retrieve its data and write a field class node
		for (field_classes = 0;; ++field_classes) {
			char buf[64];

			sprintf(buf, "fields[%d]", field_classes);
			const char *field_name = obj.get(buf);
			if(  !field_name  ||  !*field_name  ) {
				break;
			}

			sprintf(buf, "has_snow[%d]", field_classes);
			int snow_image = obj.get_int(buf, 1);
			sprintf(buf, "production_per_field[%d]", field_classes);
			int production = obj.get_int(buf, 16);
			sprintf(buf, "storage_capacity[%d]", field_classes);
			int capacity = obj.get_int(buf, 0);		// default is 0 to avoid breaking the balance of existing pakset objects
			sprintf(buf, "spawn_weight[%d]", field_classes);
			int weight = obj.get_int(buf, 1000);

			factory_field_class_writer_t::instance()->write_obj(outfp, node, field_name, snow_image, production, capacity, weight);
		}
	}

	// common, shared field data
	uint16 const probability = obj.get_int("probability_to_spawn", 10); // 0,1 %
	uint16 const max_fields  = obj.get_int("max_fields",           25);
	uint16 const min_fields  = obj.get_int("min_fields",            5);
	uint16 const start_fields  = obj.get_int("start_fields",            5);

	node.write_uint16(outfp, 0x8003,        0); // version
	node.write_uint16(outfp, probability,   2);
	node.write_uint16(outfp, max_fields,    4);
	node.write_uint16(outfp, min_fields,    6);
	node.write_uint16(outfp, start_fields,    8);
	node.write_uint16(outfp, field_classes, 10);

	node.write(outfp);
}



void factory_smoke_writer_t::write_obj(FILE* outfp, obj_node_t& parent, tabfileobj_t& obj)
{
	obj_node_t node(this, 10, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_smoke, obj.get("smoke"), true);
	koord  const pos_off   = obj.get_koord("smoketile",   koord(0, 0));
	koord  const xy_off    = obj.get_koord("smokeoffset", koord(0, 0));
	sint16 const smokespeed = obj.get_int("smokespeed",  0); 

	node.write_sint16(outfp, pos_off.x, 0);
	node.write_sint16(outfp, pos_off.y, 2);
	node.write_sint16(outfp, xy_off.x,  4);
	node.write_sint16(outfp, xy_off.y,  6);
	node.write_sint16(outfp, smokespeed, 8);

	node.write(outfp);
}


void factory_product_writer_t::write_obj(FILE* outfp, obj_node_t& parent, int capacity, int factor, const char* warename)
{
	obj_node_t node(this, sizeof(uint16) * 3, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_good, warename, true);

	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversioned
	// new version 2: pax-level added
	node.write_uint16(outfp, 0x8001,   0);

	node.write_uint16(outfp, capacity, 2);
	node.write_uint16(outfp, factor,   4);

	node.write(outfp);
}


void factory_supplier_writer_t::write_obj(FILE* outfp, obj_node_t& parent, int capacity, int count, int consumption, const char* warename)
{
	obj_node_t node(this, 8, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_good, warename, true);

	node.write_uint16(outfp, capacity,  0);
	node.write_uint16(outfp, count,     2);
	node.write_uint16(outfp, consumption, 4);
	node.write_uint16(outfp, 0,         6); //dummy, unused (and uninitialized in past versions)

	node.write(outfp);
}

void factory_upgrade_writer_t::write_obj(FILE* outfp, obj_node_t& parent, const char* str)
{
	obj_node_t node(this, 0, &parent);  
	
	xref_writer_t::instance()->write_obj(outfp, node, obj_factory, str, true);

	node.write(outfp);
}


void factory_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	string str;

	char const*            const placing     = obj.get("location");
	factory_desc_t::site_t const placement =
		!STRICMP(placing, "land")  ? factory_desc_t::Land   :
		!STRICMP(placing, "water") ? factory_desc_t::Water :
		!STRICMP(placing, "city")  ? factory_desc_t::City  :
		factory_desc_t::Land;
	uint16 const productivity = obj.get_int("productivity",        10);
	uint16 const range        = obj.get_int("range",               10);
	uint16 const dist_weight     = obj.get_int("distributionweight",   1);
	uint8  const color      = obj.get_color("mapcolor", 255);
	if (color == 255) {
		dbg->fatal( "Factory", "%s missing an identification color! (mapcolor)", obj_writer_t::last_name );
		exit(1);
	}

	uint16 const electricity_percent = obj.get_int("electricity_percent", 17);

	uint16 const expand_probability = obj.get_int("expand_probability", 0);
	uint16 const expand_minimum     = obj.get_int("expand_minimum",     0);
	uint16 const expand_range       = obj.get_int("expand_range",       0);
	uint16 const expand_times       = obj.get_int("expand_times",       0);

	uint16 const electric_boost  = (obj.get_int("electricity_boost",   1000) * 256 + 500) / 1000;
	uint16 const pax_boost       = (obj.get_int("passenger_boost",        0) * 256 + 500) / 1000;
	uint16 const mail_boost      = (obj.get_int("mail_boost",             0) * 256 + 500) / 1000;
	uint16 electric_demand       =  obj.get_int("electricity_amount", 65535);
	electric_demand              =  obj.get_int("electricity_demand", electric_demand);
	uint16 const max_distance_to_consumer = obj.get_int("max_distance_to_consumer", 65535); // In km, not tiles.
	// how long between sounds
	uint32 const sound_interval = obj.get_int("sound_interval", 10000u);

	uint16 total_len = 46;

	// prissi: must be done here, since it may affect the len of the header!
	string sound_str = ltrim(obj.get("sound"));
	sint16 sound_id = NO_SOUND;
	if (!sound_str.empty()) {
		// ok, there is some sound
		sound_id = atoi(sound_str.c_str());
		if (sound_id == 0 && sound_str[0] == '0') {
			sound_id = 0;
			sound_str = "";
		}
		else if (sound_id != 0) {
			// old style id
			sound_str = "";
		}
		if (!sound_str.empty()) {
			sound_id = LOAD_SOUND;
			total_len += sound_str.size() + 1;
		}
	}
	
	obj_node_t node(this, total_len, &parent);

	obj.put("type", "fac");

	// name/copyright is in building - write_head(fp, node, obj);
	building_writer_t::instance()->write_obj(fp, node, obj);
	if (*obj.get("smoke")) {
		factory_smoke_writer_t::instance()->write_obj(fp, node, obj);
	}
	else {
		xref_writer_t::instance()->write_obj(fp, node, obj_smoke, "", false);
	}

	uint16 supplier_count;
	for (supplier_count = 0;; ++supplier_count) {
		char buf[40];

		sprintf(buf, "inputgood[%d]", supplier_count);
		const char* good = obj.get(buf);

		if (!good || !*good) {
			break;
		}
		sprintf(buf, "inputsupplier[%d]", supplier_count);
		int supp = obj.get_int(buf, 0);
		sprintf(buf, "inputcapacity[%d]", supplier_count);
		int cap = obj.get_int(buf, 0);
		sprintf(buf, "inputfactor[%d]", supplier_count);
		int consumption = (obj.get_int(buf, 100) * 256) / 100;

		factory_supplier_writer_t::instance()->write_obj(fp, node, cap, supp, consumption, good);
	}

	uint16 product_count;
	for (product_count = 0;; ++product_count) {
		char buf[40];

		sprintf(buf, "outputgood[%d]", product_count);
		const char* good = obj.get(buf);

		if (!good || !*good) {
			break;
		}
		sprintf(buf, "outputcapacity[%d]", product_count);
		int cap = obj.get_int(buf, 0);

		if(  cap==0  ) {
			dbg->error( "factory_writer_t::write_obj()", "Factory output capacity must be larger than 0. (currently %i)", cap );
		}

		sprintf(buf, "outputfactor[%d]", product_count);
		int fac = (obj.get_int(buf, 100) * 256) / 100;

		factory_product_writer_t::instance()->write_obj(fp, node, cap, fac, good);
	}

	// fields (careful, are xref'ed)
	uint8 fields = 0;
	if(  *obj.get("fields")  ||  *obj.get("fields[0]")  ) {
		// Knightly : at least one field class available
		fields = 1;
		factory_field_group_writer_t::instance()->write_obj(fp, node, obj);
	}

	// Upgrades: these are the industry types to which this industry
	// can be upgraded. 
	// @author: jamespetts
	sint8 upgrades = 0;
	do 
	{
		char buf[40];
		sprintf(buf, "upgrade[%d]", upgrades);
		str = obj.get(buf);
		if(!str.empty()) 
		{
			xref_writer_t::instance()->write_obj(fp, node, obj_factory, str.c_str(), false);
			upgrades++;
		}
	} while (!str.empty());

	uint8 const field_output_divider = obj.get_int("field_output_divider", 1);

	// new version with factory boost, etc.
	uint16 version = 0x8004;

	// This is the overlay flag for Simutrans-Extended
	// This sets the *second* highest bit to 1. 
	version |= EX_VER;

	// Finally, this is the extended version number. This is *added*
	// to the standard version number, to be subtracted again when read.
	// Start at 0x100 and increment in hundreds (hex).

	// 0x200 - version 7.0 and greater. Includes xref factories for upgrades.
	// 0x300 - version 12.0 and greater. Removes passenger/mail parameters,
	// which are now in the gebaeude_t objects, and adds max_distance_to_consumer.
	// 0x400 - 16-bit sound ID
	version += 0x400;
	
	node.write_uint16(fp, version,						0); 
	node.write_uint16(fp, placement,					2);
	node.write_uint16(fp, productivity,					4);
	node.write_uint16(fp, range,						6);
	node.write_uint16(fp, dist_weight,					8);
	node.write_uint8 (fp, color,						10);
	node.write_uint8 (fp, fields,						11);
	node.write_uint16(fp, supplier_count,				12);
	node.write_uint16(fp, product_count,				14);
	node.write_uint16(fp, electricity_percent,			16);
	node.write_sint8 (fp, upgrades,						18);
	node.write_uint16(fp, expand_probability,			19);
	node.write_uint16(fp, expand_minimum,				21);
	node.write_uint16(fp, expand_range,					23);
	node.write_uint16(fp, expand_times,					25);
	node.write_uint16(fp, electric_boost,				27);
	node.write_uint16(fp, pax_boost,					29);
	node.write_uint16(fp, mail_boost,					31);
	node.write_uint16(fp,  electric_demand,				33);
	node.write_uint16(fp, max_distance_to_consumer,		35);
	node.write_uint32(fp, sound_interval,				37);
	node.write_uint16(fp, sound_id,						41);
	node.write_uint8(fp, field_output_divider, 			43);

	// this should be always at the end
	sint8 sound_str_len = sound_str.size();
	if (sound_str_len > 0) {
		node.write_sint8(fp, sound_str_len, 44);
		node.write_data_at(fp, sound_str.c_str(), 45, sound_str_len);
	}

	node.write(fp);
}


std::string factory_writer_t::get_node_name(FILE* fp) const
{
	obj_node_info_t node; // Name is inside building (BUIL) node, which is a child of factory (FACT) node

	fread(&node, sizeof(node), 1, fp);
	if (node.type != obj_building) return ""; // If BUIL node not found, return blank to at least not crash

	fseek(fp, node.size, SEEK_CUR);

	return building_writer_t::instance()->get_node_name(fp);
}
