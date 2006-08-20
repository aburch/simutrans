/*
 * halt_list_item.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifdef _MSC_VER
#include <string.h>
#endif

#include <stdio.h>

#include "halt_list_item.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simevent.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "../simimg.h"


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void halt_list_item_t::infowin_event(const event_t *ev)
{
    if(IS_LEFTRELEASE(ev)) {
	halt->zeige_info();
    }
}


/**
 * Zeichnet die Komponente
 * @author Markus Weber
 */
void halt_list_item_t::zeichnen(koord offset) const
{

    int halttype;       // 01-June-02    Markus Weber    Added

    clip_dimension clip = display_gib_clip_wh();

    if(! ((pos.y+offset.y) > clip.yy ||  (pos.y+offset.y) < clip.y-32) &&
	halt.is_bound()) {

	static cbuffer_t buf(8192);  // Hajo: changed from 128 to 8192 because get_short_freight_info requires that a large buffer

	buf.clear();
	buf.append(nummer);
	buf.append(".");

	display_proportional_clip(pos.x+offset.x+4, pos.y+offset.y+8,
                                  buf, ALIGN_LEFT, SCHWARZ, true);

	buf.clear();
	buf.append(translator::translate(halt->gib_name()));

        display_proportional_clip(pos.x+offset.x+33, pos.y+offset.y+8,
                                  buf, ALIGN_LEFT, SCHWARZ, true);


	// Hajo: removed call to translator, which doesn't made sense here
	buf.clear();
 	halt->get_short_freight_info(buf);
        display_proportional_clip(pos.x+offset.x+33, pos.y+offset.y+20,
                                  buf, ALIGN_LEFT, SCHWARZ, true);

	int left = 210;


 	halttype = halt->get_station_type();
 	if (halttype & haltestelle_t::railstation) {
	    display_img(skinverwaltung_t::zughaltsymbol->gib_bild_nr(0), pos.x+offset.x+left, pos.y+offset.y-37, false, false);
	  left += 23;
 	}
 	if (halttype & haltestelle_t::loadingbay) {
	    display_img(skinverwaltung_t::autohaltsymbol->gib_bild_nr(0), pos.x+offset.x+left, pos.y+offset.y-37, false, false);
	  left += 23;
 	}
 	if (halttype & haltestelle_t::busstop) {
	    display_img(skinverwaltung_t::bushaltsymbol->gib_bild_nr(0), pos.x+offset.x+left, pos.y+offset.y-37, false, false);
	  left += 23;
 	}
 	if (halttype & haltestelle_t::dock) {
	    display_img(skinverwaltung_t::shiffshaltsymbol->gib_bild_nr(0), pos.x+offset.x+left, pos.y+offset.y-37, false, false);
	  left += 23;
 	}


	const int color = halt->gib_status_farbe();

	display_fillbox_wh_clip(pos.x+offset.x+210,
				pos.y+offset.y+10,
				20,
				4,
				color,
				false);
    }
}
