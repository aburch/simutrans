/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h"

#include "../way_desc.h"
#include "../intro_dates.h"
#include "../../builder/wegbauer.h"

#include "way_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"
#include "../../tpl/array_tpl.h"


void way_reader_t::register_obj(obj_desc_t *&data)
{
	way_desc_t *desc = static_cast<way_desc_t *>(data);

	way_builder_t::register_desc(desc);
	pakset_manager_t::obj_for_xref(get_type(), desc->get_name(), data);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool way_reader_t::successfully_loaded() const
{
	return way_builder_t::successfully_loaded();
}


obj_desc_t * way_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	array_tpl<char> desc_buf(node.size);
	if (fread(desc_buf.begin(), node.size, 1, fp) != 1) {
		return NULL;
	}
	char *p = desc_buf.begin();

	// old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	int version = 0;
	way_desc_t *desc = new way_desc_t();

	if(node.size == 0) {
		// old node, version 0, compatibility code
		desc->price = 10000;
		desc->maintenance = 800;
		desc->topspeed = 999;
		desc->max_weight = 999;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->retire_date = DEFAULT_RETIRE_DATE*12;
		desc->wtyp = road_wt;
		desc->styp = type_flat;
		desc->draw_as_obj = false;
		desc->number_of_seasons = 0;
	}
	else {
		const uint16 v = decode_uint16(p);
		version = v & 0x7FFF;

		if(version==6) {
			// version 6, now with axle load
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->max_weight = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->axle_load = decode_uint16(p); // new
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = decode_uint8(p);
			desc->number_of_seasons = decode_sint8(p);
		}
		else if(version==4  ||  version==5) {
			// Versioned node, version 4+5
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->max_weight = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = decode_uint8(p);
			desc->number_of_seasons = decode_sint8(p);
		}
		else if(version==3) {
			// Versioned node, version 3
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->max_weight = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = decode_uint8(p);
			desc->number_of_seasons = 0;
		}
		else if(version==2) {
			// Versioned node, version 2
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->max_weight = decode_uint32(p);
			desc->intro_date = decode_uint16(p);
			desc->retire_date = decode_uint16(p);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->draw_as_obj = false;
			desc->number_of_seasons = 0;
		}
		else if(version == 1) {
			// Versioned node, version 1
			desc->price = decode_uint32(p);
			desc->maintenance = decode_uint32(p);
			desc->topspeed = decode_uint32(p);
			desc->max_weight = decode_uint32(p);
			uint32 intro_date= decode_uint32(p);
			desc->intro_date = (intro_date/16)*12 + (intro_date%16);
			desc->wtyp = decode_uint8(p);
			desc->styp = decode_uint8(p);
			desc->retire_date = DEFAULT_RETIRE_DATE*12;
			desc->draw_as_obj = false;
			desc->number_of_seasons = 0;
		}
		else {
			dbg->fatal( "way_reader_t::read_node()", "Cannot handle too new node version %i", version );
		}
	}

	// some internal corrections to pay for previous confusion with two waytypes
	if(desc->wtyp==tram_wt) {
		desc->styp = type_tram;
		desc->wtyp = track_wt;
	}
	else if(desc->styp==5  &&  desc->wtyp==track_wt) {
		desc->wtyp = monorail_wt;
		desc->styp = type_flat;
	}
	else if(desc->wtyp==128) {
		desc->wtyp = powerline_wt;
	}

	if(version<=2  &&  desc->wtyp==air_wt  &&  desc->topspeed>=250) {
		// runway!
		desc->styp = type_runway;
	}

	if(  version < 6  ) {
		desc->axle_load = 9999;
	}

	// front images from version 5 on
	desc->front_images = version > 4;

	DBG_DEBUG("way_reader_t::read_node()",
	     "version=%d, price=%d, maintenance=%d, topspeed=%d, max_weight=%d, "
	     "wtype=%d, styp=%d, intro=%i/%i, retire=%i/%i, axle_load=%d, ding=%i, seasons=%i",
	     version,
	     desc->price,
	     desc->maintenance,
	     desc->topspeed,
	     desc->max_weight,
	     desc->wtyp,
	     desc->styp,
	     (desc->intro_date%12)+1,
	     desc->intro_date/12,
	     (desc->retire_date%12)+1,
	     desc->retire_date/12,
	     desc->axle_load,
	     desc->draw_as_obj,
	     desc->number_of_seasons);

	return desc;
}
