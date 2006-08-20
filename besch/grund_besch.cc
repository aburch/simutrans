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

#include "../simdebug.h"
#include "../simgraph.h"
#include "spezial_obj_tpl.h"
#include "grund_besch.h"

/* may change due to summer winter changes *
 * @author prissi
 */

#ifdef DOUBLE_GROUNDS
const uint8 grund_besch_t::slopetable[80] =
{
	0, 1 , 0xFF,
	2 , 3 , 0xFF, 0xFF, 0xFF, 0xFF,
	4 , 5, 0xFF, 6, 7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	8, 9, 0xFF, 10, 11, 0xFF, 0xFF, 0xFF, 0xFF, 12, 13, 0xFF,  14, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
#else
#endif

const grund_besch_t *grund_besch_t::boden = NULL;
const grund_besch_t *grund_besch_t::ufer = NULL;

/*
 *  static data
 */

const grund_besch_t *grund_besch_t::standard_boden = NULL;
const grund_besch_t *grund_besch_t::standard_ufer = NULL;
const grund_besch_t *grund_besch_t::winter_boden = NULL;
const grund_besch_t *grund_besch_t::winter_ufer = NULL;
const grund_besch_t *grund_besch_t::wasser = NULL;
const grund_besch_t *grund_besch_t::fundament = NULL;
const grund_besch_t *grund_besch_t::slopes = NULL;
const grund_besch_t *grund_besch_t::fences = NULL;
const grund_besch_t *grund_besch_t::marker = NULL;
const grund_besch_t *grund_besch_t::ausserhalb = NULL;

static spezial_obj_tpl<grund_besch_t> grounds[] = {
    { &grund_besch_t::standard_boden,	    "Gras" },
    { &grund_besch_t::standard_ufer,	    "Shore" },
    { &grund_besch_t::wasser,	    "Water" },
    { &grund_besch_t::fundament,    "Basement" },
    { &grund_besch_t::slopes,    "Slopes" },
    { &grund_besch_t::fences,   "Fence" },
    { &grund_besch_t::marker,   "Marker" },
    { &grund_besch_t::ausserhalb,   "Outside" },
    { NULL, NULL }
};

static spezial_obj_tpl<grund_besch_t> winter_grounds[] = {
    { &grund_besch_t::winter_boden,	    "WinterGras" },
    { &grund_besch_t::winter_ufer,	    "WinterShore" },
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
	if(strcmp("Outside", besch->gib_name())==0) {
		const bild_besch_t *bild = static_cast<const bildliste2d_besch_t *>(besch->gib_kind(2))->gib_bild(0,0);
		dbg->message("grund_besch_t::register_besch()","setting raster width to %i",bild->w);
		display_set_base_raster_width( bild->w );
	}
	if(!::register_besch(grounds, besch)) {
		return ::register_besch(winter_grounds, besch);
	}
	return true;
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
	DBG_MESSAGE("grund_besch_t::alles_geladen()","boden");
#if 0
	// if you want to know it in detail
	// standard is "summer"
	for(int i=0; i<78;  i++  ) {
		DBG_MESSAGE("boden","%i=%p",i,static_cast<const bildliste2d_besch_t *>(grund_besch_t::standard_boden->gib_kind(2))->gib_liste(i) );
	}
	for(int i=0; i<18;  i++  ) {
		DBG_MESSAGE("slopes","%i=%p",i,static_cast<const bildliste2d_besch_t *>(grund_besch_t::slopes->gib_kind(2))->gib_liste(i) );
	}
#endif
	grund_besch_t::boden = grund_besch_t::standard_boden;
	grund_besch_t::ufer = grund_besch_t::standard_ufer;
	return ::alles_geladen(grounds);
}
