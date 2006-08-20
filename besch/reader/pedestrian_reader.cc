/*
 *
 *  pedestrian_reader.cpp
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */

#include <stdio.h>

#include "../../simpeople.h"
#include "../fussgaenger_besch.h"

#include "pedestrian_reader.h"

/*
 *  member function:
 *      pedestrian_reader_t::register_obj()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Argumente:
 *      obj_besch_t *&data
 */
void pedestrian_reader_t::register_obj(obj_besch_t *&data)
{
    fussgaenger_besch_t *besch = static_cast<fussgaenger_besch_t  *>(data);

    fussgaenger_t::register_besch(besch);

    obj_for_xref(get_type(), besch->gib_name(), data);
//    printf("...Fussgaenger %s geladen\n", besch->gib_name());
}



bool pedestrian_reader_t::successfully_loaded() const
{
    return fussgaenger_t::laden_erfolgreich();
}
