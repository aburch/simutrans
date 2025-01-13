/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "factory_desc.h"
#include "xref_desc.h"
#include "../network/checksum.h"


void field_class_desc_t::calc_checksum(checksum_t* chk) const
{
#if MSG_LEVEL>0
	PAKSET_INFO("--\n", "obj=field\nname=%s", get_name());
	if (get_copyright()) PAKSET_INFO("copyright=", "%s", get_copyright());
	if (snow_image) PAKSET_INFO("has_snow=", "1");
	PAKSET_INFO("production=", "%d", production_per_field);
	if (storage_capacity) PAKSET_INFO("capacity=", "%d", storage_capacity);
	PAKSET_INFO("spawn_weight=", "%d", spawn_weight);
#endif
	chk->input(production_per_field);
	chk->input(storage_capacity);
	chk->input(spawn_weight);
}


void field_group_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input(probability);
	chk->input(max_fields);
	chk->input(min_fields);
	chk->input(field_classes);
	for(uint16 i=0; i<field_classes; i++) {
		const field_class_desc_t *fc = get_field_class(i);
		fc->calc_checksum(chk);
	}
}


void factory_supplier_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input(capacity);
	chk->input(supplier_count);
	chk->input(consumption);
	chk->input(get_input_type()->get_name());
}


void factory_product_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input(capacity);
	chk->input(factor);
	chk->input(get_output_type()->get_name());
}


void factory_desc_t::correct_smoke()
{
	if(  smokerotations == 0   &&  get_smoke()  ) {
		// old type of factory, we have to build the tile and smoke offsets here
		const smoke_desc_t *oldsmoke = get_smoke();
		const koord size = get_building()->get_size(0)-koord(1,1);

		smokerotations = get_building()->get_all_layouts();
		for( int i = 0; i < smokerotations; i++ ) {
			smoketile[i] = oldsmoke->get_pos_off(size,i);
			smokeoffset[i] = oldsmoke->get_xy_off(i);
			smokeoffset[i] -= koord(0, LEGACY_SMOKE_YOFFSET);
		}
		smokeuplift = DEFAULT_SMOKE_UPLIFT;
		smokelifetime = DEFAULT_FACTORYSMOKE_TIME;
	}
}


void factory_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input((uint8)placement);
	chk->input(productivity);
	chk->input(range);
	chk->input(distribution_weight);
	chk->input(color);
	chk->input(supplier_count);
	chk->input(product_count);
	chk->input(fields);
	chk->input(pax_level);
	chk->input(electricity_producer);
	chk->input(expand_probability);
	chk->input(expand_minimum);
	chk->input(expand_range);
	chk->input(expand_times);
	chk->input(electric_boost);
	chk->input(pax_boost);
	chk->input(mail_boost);
	chk->input(electric_demand);
	chk->input(pax_demand);
	chk->input(mail_demand);

	for (uint8 i=0; i<supplier_count; i++) {
		const factory_supplier_desc_t *supp = get_supplier(i);
		supp->calc_checksum(chk);
	}

	for (uint8 i=0; i<product_count; i++) {
		const factory_product_desc_t *prod = get_product(i);
		prod->calc_checksum(chk);
	}

#if MSG_LEVEL>0
	PAKSET_INFO("obj=factory", "");
	PAKSET_INFO("productivity=", "%d", productivity);
	PAKSET_INFO("range=", "%d", range);
	PAKSET_INFO("distribution_weight=", "%d", distribution_weight); // there is probably the same in the chance field in the building part
	PAKSET_INFO("mapcolor=", "%d", color);
	if (pax_level != 12) PAKSET_INFO("pax_level=", "%d", pax_level);
	if (expand_minimum != 0) PAKSET_INFO("expand_minimum=", "%d", expand_minimum);
	if (expand_range != 0) PAKSET_INFO("expand_range=", "%d", expand_range);
	if (expand_times != 0) PAKSET_INFO("expand_times=", "%d", expand_times);
	if (electric_demand != 65535u) PAKSET_INFO("electricity_demand=", "%d", electric_demand);
	if (pax_demand != 65535u) PAKSET_INFO("passenger_demand=", "%d", pax_demand);
	if (mail_demand != 65535u) PAKSET_INFO("mail_demand=", "%d", mail_demand);
	if (expand_times != 0) PAKSET_INFO("expand_times=", "%d", expand_times);
	if (electric_boost != 256) PAKSET_INFO("electricity_boost=", "%d", (electric_boost*1000)/256);
	if (pax_boost != 0) PAKSET_INFO("passenger_boost=", "%d", pax_boost);
	if (mail_boost != 0) PAKSET_INFO("mail_boost=", "%d", mail_boost);
	if (sound_interval != 0xFFFFFFFFul) PAKSET_INFO("sound_interval=", "%lu", sound_interval);
#endif

	get_building()->calc_checksum(chk);

#if MSG_LEVEL>0
	if (smokerotations > 0) {
		if (smokeuplift != DEFAULT_SMOKE_UPLIFT) PAKSET_INFO("smokeuplift=", "%lu", smokeuplift);
		if (smokelifetime != DEFAULT_FACTORYSMOKE_TIME) PAKSET_INFO("smokelifetime=", "%lu", smokelifetime);
		for (int i = 0; i < smokerotations; i++) {
			PAKSET_INFO("smoketile", "[%d]=%d,%d", i, smoketile[i].x, smoketile[i].y);
			PAKSET_INFO("smokeoffset", "[%d]=%d,%d", i, smokeoffset[i].x, smokeoffset[i].y);
		}
	}
	else if (smoketile[0].x | smoketile[0].y | smokeoffset[0].x | smokeoffset[0].y) {
		if (smokeuplift != DEFAULT_SMOKE_UPLIFT) PAKSET_INFO("smokeuplift=", "%lu", smokeuplift);
		if (smokelifetime != DEFAULT_FACTORYSMOKE_TIME) PAKSET_INFO("smokelifetime=", "%lu", smokelifetime);
		PAKSET_INFO("smoketile", "[%d]=%d,%d", 0, smoketile[0].x, smoketile[0].y);
		PAKSET_INFO("smokeoffset", "[%d]=%d,%d", 0, smokeoffset[0].x, smokeoffset[0].y);
	}
	if (const field_group_desc_t* field_group = get_field_group()) {
		if (int count = field_group->get_field_class_count()) {
			PAKSET_INFO("probability_to_spawn", "%d", field_group->get_probability());
			PAKSET_INFO("min_fields", "%d", field_group->get_min_fields());
			PAKSET_INFO("max_fields", "%d", field_group->get_max_fields());
			PAKSET_INFO("start_fields", "%d", field_group->get_start_fields());
			for (int i = 0; i < count; i++) {
				PAKSET_INFO("fields", "[%d]=%s", i, field_group->get_field_class(i)->get_name());
			}
		}
	}
	for (uint8 i = 0; i < supplier_count; i++) {
		const factory_supplier_desc_t* supp = get_supplier(i);
		PAKSET_INFO("inputgood", "[%d]=%s", i, supp->get_input_type()->get_name());
		PAKSET_INFO("inputsupplier", "[%d]=%d", i, supp->get_supplier_count());
		PAKSET_INFO("inputcapacity", "[%d]=%d", i, supp->get_capacity());
		PAKSET_INFO("inputfactor", "[%d]=%d", i, supp->get_consumption());
	}
	for (uint8 i = 0; i < product_count; i++) {
		const factory_product_desc_t* prod = get_product(i);
		PAKSET_INFO("outputgood", "[%d]=%s", i, prod->get_output_type()->get_name());
		PAKSET_INFO("outputcapacity", "[%d]=%d", i, prod->get_capacity());
		PAKSET_INFO("outputfactor", "[%d]=%d", i, prod->get_factor());
	}

#endif
	// moved here, since the field class write out the field entries
	if (const field_group_desc_t* field_group = get_field_group()) {
		field_group->calc_checksum(chk);
	}

	PAKSET_INFO("--", "");
}
