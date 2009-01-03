/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
	changed = false;
	richtung = ribi_t::alle;
	bild = IMG_LEER;
	area = koord(0,0);
	center = 0;
	rdwr(file);
}


zeiger_t::zeiger_t(karte_t *welt, koord3d pos, spieler_t *sp) :
    ding_t(welt, pos)
{
	changed = false;
	setze_besitzer( sp );
	richtung = ribi_t::alle;
	bild = IMG_LEER;
	area = koord(0,0);
	center = 0;
}


void
zeiger_t::change_pos(koord3d k )
{
	if(k!=gib_pos()) {
		// remove from old position
		// and clear mark
		grund_t *gr = welt->lookup(gib_pos());
		if(gr==NULL) {
			gr = welt->lookup_kartenboden(gib_pos().gib_2d());
		}
		if(gr) {
			if(gr->gib_halt().is_bound()) {
				gr->gib_halt()->mark_unmark_coverage( false );
			}
			welt->mark_area( gib_pos()-(area*center)/2, area, false );
		}
		mark_image_dirty( gib_bild(), 0 );
		set_flag(ding_t::dirty);

		ding_t::setze_pos(k);
		if(gib_yoff()==Z_PLAN) {
			gr = welt->lookup(k);
			if(gr==NULL) {
				gr = welt->lookup_kartenboden(k.gib_2d());
			}
			if(gr) {
				if(gr->gib_halt().is_bound()  &&  umgebung_t::station_coverage_show) {
					gr->gib_halt()->mark_unmark_coverage( true );
				}
				// only mark this, if it is not in underground mode
				// or in underground mode, if it is deep enough
//				if(!grund_t::underground_mode  ||  gib_pos().z<gr->gib_hoehe()) {
					welt->mark_area( k-(area*center)/2, area, true );
				//}
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


void
zeiger_t::setze_bild( image_id b )
{
	// mark dirty
	mark_image_dirty( bild, 0 );
	mark_image_dirty( b, 0 );
	bild = b;
	if(  (area.x|area.y)>1  ) {
		welt->mark_area( gib_pos()-(area*center)/2, area, false );
	}
	if(!changed) {
		area = koord(0,0);
		center = 0;
	}
}


/* change the marked area around the cursor */
void
zeiger_t::setze_area(koord new_area, uint8 new_center)
{
	changed = true;
	if(new_area==area  &&  (new_center^center)) {
		return;
	}
	welt->mark_area( gib_pos()-(area*center)/2, area, false );
	area = new_area;
	center = new_center;
	welt->mark_area( gib_pos()-(area*center)/2, area, true );
}


// returns true ONCE, if the areas was change before this call
bool zeiger_t::area_changed()
{
	bool ch = changed;
	changed = false;
	return ch;
}
