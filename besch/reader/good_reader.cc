#include <stdio.h>
#include "../../simdebug.h"
#include "../../bauer/warenbauer.h"

#include "good_reader.h"
#include "../obj_node_info.h"
#include "../ware_besch.h"
#include "../../network/pakset_info.h"


void good_reader_t::register_obj(obj_besch_t *&data)
{
	ware_besch_t *besch = static_cast<ware_besch_t *>(data);

	warenbauer_t::register_besch(besch);
	DBG_DEBUG("good_reader_t::register_obj()","loaded good '%s'", besch->get_name());

	obj_for_xref(get_type(), besch->get_name(), data);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool good_reader_t::successfully_loaded() const
{
	return warenbauer_t::alles_geladen(); //"Alles geladen" = "Everything laoded" (Babelfish)
}


obj_besch_t * good_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	ware_besch_t *besch = new ware_besch_t();

	// some defaults
	besch->speed_bonus = 0;
	besch->weight_per_unit = 100;
	besch->color = 255;

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
		experimental_version -= 1;
	}

	if(version == 1) {
		// Versioned node, version 1
		besch->base_values.append(fare_stage_t(0, decode_uint16(p)));
		besch->catg = (uint8)decode_uint16(p);
		besch->speed_bonus = decode_uint16(p);
		besch->weight_per_unit = 100;

	} else if(version == 2) {
		// Versioned node, version 2
		besch->base_values.append(fare_stage_t(0, decode_uint16(p)));
		besch->catg = (uint8)decode_uint16(p);
		besch->speed_bonus = decode_uint16(p);
		besch->weight_per_unit = decode_uint16(p);

	} else if(version == 3) {
		// Versioned node, version 3
		const uint16 base_value = decode_uint16(p);
		besch->catg = decode_uint8(p);
		besch->speed_bonus = decode_uint16(p);
		besch->weight_per_unit = decode_uint16(p);
		besch->color = decode_uint8(p);
		if(experimental)
		{
			const uint8 fare_stages = decode_uint8(p);
			if(fare_stages > 0)
			{
				// The base value is not used if fare stages are used. 
				for(int i = 0; i < fare_stages; i ++)
				{
					const uint16 to_distance = decode_uint16(p);
					const uint16 val = decode_uint16(p);
					besch->base_values.append(fare_stage_t((uint32)to_distance, val));
				}
			}
			else
			{
				besch->base_values.append(fare_stage_t(0, base_value));
			}
		}
		else
		{
			besch->base_values.append(fare_stage_t(0, base_value));
		}
	}
	else {
		// old node, version 0
		besch->base_values.append(fare_stage_t(0, v));
		besch->catg = (uint8)decode_uint16(p);
	}

	DBG_DEBUG("good_reader_t::read_node()","version=%d value=%d catg=%d bonus=%d",version, besch->base_values.get_count() > 0 ? besch->base_values[0].price : 0, besch->catg, besch->speed_bonus);


  return besch;
}
