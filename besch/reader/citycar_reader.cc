/*
 *
 *  citycar_reader.cpp
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

#include "../../tpl/stringhashtable_tpl.h"

#include "../../simverkehr.h"
#include "../stadtauto_besch.h"

#include "citycar_reader.h"


/*
 *  member function:
 *      citycar_reader_t::register_obj()
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
void citycar_reader_t::register_obj(obj_besch_t *&data)
{
    stadtauto_besch_t *besch = static_cast<stadtauto_besch_t *>(data);

    stadtauto_t::register_besch(besch);
//    printf("...Stadtauto %s geladen\n", besch->gib_name());
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      citycar_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool citycar_reader_t::successfully_loaded() const
{
    return stadtauto_t::laden_erfolgreich();
}
