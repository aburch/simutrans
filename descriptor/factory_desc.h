/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_FACTORY_DESC_H
#define DESCRIPTOR_FACTORY_DESC_H


#include "obj_desc.h"
#include "building_desc.h"
#include "skin_desc.h"
#include "goods_desc.h"
#include "../dataobj/koord.h"
#include "../tpl/weighted_vector_tpl.h"


#define DEFAULT_FACTORYSMOKE_TIME (2499)
#define DEFAULT_SMOKE_UPLIFT (16)
#define LEGACY_SMOKE_YOFFSET (8)


class checksum_t;

/**
 * this desc will store data specific to each class of fields
 * Fields are xref'ed from skin_desc_t
 */
class field_class_desc_t : public obj_desc_t {
	friend class factory_field_class_reader_t;
	friend class factory_field_group_reader_t; // this is a special case due to desc restructuring

private:
	uint8  snow_image;           // 0 or 1 for snow
	uint16 production_per_field;
	uint16 storage_capacity;
	uint16 spawn_weight;

public:
	skin_desc_t const* get_images() const { return get_child<skin_desc_t>(0); }
	const char *get_name() const { return get_images()->get_name(); }
	const char *get_copyright() const { return get_images()->get_copyright(); }

	uint8 has_snow_image() const { return snow_image; }
	uint16 get_field_production() const { return production_per_field; }
	uint16 get_storage_capacity() const { return storage_capacity; }
	uint16 get_spawn_weight() const { return spawn_weight; }

	void calc_checksum(checksum_t *chk) const;
};


// this desc now only contains common, shared data regarding fields
class field_group_desc_t : public obj_desc_t {
	friend class factory_field_group_reader_t;

private:
	uint16 probability;     // between 0 ...10000
	uint16 max_fields;      // maximum number of fields around a single factory
	uint16 min_fields;      // minimum number of fields around a single factory
	uint16 start_fields;    // number of fields between min and start_fields to spawn on creation
	uint16 field_classes;   // number of field classes

	weighted_vector_tpl<uint16> field_class_indices;

public:
	// fills the array, is only called once during successfully_loaded() after resolve xrefs
	void init_field_class_indices()
	{
		if(  field_classes>0  ) {
			field_class_indices.clear();
			field_class_indices.resize( field_classes );
			for(  uint16 i=0  ;  i<field_classes  ;  ++i  ) {
				field_class_indices.append( i, get_field_class(i)->get_spawn_weight() );
			}
		}
	}

	uint16 get_probability() const { return probability; }
	uint16 get_max_fields() const { return max_fields; }
	uint16 get_min_fields() const { return min_fields; }
	uint16 get_start_fields() const { return start_fields; }
	uint16 get_field_class_count() const { return field_classes; }
	field_class_desc_t const* get_field_class(uint16 const idx) const { return idx < field_classes ? get_child<field_class_desc_t>(idx) : 0; }
	const weighted_vector_tpl<uint16> &get_field_class_indices() const { return field_class_indices; }

	void calc_checksum(checksum_t *chk) const;
};




/**
 * Smoke objects for factories.
 *
 * Child nodes:
 *  0   SKin
 */
class smoke_desc_t : public obj_desc_t {
	friend class factory_smoke_reader_t;

private:
	koord pos_off;
	koord xy_off;

public:
	const char *get_name() const { return get_images()->get_name(); }
	const char *get_copyright() const { return get_images()->get_copyright(); }
	skin_desc_t const* get_images() const { return get_child<skin_desc_t>(0); }

	// get the tile with the smoke
	koord get_pos_off( koord size, uint8 rotation) const {
		switch( rotation%4 ) {
			case 1: return koord( size.y-pos_off.y, pos_off.x );
			case 2: return koord( size.x-pos_off.x, size.y-pos_off.y );
			case 3: return koord( pos_off.y, size.x-pos_off.x );
		}
		return pos_off;
	}

	// offset in pixel (depends on OBJECT_OFFSET_STEPS==16)
	koord get_xy_off(uint8 rotation) const {
		switch( rotation%4 ) {
			case 1: return koord( 0, xy_off.y+xy_off.x/2 );
			case 2: return koord( -xy_off.x, xy_off.y );
			case 3: return koord( 0, xy_off.y-xy_off.x/2 );
		}
		return xy_off;
	}
};


/**
 * Information about required goods for production
 *
 * Child nodes:
 *  0   Ware
 */
class factory_supplier_desc_t : public obj_desc_t {
	friend class factory_supplier_reader_t;

private:
	uint16  capacity;
	uint16  supplier_count;
	uint16  consumption;

public:
	goods_desc_t const* get_input_type() const { return get_child<goods_desc_t>(0); }
	uint16 get_capacity() const { return capacity; }
	uint16 get_supplier_count() const { return supplier_count; }
	uint16 get_consumption() const { return consumption; }
	void calc_checksum(checksum_t *chk) const;
};


/**
 * Information about produced goods of a factory
 *
 * Child nodes:
 *  0   Ware
 */
class factory_product_desc_t : public obj_desc_t {
	friend class factory_product_reader_t;

private:
	uint16 capacity;

	/**
	 * How much of this product is derived from one unit of factory
	 * production? 256 means 1.0
	 */
	uint16 factor;

public:
	goods_desc_t const* get_output_type() const { return get_child<goods_desc_t>(0); }
	uint16 get_capacity() const { return capacity; }
	uint16 get_factor() const { return factor; }
	void calc_checksum(checksum_t *chk) const;
};


/**
 * Factory.
 *
 * Child nodes:
 *  0   House descriptor
 *  1   Smoke descriptor
 *  2   Supplier descriptor 1
 *  3   Supplier descriptor 2
 * ... ...
 *  n+1 Supplier descriptor n
 *  n+2 Consumer descriptor 1
 *  n+3 Consumer descriptor 2
 * ... ...
 */
class factory_desc_t : public obj_desc_t {
	friend class factory_reader_t;

public:
	enum site_t {
		Land,
		Water,
		City,
		river,
		shore,
		forest
	};

private:
	site_t placement;
	uint16 productivity;
	uint16 range;
	uint16 distribution_weight; // probability of construction of this factory
	uint8 color;
	uint16 supplier_count;
	uint16 product_count;
	uint8 fields; // only if there are any ...
	uint16 pax_level;
	bool electricity_producer;
	uint16 expand_probability;
	uint16 expand_minimum;
	uint16 expand_range;
	uint16 expand_times;
	uint16 electric_boost;
	uint16 pax_boost;
	uint16 mail_boost;
	uint16 electric_demand;
	uint16 pax_demand;
	uint16 mail_demand;
	uint16 smokeuplift;
	uint16 smokelifetime;
	koord  smoketile[4];
	koord  smokeoffset[4];
	uint8  smokerotations;
	sint8  sound_id;
	uint32 sound_interval;

public:
	const char *get_name() const { return get_building()->get_name(); }
	const char *get_copyright() const { return get_building()->get_copyright(); }
	building_desc_t  const* get_building()  const { return get_child<building_desc_t>(0); }

	smoke_desc_t const* get_smoke() const { return get_child<smoke_desc_t>(1); }
	void correct_smoke();
	koord get_smoketile( uint8 rot ) const { return smoketile[ rot%smokerotations ]; }
	koord get_smokeoffset( uint8 rot ) const { return smokeoffset[ rot%smokerotations ]; }
	uint16 get_smokeuplift() const { return smokeuplift; }
	uint16 get_smokelifetime() const { return smokelifetime; }

	// we must take care, for the case of no producer/consumer
	const factory_supplier_desc_t *get_supplier(uint16 i) const
	{
		return i < supplier_count ? get_child<factory_supplier_desc_t>(2 + i) : 0;
	}
	const factory_product_desc_t *get_product(uint16 i) const
	{
		return i < product_count ? get_child<factory_product_desc_t>(2 + supplier_count + i) : 0;
	}
	const field_group_desc_t *get_field_group() const {
		if(!fields) {
			return NULL;
		}
		return get_child<field_group_desc_t>(2 + supplier_count + product_count);
	}

	bool is_consumer_only() const { return product_count    == 0; }
	bool is_producer_only() const { return supplier_count == 0; }

	uint16 get_supplier_count() const { return supplier_count; }
	uint16 get_product_count() const { return product_count; }

	/* where to built */
	site_t get_placement() const { return placement; }
	uint16 get_distribution_weight() const { return distribution_weight; }

	PIXVAL get_color() const { return color_idx_to_rgb(color); }

	void set_productivity(uint16 p) { productivity=p; }
	uint16 get_productivity() const { return productivity; }
	uint16 get_range() const { return range; }

	/* level for mail and passenger generation */
	uint16 get_pax_level() const { return pax_level; }

	bool is_electricity_producer() const { return electricity_producer; }

	uint16 get_expand_probability() const { return expand_probability; }
	uint16 get_expand_minimum() const { return expand_minimum; }
	uint16 get_expand_range() const { return expand_range; }
	uint16 get_expand_times() const { return expand_times; }

	uint16 get_electric_boost() const { return electric_boost; }
	uint16 get_pax_boost() const { return pax_boost; }
	uint16 get_mail_boost() const { return mail_boost; }
	uint16 get_electric_demand() const { return electric_demand; }
	uint16 get_pax_demand() const { return pax_demand; }
	uint16 get_mail_demand() const { return mail_demand; }

	// more effects when producing
	sint8 get_sound() const { return sound_id; }
	uint32 get_sound_interval_ms() const { return sound_interval; }

	void calc_checksum(checksum_t *chk) const;
};

#endif
