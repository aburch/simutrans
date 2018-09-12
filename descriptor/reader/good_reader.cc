/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"
#include "../../bauer/goods_manager.h"

#include "good_reader.h"
#include "../obj_node_info.h"
#include "../goods_desc.h"
#include "../../network/pakset_info.h"


void goods_reader_t::register_obj(obj_desc_t *&data)
{
	goods_desc_t *desc = static_cast<goods_desc_t *>(data);

	goods_manager_t::register_desc(desc);
	DBG_DEBUG("goods_reader_t::register_obj()","loaded good '%s'", desc->get_name());

	obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool goods_reader_t::successfully_loaded() const
{
	return goods_manager_t::successfully_loaded();
}


obj_desc_t * goods_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	goods_desc_t *desc = new goods_desc_t();

	// some defaults
	desc->speed_bonus = 0;
	desc->weight_per_unit = 100;
	desc->color = 255;

	// Read data
	fread(desc_buf, node.size, 1, fp);

	char * p = desc_buf;

	// old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Extended
	//@author: jamespetts
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
		extended_version -= 1;
	}

	if(version == 1) {
		// Versioned node, version 1
		desc->base_values.append(fare_stage_t(0, decode_uint16(p)));
		desc->catg = (uint8)decode_uint16(p);
		desc->speed_bonus = decode_uint16(p);
		desc->weight_per_unit = 100;

	} else if(version == 2) {
		// Versioned node, version 2
		desc->base_values.append(fare_stage_t(0, decode_uint16(p)));
		desc->catg = (uint8)decode_uint16(p);
		desc->speed_bonus = decode_uint16(p);
		desc->weight_per_unit = decode_uint16(p);

	} else if(version == 3) {
		// Versioned node, version 3
		const uint16 base_value = decode_uint16(p);
		desc->catg = decode_uint8(p);
		desc->speed_bonus = decode_uint16(p);
		desc->weight_per_unit = decode_uint16(p);
		desc->color = decode_uint8(p);
		if(extended)
		{
			if (extended_version >= 1)
			{
				desc->number_of_classes = decode_uint8(p);
			}
			const uint8 fare_stages = decode_uint8(p);
			if(fare_stages > 0)
			{
				// The base value is not used if fare stages are used.
				for(int i = 0; i < fare_stages; i ++)
				{
					const uint16 to_distance = decode_uint16(p);
					const uint16 val = decode_uint16(p);
					desc->base_values.append(fare_stage_t((uint32)to_distance, val));
				}
			}
			else
			{
				desc->base_values.append(fare_stage_t(0, base_value));
			}

			if (extended_version >= 1)
			{
				uint16 class_revenue_percentage;
				for (uint8 i = 0; i < desc->number_of_classes; i++)
				{
					class_revenue_percentage = decode_uint16(p);
					desc->class_revenue_percentages.append(class_revenue_percentage);
				}
			}
		}
		else
		{
			desc->base_values.append(fare_stage_t(0, base_value));
		}
	}
	else {
		// old node, version 0
		desc->base_values.append(fare_stage_t(0, v));
		desc->catg = (uint8)decode_uint16(p);
	}

	if (!extended || extended_version < 1)
	{
		desc->number_of_classes = 1;
		desc->class_revenue_percentages.append(100);
	}

	DBG_DEBUG("goods_reader_t::read_node()", "version=%d, value=%d, catg=%d, bonus=%d, weight=%i, color=%i",
		version,
		desc->base_values.get_count() > 0 ? desc->base_values[0].price : 0,
		desc->catg,
		desc->speed_bonus,
		desc->weight_per_unit,
		desc->color,
		desc->number_of_classes);


  return desc;
}
