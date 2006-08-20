/*
 * world_view_t.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "world_view_t.h"
#include "../simevent.h"
#include "../simworld.h"
#include "../simgraph.h"
#include "../boden/grund.h"
#include "../dataobj/umgebung.h"


world_view_t::world_view_t(karte_t *welt, koord location)
{
    this->location = location;
    this->welt = welt;

    setze_groesse(koord(64,56));
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void
world_view_t::infowin_event(const event_t *ev)
{
    if(IS_LEFTRELEASE(ev)) {
	if(welt->ist_in_kartengrenzen(location)) {
	    welt->zentriere_auf(location);
	}
    }
}


static const koord offsets[18] =
{
    koord(-2, -1),
    koord(-32, -48),
    koord(-1, -2),
    koord(32, -48),

    koord(-1, -1),
    koord(0, -32),

    koord(-1, 0),
    koord(-32, -16),
    koord(0, -1),
    koord(32, -16),

    koord(0, 0),
    koord(0, 0),

    koord(0, 1),
    koord(-32, 16),
    koord(1, 0),
    koord(32, 16),

    koord(1, 1),
    koord(0, 32),
};


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void
world_view_t::zeichnen(koord offset) const
{
    const planquadrat_t * plan = welt->lookup(location);

    if(plan) {
        const int raster = get_tile_raster_width();
	const int scale = raster/64;

        const int hgt = (plan->gib_kartenboden()->gib_hoehe() - 12)*scale;

	const koord pos = gib_pos()+offset;
	int i;

	PUSH_CLIP(pos.x, pos.y, 64, 56);

	const koord display_off = koord( -((raster-64)/2), -((raster-64)/2));


	const int show_names = umgebung_t::show_names;
	umgebung_t::show_names = 0;

	for(i=0; i<18; i+=2) {
	    const koord k = location + offsets[i];
	    const koord off = offsets[i+1]*scale;

	    plan = welt->lookup(k);
	    if(plan) {
		plan->display_boden(pos.x+off.x+display_off.x,
				    pos.y+off.y+display_off.y+hgt,
				    false);
	    }
	}

	for(i=0; i<18; i+=2) {
	    const koord k = location + offsets[i];
	    const koord off = offsets[i+1]*scale;

	    plan = welt->lookup(k);
	    if(plan) {
		plan->display_dinge(pos.x+off.x+display_off.x,
				    pos.y+off.y+hgt+display_off.y,
				    false);
	    }
	}

	umgebung_t::show_names = show_names;

	POP_CLIP();
    }
}
