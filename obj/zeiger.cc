/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/** @file zeiger.cc object to mark tiles */

#include <stdio.h>

#include "../simworld.h"
#include "../simobj.h"
#include "../simhalt.h"
#include "../boden/grund.h"
#include "../dataobj/environment.h"
#include "zeiger.h"

zeiger_t::zeiger_t(loadsave_t *file) : obj_no_info_t()
{
	bild = IMG_LEER;
	after_bild = IMG_LEER;
	area = koord(0,0);
	center = false;
	rdwr(file);
}


zeiger_t::zeiger_t(koord3d pos, spieler_t *sp) :
    obj_no_info_t(pos)
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
		mark_image_dirty( get_bild(), 0 );
		mark_image_dirty( get_after_bild(), 0 );
		set_flag( obj_t::dirty );

		obj_t::set_pos( k );
		if(  get_yoff() == Z_PLAN  ) {
			gr = welt->lookup( k );
			if(  gr == NULL  ) {
				gr = welt->lookup_kartenboden( k.get_2d() );
			}
			if(gr) {
				if(  gr->get_halt().is_bound()  &&  env_t::station_coverage_show  ) {
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
	mark_image_dirty( bild, 0 );
	mark_image_dirty( b, 0 );
	bild = b;
}


void zeiger_t::set_after_bild( image_id b )
{
	// mark dirty
	mark_image_dirty( after_bild, 0 );
	mark_image_dirty( b, 0 );
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
