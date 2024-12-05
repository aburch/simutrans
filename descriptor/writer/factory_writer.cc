/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

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

	// data specific to each field class
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
		int capacity = obj.get_int("storage_capacity", 0); // default is 0 to avoid breaking the balance of existing pakset objects
		int weight = obj.get_int("spawn_weight", 1000);

		factory_field_class_writer_t::instance()->write_obj(outfp, node, field_name, snow_image, production, capacity, weight);
	}
	else {
		// for each field class, retrieve its data and write a field class node
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
			int capacity = obj.get_int(buf, 0); // default is 0 to avoid breaking the balance of existing pakset objects
			sprintf(buf, "spawn_weight[%d]", field_classes);
			int weight = obj.get_int(buf, 1000);

			factory_field_class_writer_t::instance()->write_obj(outfp, node, field_name, snow_image, production, capacity, weight);
		}
	}

	// common, shared field data
	uint16       probability  = obj.get_int("probability_to_spawn", 10); // 0,1 %
	uint16 const max_fields   = obj.get_int("max_fields",           25);
	uint16 const min_fields   = obj.get_int("min_fields",            5);
	uint16 const start_fields = obj.get_int("start_fields",          5);

	if (probability >= 10000) {
		printf("probability_to_spawn too large, set to 10,000\n");
		probability = 10000;
	}

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
	sint16 const smokespeed = 0; /* was obj.get_int("smokespeed",  0); */

	node.write_sint16(outfp, pos_off.x,  0);
	node.write_sint16(outfp, pos_off.y,  2);
	node.write_sint16(outfp, xy_off.x,   4);
	node.write_sint16(outfp, xy_off.y,   6);
	node.write_sint16(outfp, smokespeed, 8);

	node.write(outfp);
}


void factory_product_writer_t::write_obj(FILE* outfp, obj_node_t& parent, int capacity, int factor, const char* warename)
{
	obj_node_t node(this, sizeof(uint16) * 3, &parent);

	xref_writer_t::instance()->write_obj(outfp, node, obj_good, warename, true);

	// Version needs high bit set as trigger -> this is required
	// as marker because formerly nodes were unversioned
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

	node.write_uint16(outfp, capacity,    0);
	node.write_uint16(outfp, count,       2);
	node.write_uint16(outfp, consumption, 4);
	node.write_uint16(outfp, 0,           6); //dummy, unused (and uninitialized in past versions)

	node.write(outfp);
}


void factory_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	char const*            const placing     = obj.get("location");
	factory_desc_t::site_t const placement =
		!STRICMP(placing, "land")  ? factory_desc_t::Land  :
		!STRICMP(placing, "water") ? factory_desc_t::Water :
		!STRICMP(placing, "city")  ? factory_desc_t::City  :
		!STRICMP(placing, "river") ? factory_desc_t::river  :
		!STRICMP(placing, "shore") ? factory_desc_t::shore  :
		!STRICMP(placing, "forest")? factory_desc_t::forest :
		factory_desc_t::Land;
	uint16 const productivity = obj.get_int("productivity",        10);
	uint16 const range        = obj.get_int("range",               10);
	uint16 const chance       = obj.get_int("distributionweight",   1);
	uint8  const color        = obj.get_color("mapcolor", 255);

	if (color == 255) {
		dbg->fatal( "Factory", "%s missing an identification color! (mapcolor)", obj_writer_t::last_name );
	}

	uint16 const pax_level = obj.get_int("pax_level", 12);

	uint16 expand_probability = obj.get_int("expand_probability", 0);
	if (expand_probability >= 10000) {
		printf("expand_probability too large, set to 10,000\n");
		expand_probability = 10000;
	}

	uint16 const expand_minimum     = obj.get_int("expand_minimum",     0);
	uint16 const expand_range       = obj.get_int("expand_range",       0);
	uint16 const expand_times       = obj.get_int("expand_times",       0);

	uint16 const electric_boost  = (obj.get_int("electricity_boost",   1000) * 256 + 500) / 1000;
	uint16 const pax_boost       = (obj.get_int("passenger_boost",        0) * 256 + 500) / 1000;
	uint16 const mail_boost      = (obj.get_int("mail_boost",             0) * 256 + 500) / 1000;
	uint16 electric_demand       =  obj.get_int("electricity_amount", 65535);
	electric_demand              =  obj.get_int("electricity_demand", electric_demand);
	uint16 const pax_demand      =  obj.get_int("passenger_demand",   65535);
	uint16 const mail_demand     =  obj.get_int("mail_demand",        65535);
	// how long between sounds
	uint32 const sound_interval   =  obj.get_int("sound_interval",     0xFFFFFFFFul );

	uint16 total_len = 80;

	// must be done here, since it may affect the len of the header!
	string sound_str = ltrim( obj.get("sound") );
	sint8 sound_id=NO_SOUND;
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

		if(  cap<11  ) {
			dbg->error( "factory_writer_t::write_obj()", "Factory outputcapacity must be larger than 10! (currently %i)", cap );
		}

		sprintf(buf, "outputfactor[%d]", product_count);
		int fac = (obj.get_int(buf, 100) * 256) / 100;

		factory_product_writer_t::instance()->write_obj(fp, node, cap, fac, good);
	}

	// fields (careful, are xref'ed)
	uint8 fields = 0;
	if(  *obj.get("fields")  ||  *obj.get("fields[0]")  ) {
		// at least one field class available
		fields = 1;
		factory_field_group_writer_t::instance()->write_obj(fp, node, obj);
	}

	// now used here instead with the smoke

	koord  pos_off[ 4 ];
	koord  xy_off[ 4 ];
	uint8  num_smoke_offsets = 0;
	sint16 const smokeuplift = obj.get_int("smokeuplift", DEFAULT_SMOKE_UPLIFT );
	sint16 const smokelifetime = obj.get_int("smokelifetime", DEFAULT_FACTORYSMOKE_TIME );
	char str_smoketile[] = "smoketile[0]";
	char str_smokeoffset[] = "smokeoffset[0]";
	if(  *obj.get( str_smoketile )  ) {
		for( int i = 0; i < 4;  i++  ) {
			str_smoketile[10] = '0' + i;
			str_smokeoffset[12] = '0' + i;
			if( !*obj.get( str_smoketile ) ) {
				break;
			}
			pos_off[ i ] = obj.get_koord( str_smoketile, koord( 0, 0 ) );
			if( !*obj.get( str_smokeoffset ) ) {
				dbg->error( "factory_writer_t::write_obj", "%s defined but not %s!", str_smoketile, str_smokeoffset );
			}
			xy_off[ i ] = obj.get_koord( str_smokeoffset, koord( 0, 0 ) );
			num_smoke_offsets++;
		}
	}
	else {
		pos_off[0] = obj.get_koord( "smoketile", koord( 0, 0 ) );
		xy_off[0] = obj.get_koord( "smokeoffset", koord( 0, 0 ) );
	}

	// new version with pax_level, and smoke offsets part of factory
	node.write_uint16(fp, 0x8005,              0); // version
	node.write_uint16(fp, placement,           2);
	node.write_uint16(fp, productivity,        4);
	node.write_uint16(fp, range,               6);
	node.write_uint16(fp, chance,              8);
	node.write_uint8 (fp, color,              10);
	node.write_uint8 (fp, fields,             11);
	node.write_uint16(fp, supplier_count,     12);
	node.write_uint16(fp, product_count,      14);
	node.write_uint16(fp, pax_level,          16);
	node.write_uint16(fp, expand_probability, 18);
	node.write_uint16(fp, expand_minimum,     20);
	node.write_uint16(fp, expand_range,       22);
	node.write_uint16(fp, expand_times,       24);
	node.write_uint16(fp, electric_boost,     26);
	node.write_uint16(fp, pax_boost,          28);
	node.write_uint16(fp, mail_boost,         30);
	node.write_uint16(fp, electric_demand,    32);
	node.write_uint16(fp, pax_demand,         34);
	node.write_uint16(fp, mail_demand,        36);
	node.write_uint32(fp, sound_interval,     38);
	node.write_uint8 (fp, sound_id,           42);

	node.write_uint8 (fp, num_smoke_offsets,  43);
	for( int i = 0; i < 4; i++ ) {
		node.write_sint16( fp, pos_off[i].x, 44+i*8 );
		node.write_sint16( fp, pos_off[i].y, 46+i*8 );
		node.write_sint16( fp, xy_off[i].x,  48+i*8 );
		node.write_sint16( fp, xy_off[i].y,  50+i*8 );
	}
	node.write_sint16(fp, smokeuplift,       76);
	node.write_sint16(fp, smokelifetime,     78);

	// this should be always at the end
	sint8 sound_str_len = sound_str.size();
	if (sound_str_len > 0) {
		node.write_sint8  (fp, sound_str_len, 80);
		node.write_data_at(fp, sound_str.c_str(), 81, sound_str_len);
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
