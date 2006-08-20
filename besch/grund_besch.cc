/*
 *
 *  grund_besch.cpp
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

#include "spezial_obj_tpl.h"
#include "grund_besch.h"

/*
 *  static data
 */

const grund_besch_t *grund_besch_t::boden = NULL;
const grund_besch_t *grund_besch_t::ufer = NULL;
const grund_besch_t *grund_besch_t::wasser = NULL;
const grund_besch_t *grund_besch_t::fundament = NULL;
const grund_besch_t *grund_besch_t::ausserhalb = NULL;

static spezial_obj_tpl<grund_besch_t> spezial_objekte[] = {
    { &grund_besch_t::boden,	    "Gras" },
    { &grund_besch_t::ufer,	    "Shore" },
    { &grund_besch_t::wasser,	    "Water" },
    { &grund_besch_t::fundament,    "Basement" },
    { &grund_besch_t::ausserhalb,   "Outside" },
    { NULL, NULL }
};

/*
 *  member function:
 *      grund_besch_t::register_besch()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Ein weiteres geladenes Objekt rgistrieren.
 *
 *  Return type:
 *      bool
 *
 *  Argumente:
 *      const grund_besch_t *besch
 */
bool grund_besch_t::register_besch(const grund_besch_t *besch)
{
    return ::register_besch(spezial_objekte, besch);
}

/*
 *  member function:
 *      grund_besch_t::alles_geladen()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Alle zwingend erforderlich
 *
 *  Return type:
 *      bool
 */
bool grund_besch_t::alles_geladen()
{
    return ::alles_geladen(spezial_objekte);
}
