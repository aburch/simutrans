/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../vehicle/simpeople.h"
#include "../pedestrian_desc.h"
#include "../obj_node_info.h"

#include "pedestrian_reader.h"
#include "../../network/pakset_info.h"


void pedestrian_reader_t::register_obj(obj_desc_t *&data)
{
	pedestrian_desc_t *desc = static_cast<pedestrian_desc_t  *>(data);

	pedestrian_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool pedestrian_reader_t::successfully_loaded() const
{
	return pedestrian_t::successfully_loaded();
}


/**
 * Read a pedestrian info node. Does version check and
 * compatibility transformations.
 */
obj_desc_t * pedestrian_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	pedestrian_desc_t *desc = new pedestrian_desc_t();

	// Read data
	fread(desc_buf, node.size, 1, fp);
	char * p = desc_buf;

	// old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Extended
	// @author: jamespetts

	uint16 extended_version = 0;
	const bool extended = version > 0 ? v & EX_VER : false;
	if(version > 0)
	{
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
	}

	desc->steps_per_frame = 0;
	desc->offset = 20;

	if(version == 0) {
		// old, nonversion node
		desc->distribution_weight = v;

		// This was a spare datum set to zero on all older versions
		uint16 intro = decode_uint16(p);
		if (intro > 0)
		{
			desc->intro_date = intro;
			desc->retire_date = decode_uint16(p);
		}
	}
	else if (version == 1) {
		desc->distribution_weight = decode_uint16(p);
		desc->steps_per_frame = decode_uint16(p);
		desc->offset = decode_uint16(p);

		// Extended only
		if(extended)
		{
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
		}
	}

	DBG_DEBUG("pedestrian_reader_t::read_node()", "version=%i, chance=%i", version, desc->distribution_weight);
	return desc;
}
