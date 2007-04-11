/*
 * zeiger.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simhalt.h"
#include "../boden/grund.h"
#include "../dataobj/umgebung.h"
#include "zeiger.h"

zeiger_t::zeiger_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	richtung = ribi_t::alle;
	bild = IMG_LEER;
	rdwr(file);
}


zeiger_t::zeiger_t(karte_t *welt, koord3d pos, spieler_t *sp) :
    ding_t(welt, pos)
{
	setze_besitzer( sp );
	richtung = ribi_t::alle;
	area = 0;
}


void
zeiger_t::change_pos(koord3d k)
{
	if(k!=gib_pos()) {
		// remove from old position
		// and clear mark
		grund_t *gr=welt->lookup_kartenboden(gib_pos().gib_2d());
		if(gr) {
			if(gr->gib_halt().is_bound()) {
				gr->gib_halt()->mark_unmark_coverage( false );
			}
			welt->mark_area( gib_pos().x, gib_pos().y, area, false );
		}
		mark_image_dirty( gib_bild(), 0 );
		set_flag(ding_t::dirty);

		ding_t::setze_pos(k);
		if(gib_yoff()==welt->Z_PLAN) {
			gr=welt->lookup_kartenboden(k.gib_2d());
			if(gr) {
				if(gr->gib_halt().is_bound()  &&  umgebung_t::station_coverage_show) {
					gr->gib_halt()->mark_unmark_coverage( true );
				}
				// only mark this, if it is not in underground mode
				// or in underground mode, if it is deep enough
				if(!grund_t::underground_mode  ||  gib_pos().z<gr->gib_hoehe()) {
					welt->mark_area( gib_pos().x, gib_pos().y, area, true );
				}
			}
		}
	}
}


void
zeiger_t::setze_richtung(ribi_t::ribi r)
{
	if(richtung != r) {
		richtung = r;
	}
}



/* change the marked area around the cursor */
void
zeiger_t::setze_area(uint8 new_area)
{
	if(new_area==area) {
		return;
	}
	if(new_area < area) {
		welt->mark_area( gib_pos().x, gib_pos().y, area, false );
	}
	area = new_area;
	welt->mark_area( gib_pos().x, gib_pos().y, area, true );
}
