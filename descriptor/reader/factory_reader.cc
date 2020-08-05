/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simfab.h"
#include "../../bauer/fabrikbauer.h"
#include "../../simdebug.h"
#include "../obj_node_info.h"
#include "../sound_desc.h"
#include "../factory_desc.h"
#include "../xref_desc.h"
#include "../../network/pakset_info.h"

#include "factory_reader.h"


// Knightly : determine the combined probability of 256 rounds of chances
uint16 rescale_probability(const uint16 p)
{
	if(  p  ) {
		sint64 pp = ( (sint64)p << 30 ) / 10000LL;
		sint64 qq = ( 1LL << 30 ) - pp;
		uint16 ss = 256u;
		while(  (ss >>= 1)  ) {
			pp += (pp * qq) >> 30;
			qq = (qq * qq) >> 30;
		}
		return (uint16)(((pp * 10000LL) + (1LL << 29)) >> 30);
	}
	return p;
}


obj_desc_t *factory_field_class_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	field_class_desc_t *desc = new field_class_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	uint16 v = decode_uint16(p);
	if(  v==0x8001  ) {
		// Knightly : field class specific data
		desc->snow_image = decode_uint8(p);
		desc->production_per_field = decode_uint16(p);
		desc->storage_capacity = decode_uint16(p);
		desc->spawn_weight = decode_uint16(p);

		DBG_DEBUG("factory_field_class_reader_t::read_node()", "has_snow %i, production %i, capacity %i, spawn_weight %i",
			desc->snow_image,
			desc->production_per_field,
			desc->storage_capacity,
			desc->spawn_weight);
	}
	else {
		dbg->fatal("factory_field_class_reader_t::read_node()","unknown version %i", v&0x00ff );
	}

	return desc;
}


obj_desc_t *factory_field_group_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	field_group_desc_t *desc = new field_group_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	uint16 v = decode_uint16(p);
	if(  v==0x8003  ) {
		desc->probability = rescale_probability( decode_uint16(p) );
		desc->max_fields = decode_uint16(p);
		desc->min_fields = decode_uint16(p);
		desc->start_fields = decode_uint16(p);
		desc->field_classes = decode_uint16(p);

	}
	else if(  v==0x8002  ) {
		// Knightly : this version only store shared, common data
		desc->probability = rescale_probability( decode_uint16(p) );
		desc->max_fields = decode_uint16(p);
		desc->min_fields = decode_uint16(p);
		desc->field_classes = decode_uint16(p);

	}
	else if(  v==0x8001  ) {
		/* Knightly :
		 *   leave shared, common data in field desc
		 *   field class specific data goes to field class desc
		 */
		field_class_desc_t *const field_class_desc = new field_class_desc_t();

		field_class_desc->snow_image = decode_uint8(p);
		desc->probability = rescale_probability( decode_uint16(p) );
		field_class_desc->production_per_field = decode_uint16(p);
		desc->max_fields = decode_uint16(p);
		desc->min_fields = decode_uint16(p);
		desc->field_classes = 1;
		field_class_desc->storage_capacity = 0;
		field_class_desc->spawn_weight = 1000;

		/* Knightly :
		 *   store it in a static variable for further processing
		 *   later in factory_field_reader_t::register_obj()
		 */
		incomplete_field_class_desc = field_class_desc;

		DBG_DEBUG("factory_field_group_reader_t::read_node()", "version=%i, probability=%i, fields: max=%i / min=%i / start=%i, field classes=%i, storage=%i, field_prod=%i, chance=%i, has_snow=%i",
			v,
			desc->probability,
			desc->max_fields,
			desc->min_fields,
			desc->start_fields,
			desc->field_classes,
			field_class_desc->storage_capacity,
			field_class_desc->production_per_field,
			field_class_desc->spawn_weight,
			field_class_desc->snow_image);
	}
	else {
		dbg->fatal("factory_field_group_reader_t::read_node()","unknown version %i", v&0x00ff );
	}

	if (v > 0x8001) {
		DBG_DEBUG("factory_field_group_reader_t::read_node()", "version=%i, probability=%i, fields: max=%i / min=%i / start=%i, field classes=%i",
			v,
			desc->probability,
			desc->max_fields,
			desc->min_fields,
			desc->start_fields,
			desc->field_classes);
	}

	return desc;
}


void factory_field_group_reader_t::register_obj(obj_desc_t *&data)
{
	field_group_desc_t *const desc = static_cast<field_group_desc_t *>(data);

	// Knightly : check if we need to continue with the construction of field class desc
	if (field_class_desc_t *const field_class_desc = incomplete_field_class_desc) {
		// we *must* transfer the obj_desc_t array and not just the desc object itself
		// as xref reader has already logged the address of the array element for xref resolution
		field_class_desc->children  = desc->children;
		desc->children              = new obj_desc_t*[1];
		desc->children[0]           = field_class_desc;
		incomplete_field_class_desc = NULL;
	}
}

obj_desc_t *factory_smoke_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	smoke_desc_t *desc = new smoke_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	sint16 x = decode_sint16(p);
	sint16 y = decode_sint16(p);
	desc->pos_off = koord( x, y );

	x = decode_sint16(p);
	y = decode_sint16(p);

	desc->xy_off = koord( x, y );
	/*smoke speed*/ decode_sint16(p);

	DBG_DEBUG("factory_product_reader_t::read_node()","zeitmaske=%d (size %i)",node.size);

	return desc;
}


obj_desc_t *factory_supplier_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_product_reader_t::read_node()", "called");
	ALLOCA(char, desc_buf, node.size);

	factory_supplier_desc_t *desc = new factory_supplier_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

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
		desc->capacity = v;
		desc->supplier_count = decode_uint16(p);
		desc->consumption = decode_uint16(p);
	}

	DBG_DEBUG("factory_product_reader_t::read_node()", "version=%d, capacity=%d, count=%d, consumption=%d",
		0,
		desc->capacity,
		desc->supplier_count,
		desc->consumption);

	return desc;
}


obj_desc_t *factory_product_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_product_reader_t::read_node()", "called");
	ALLOCA(char, desc_buf, node.size);

	factory_product_desc_t *desc = new factory_product_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);

	char * p = desc_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 1) {
		// Versioned node, version 1
		desc->capacity = decode_uint16(p);
		desc->factor = decode_uint16(p);
	}
	else {
		// old node, version 0
		decode_uint16(p);
		desc->capacity = v;
		desc->factor = 256;
	}

	DBG_DEBUG("factory_product_reader_t::read_node()", "version=%d capacity=%d factor=%x", version, desc->capacity, desc->factor);
	return desc;
}


obj_desc_t *factory_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_reader_t::read_node()", "called");
	ALLOCA(char, desc_buf, node.size);

	factory_desc_t *desc = new factory_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);

	desc->sound_id = NO_SOUND;
	desc->sound_interval = 10000u;

	char * p = desc_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Extended
	// @author: jamespetts

	const bool extended = version > 0 ? v & EX_VER : false;
	uint16 extended_version = 0;
	if(extended)
	{
		// Extended version to start at 0 and increment.
		version = version & EX_VER ? version & 0x3FFF : 0;
		while(version > 0x100)
		{
			version -= 0x100;
			extended_version ++;
		}
		extended_version -=1;
	}

	typedef factory_desc_t::site_t site_t;
	if (version == 4)
	{
		// Versioned node, version 4 with sound and animation
		desc->placement = (site_t)decode_uint16(p);
		desc->productivity = decode_uint16(p);
		desc->range = decode_uint16(p);
		desc->distribution_weight = decode_uint16(p);
		desc->color = decode_uint8(p);
		desc->fields = decode_uint8(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		if (extended)
		{
			desc->pax_level = 65535;
		}
		else
		{
			desc->pax_level = decode_uint16(p);
		}
		if (extended)
		{
			desc->electricity_proportion = decode_uint16(p);
			desc->inverse_electricity_proportion = 100 / desc->electricity_proportion;

			desc->upgrades = decode_uint8(p);

			if (extended_version > 4)
			{
				// Check for incompatible future versions
				dbg->fatal("factory_reader_t::read_node()", "Incompatible pak file version for Simutrans-Extended, number %i", extended_version);
			}
		}
		desc->expand_probability = rescale_probability(decode_uint16(p));
		desc->expand_minimum = decode_uint16(p);
		desc->expand_range = decode_uint16(p);
		desc->expand_times = decode_uint16(p);
		desc->electric_boost = decode_uint16(p);
		desc->pax_boost = decode_uint16(p);
		desc->mail_boost = decode_uint16(p);
		desc->electric_demand = decode_uint16(p);
		if (extended && extended_version > 1)
		{
			desc->pax_demand = 65535;
			desc->mail_demand = 65535;
			desc->base_max_distance_to_consumer = decode_uint16(p);
		}
		else
		{
			desc->pax_demand = decode_uint16(p);
			desc->mail_demand = decode_uint16(p);
			desc->base_max_distance_to_consumer = 65535;
		}
		desc->sound_interval = decode_uint32(p);
		if (extended_version >= 3)
		{
			desc->sound_id = decode_sint16(p);
		}
		else
		{
			desc->sound_id = decode_sint8(p);
		}
		desc->field_output_divider = decode_uint8(p);

		if (extended && extended_version >= 4)
		{
			desc->base_max_distance_to_supplier = decode_uint16(p);
		}
		else
		{
			desc->base_max_distance_to_supplier = 65535;
		}

		DBG_DEBUG("factory_reader_t::read_node()", "version=4, platz=%i, supplier_count=%i, pax=%i, sound_interval=%li, sound_id=%i", desc->placement, desc->supplier_count, desc->pax_level, desc->sound_interval, desc->sound_id);
	}
	else if(version == 3) {
		// Versioned node, version 3
		desc->placement = (site_t)decode_uint16(p);
		desc->productivity = decode_uint16(p);
		desc->range = decode_uint16(p);
		desc->distribution_weight = decode_uint16(p);
		desc->color = decode_uint8(p);
		desc->fields = decode_uint8(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		if(extended && extended_version > 1)
		{
			desc->pax_level = 65535;
		}
		else
		{
			desc->pax_level = decode_uint16(p);
		}
		if(extended)
		{
			desc->electricity_proportion = decode_uint16(p);
			desc->inverse_electricity_proportion = 100 / desc->electricity_proportion;

			if(extended_version >= 1)
			{
				desc->upgrades = decode_uint8(p);
			}
			else
			{
				desc->upgrades = 0;
			}
			if(extended_version > 4)
			{
				// Check for incompatible future versions
				dbg->fatal( "factory_reader_t::read_node()","Incompatible pak file version for Simutrans-Extended, number %i", extended_version );
			}
		}
		desc->expand_probability = rescale_probability( decode_uint16(p) );
		desc->expand_minimum = decode_uint16(p);
		desc->expand_range = decode_uint16(p);
		desc->expand_times = decode_uint16(p);
		desc->electric_boost = decode_uint16(p);
		desc->pax_boost = decode_uint16(p);
		desc->mail_boost = decode_uint16(p);
		desc->electric_demand = decode_uint16(p);
		desc->field_output_divider = 1;
		if(extended && extended_version > 1)
		{
			desc->pax_demand = 65535;
			desc->mail_demand = 65535;
			desc->base_max_distance_to_consumer = decode_uint16(p);
		}
		else
		{
			desc->pax_demand = decode_uint16(p);
			desc->mail_demand = decode_uint16(p);
			desc->base_max_distance_to_consumer = 65535;
		}
		desc->base_max_distance_to_supplier = 65535;
		DBG_DEBUG("factory_reader_t::read_node()","version=3, platz=%i, supplier_count=%i, pax=%i", desc->placement, desc->supplier_count, desc->pax_level );
	} else if(version == 2) {
		// Versioned node, version 2
		desc->placement = (site_t)decode_uint16(p); //"placement" (Babelfish)
		desc->productivity = decode_uint16(p); //"productivity" (Babelfish)
		desc->range = decode_uint16(p); //"range" (Babelfish)
		desc->distribution_weight = decode_uint16(p); //"weighting" (Babelfish)
		if(desc->distribution_weight < 1)
		{
			// Avoid divide by zero errors when
			// determining industry density figures.
			desc->distribution_weight = 1;
		}
		desc->color = decode_uint8(p); //"identification colour code" (Babelfish)
		desc->fields = decode_uint8(p); //"fields" (Babelfish)
		desc->supplier_count = decode_uint16(p); //"supplier" (Babelfish)
		desc->product_count = decode_uint16(p); //"products" (Babelfish)
		desc->pax_level = decode_uint16(p);
		if(extended)
		{
			desc->electricity_proportion = decode_uint16(p);
			desc->inverse_electricity_proportion = 100 / desc->electricity_proportion;

			if(extended_version >= 1)
			{
				desc->upgrades = decode_uint8(p);
			}
			else
			{
				desc->upgrades = 0;
			}
			if(extended_version > 1)
			{
				// Check for incompatible future versions
				dbg->fatal( "factory_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", extended_version );
			}
		}
		desc->expand_probability = 0;
		desc->expand_minimum = 0;
		desc->expand_range = 0;
		desc->expand_times = 0;
		desc->electric_boost = 256;
		desc->pax_boost = 0;
		desc->mail_boost = 0;
		desc->electric_demand = 65535;
		desc->pax_demand = 65535;
		desc->mail_demand = 65535;
		desc->base_max_distance_to_consumer = 65535;
		desc->base_max_distance_to_supplier = 65535;
		desc->field_output_divider = 1;
	} else if(version == 1)
	{
		// Versioned node, version 1
		desc->placement = (site_t)decode_uint16(p);
		desc->productivity = decode_uint16(p);
		desc->range = decode_uint16(p);
		desc->distribution_weight = decode_uint16(p);
		if(desc->distribution_weight < 1)
		{
			// Avoid divide by zero errors when
			// determining industry density figures.
			desc->distribution_weight = 1;
		}
		desc->color = (uint8)decode_uint16(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		desc->pax_level = decode_uint16(p);
		desc->fields = 0;
		desc->expand_probability = 0;
		desc->expand_minimum = 0;
		desc->expand_range = 0;
		desc->expand_times = 0;
		desc->electric_boost = 256;
		desc->pax_boost = 0;
		desc->mail_boost = 0;
		desc->electric_demand = 65535;
		desc->pax_demand = 65535;
		desc->mail_demand = 65535;
		desc->base_max_distance_to_consumer = 65535;
		desc->base_max_distance_to_supplier = 65535;
		desc->field_output_divider = 1;
	}

	else
	{
		// old node, version 0, without pax_level
		DBG_DEBUG("factory_reader_t::read_node()","version=0");
		desc->placement = (site_t)v;
		decode_uint16(p);	// alsways zero
		desc->productivity = decode_uint16(p)|0x8000;
		desc->range = decode_uint16(p);
		desc->distribution_weight = decode_uint16(p);
		if(desc->distribution_weight < 1)
		{
			// Avoid divide by zero errors when
			// determining industry density figures.
			desc->distribution_weight = 1;
		}
		desc->color = (uint8)decode_uint16(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		desc->pax_level = 12;
		desc->fields = 0;
		desc->expand_probability = 0;
		desc->expand_minimum = 0;
		desc->expand_range = 0;
		desc->expand_times = 0;
		desc->electric_boost = 256;
		desc->pax_boost = 0;
		desc->mail_boost = 0;
		desc->electric_demand = 65535;
		desc->pax_demand = 65535;
		desc->mail_demand = 65535;
		desc->base_max_distance_to_consumer = 65535;
		desc->base_max_distance_to_supplier = 65535;
		desc->field_output_divider = 1;
	}

	if(!extended)
	{
		desc->electricity_proportion = 17;
		desc->inverse_electricity_proportion = 100 / desc->electricity_proportion;
		desc->upgrades = 0;
	}

	desc->max_distance_to_consumer = desc->base_max_distance_to_consumer;
	desc->max_distance_to_supplier = desc->base_max_distance_to_supplier;

	DBG_DEBUG("factory_reader_t::read_node()",
		"version=%i, place=%i, productivity=%i, suppliers=%i, products=%i, fields=%i, range=%i, level=%i"
		 "Demands: pax=%i / mail=%i / electric=%i,"
		 "Boosts: pax=%i / mail=%i / electric=%i,"
		 "Expand: prob=%i / min=%i / range=%i / times=%i"
		 "chance=%i, sound: id=%i / interval=%i, color=%i",
		version,
		desc->placement,
		desc->productivity,
		desc->supplier_count,
		desc->product_count,
		desc->fields,
		desc->range,
		desc->pax_level,
		desc->pax_demand,
		desc->mail_demand,
		desc->electric_demand,
		desc->pax_boost,
		desc->mail_boost,
		desc->electric_boost,
		desc->expand_probability,
		desc->expand_minimum,
		desc->expand_range,
		desc->expand_times,
		desc->distribution_weight,
		desc->sound_id,
		desc->sound_interval,
		desc->color);

	if ((sint16)desc->sound_id == LOAD_SOUND) {
		uint8 len = decode_sint8(p);
		char wavname[256];
		wavname[len] = 0;
		for (uint8 i = 0; i<len; i++) {
			wavname[i] = decode_sint8(p);
		}
		desc->sound_id = (sint8)sound_desc_t::get_sound_id(wavname);
		DBG_MESSAGE("vehicle_reader_t::register_obj()", "sound %s to %i", wavname, desc->sound_id);

	}
	else if (desc->sound_id >= 0 && desc->sound_id <= MAX_OLD_SOUNDS) {
		sint16 old_id = desc->sound_id;
		desc->sound_id = (sint8)sound_desc_t::get_compatible_sound_id((sint8)old_id);
		DBG_MESSAGE("vehicle_reader_t::register_obj()", "old sound %i to %i", old_id, desc->sound_id);
	}

	return desc;
}


void factory_reader_t::register_obj(obj_desc_t *&data)
{
	factory_desc_t* desc = static_cast<factory_desc_t*>(data);
	size_t fab_name_len = strlen( desc->get_name() );
	desc->electricity_producer = ( fab_name_len>11   &&  (strcmp(desc->get_name()+fab_name_len-9, "kraftwerk")==0  ||  strcmp(desc->get_name()+fab_name_len-11, "Power Plant")==0) );
	factory_builder_t::register_desc(desc);
	obj_for_xref(get_type(), desc->get_name(), data);
}


bool factory_reader_t::successfully_loaded() const
{
	return factory_builder_t::successfully_loaded();
}
