/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

#include <stdio.h>

#include "ifc/karte_modell.h"
#include "simview.h"
#include "simgraph.h"


karte_ansicht_t::karte_ansicht_t(karte_modell_t *welt)
{
    this->welt = welt;
}


void
karte_ansicht_t::display(bool dirty)
{
	scale = get_tile_raster_width()/64;

	// zuerst den boden zeichnen
	// denn der Boden kann kein Objekt verdecken
	dirty = dirty || welt->ist_dirty();
	welt->setze_dirty_zurueck();

	const int IMG_SIZE = get_tile_raster_width();

	const int const_x_off = gib_anzeige_breite()/2 + welt->gib_x_off();
	const int dpy_width = gib_anzeige_breite()/IMG_SIZE + 2;
	const int dpy_height = (gib_anzeige_hoehe()*4)/IMG_SIZE;

	const int i_off = welt->gib_ij_off().x;
	const int j_off = welt->gib_ij_off().y;
	int	y;

	for(y=-12; y<dpy_height+15; y++) {

		const int ypos = y*IMG_SIZE/4+16 + welt->gib_y_off();

		for(int x=-dpy_width + (y & 1); x<=dpy_width+2; x+=2) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*IMG_SIZE/2 + const_x_off;

			display_boden(i, j, xpos, ypos, dirty);
		}
	}

	for(y=-12; y<dpy_height+15; y++) {

		const int ypos = y*IMG_SIZE/4+16 + welt->gib_y_off();

		for(int x=-dpy_width + (y & 1); x<=dpy_width+2; x+=2) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*IMG_SIZE/2 + const_x_off;

			display_dinge(i, j, xpos, ypos, dirty);
		}
	}
}
