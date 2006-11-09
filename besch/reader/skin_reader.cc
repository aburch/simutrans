#include <stdio.h>

#include "../../simdings.h"
#include "../../simdebug.h"
#include "../../simskin.h"

#include "../skin_besch.h"
#include "skin_reader.h"


void skin_reader_t::register_obj(obj_besch_t *&data)
{
	skin_besch_t* besch = static_cast<skin_besch_t*>(data);

    if(get_skintype() != skinverwaltung_t::nothing)
	skinverwaltung_t::register_besch(get_skintype(), besch);
    else
	obj_for_xref(get_type(), besch->gib_name(), data);
}


bool skin_reader_t::successfully_loaded() const
{
	DBG_MESSAGE("skin_reader_t::successfully_loaded()","");
    return skinverwaltung_t::alles_geladen(get_skintype());
}
