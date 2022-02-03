/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../../obj/simobj.h"
#include "../../simdebug.h"
#include "../../simskin.h"

#include "../../obj/wolke.h"

#include "../skin_desc.h"
#include "skin_reader.h"



void skin_reader_t::register_obj(obj_desc_t *&data)
{
	skin_desc_t* desc = static_cast<skin_desc_t*>(data);

	if(get_skintype() != skinverwaltung_t::nothing) {
		skinverwaltung_t::register_desc(get_skintype(), desc);
	}
	else {
		obj_for_xref(get_type(), desc->get_name(), data);
		// smoke needs its own registering
		if(  get_type()==obj_smoke  ) {
			wolke_t::register_desc(desc);
		}
	}
}


bool skin_reader_t::successfully_loaded() const
{
	DBG_MESSAGE("skin_reader_t::successfully_loaded()","");
	return skinverwaltung_t::successfully_loaded(get_skintype());
}


obj_desc_t* skin_reader_t::read_node(FILE*, obj_node_info_t& info)
{
	return obj_reader_t::read_node<skin_desc_t>(info);
}
