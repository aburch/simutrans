/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/** @file zeiger.cc object to mark tiles */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simhalt.h"
#include "../boden/grund.h"
#include "../dataobj/umgebung.h"
#include "zeiger.h"

zeiger_t::zeiger_t(karte_t *welt, loadsave_t *file) : ding_no_info_t(welt)
{
	bild = IMG_LEER;
	after_bild = IMG_LEER;
	area = koord(0,0);
	center = false;
	rdwr(file);
}


zeiger_t::zeiger_t(karte_t *welt, koord3d pos, spieler_t *sp) :
    ding_no_info_t(welt, pos)
{
	set_besitzer( sp );
	bild = IMG_LEER;
	after_bild = IMG_LEER;
	area = koord(0,0);
	center = false;
}


/**
 * We want to be able to highlight the current tile.
 * Unmarks area around old and marks area around new position.
 * Use this routine to change position.
 * @author Hj. Malthaner
 */
void zeiger_t::change_pos(koord3d k )
{
	if(  k != get_pos()  ) {
		// remove from old position
		// and clear mark
		grund_t *gr = welt->lookup( get_pos() );
		if(gr==NULL) {
			gr = welt->lookup_kartenboden( get_pos().get_2d() );
		}
		if(gr) {
			if(  gr->get_halt().is_bound()  ) {
				gr->get_halt()->mark_unmark_coverage( false );
			}
			welt->mark_area( get_pos()-(area*center)/2, area, false );
		}
		if(  get_pos().x >= welt->get_size().x-1  ||  get_pos().y >= welt->get_size().y-1  ) {
			// the raise and lower tool actually can go to size!
			welt->set_background_dirty();
			// this removes crap form large cursors overlapping into the nirvana
		}
		mark_image_dirty( get_bild(), get_yoff() );
		mark_image_dirty( get_after_bild(), get_yoff() );
		set_flag( ding_t::dirty );

		ding_t::set_pos( k );
		if(  get_yoff() == Z_PLAN  ) {
			gr = welt->lookup( k );
			if(  gr == NULL  ) {
				gr = welt->lookup_kartenboden( k.get_2d() );
			}
			if(gr) {
				if(  gr->get_halt().is_bound()  &&  umgebung_t::station_coverage_show  ) {
					gr->get_halt()->mark_unmark_coverage( true );
				}
				welt->mark_area( k-(area*center)/2, area, true );
			}
		}
	}
}


void zeiger_t::set_bild( image_id b )
{
	// mark dirty
	mark_image_dirty( bild, get_yoff() );
	mark_image_dirty( b, get_yoff() );
	bild = b;
}


void zeiger_t::set_after_bild( image_id b )
{
	// mark dirty
	mark_image_dirty( after_bild, get_yoff() );
	mark_image_dirty( b, get_yoff() );
	after_bild = b;
}


/**
 * Set area to be marked around cursor
 * @param area size of marked area
 * @param center true if cursor is centered within marked area
 */
void zeiger_t::set_area(koord new_area, bool new_center)
{
	if(new_area==area  &&  new_center==center) {
		return;
	}
	welt->mark_area( get_pos()-(area*center)/2, area, false );
	area = new_area;
	center = new_center;
	welt->mark_area( get_pos()-(area*center)/2, area, true );
}
