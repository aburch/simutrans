#include <stdio.h>

#include "../../simobj.h"
#include "../../simdebug.h"
#include "../../simskin.h"

#include "../../obj/wolke.h"

#include "../skin_besch.h"
#include "skin_reader.h"



void skin_reader_t::register_obj(obj_besch_t *&data)
{
	skin_besch_t* besch = static_cast<skin_besch_t*>(data);

	if(get_skintype() != skinverwaltung_t::nothing) {
		skinverwaltung_t::register_besch(get_skintype(), besch);
	}
	else {
		obj_for_xref(get_type(), besch->get_name(), data);
		// smoke needs its own registering
		if(  get_type()==obj_smoke  ) {
			wolke_t::register_besch(besch);
		}
	}
}


bool skin_reader_t::successfully_loaded() const
{
	DBG_MESSAGE("skin_reader_t::successfully_loaded()","");
	return skinverwaltung_t::alles_geladen(get_skintype());
}


obj_besch_t* skin_reader_t::read_node(FILE*, obj_node_info_t& info)
{
	return obj_reader_t::read_node<skin_besch_t>(info);
}
