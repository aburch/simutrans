/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simtools.h"
#include "../besch/fabrik_besch.h"
#include "wolke.h"

#include "../dataobj/loadsave.h"
#include "../tpl/stringhashtable_tpl.h"



wolke_t::wolke_t(karte_t *welt, koord3d pos, sint8 x_off, sint8 y_off, image_id bild, bool increment) :
    ding_t(welt, pos)
{
	base_y_off = (y_off*TILE_STEPS-(TILE_STEPS/2))/16;
	setze_xoff( (x_off*TILE_STEPS)/16 );
	setze_yoff( base_y_off );
	insta_zeit = 0;
	this->increment = increment;
	base_image = bild;
}



wolke_t::~wolke_t()
{
	mark_image_dirty( gib_bild(), 0 );
}



wolke_t::wolke_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	rdwr(file);
}



void
wolke_t::rdwr(loadsave_t *file)
{
	// not saving cloads!
	assert(file->is_loading());

	ding_t::rdwr( file );

	file->rdwr_long(insta_zeit, "\n");
	if(file->get_version()<88005) {
		insta_zeit = 0;
	}
	uint16 dummy=base_y_off;
	file->rdwr_short(dummy, "\n");
	file->rdwr_short(dummy, "\n");
	base_image = IMG_LEER;

	// do not remove from this position, since there will be nothing
	ding_t::set_flag(ding_t::not_on_map);
}



bool
wolke_t::sync_step(long delta_t)
{
	insta_zeit += delta_t;
	if(insta_zeit<2500) {
		if(base_y_off - ((insta_zeit*TILE_STEPS) >> 12)!=gib_yoff()) {
			// move/change cloud ...
			if(!get_flag(ding_t::dirty)) {
				mark_image_dirty(gib_bild(),0);
			}
			setze_yoff(base_y_off - ((insta_zeit*TILE_STEPS) >> 12));
			set_flag(ding_t::dirty);
		}
		return true;
	}
	// delete wolke ...
	return false;
}



/**
 * Wird aufgerufen, wenn wolke entfernt wird. Entfernt wolke aus
 * der Verwaltung synchroner Objekte.
 * @author Hj. Malthaner
 */
void wolke_t::entferne(spieler_t *)
{
	welt->sync_remove(this);
}




/***************************** just for compatibility, the old raucher and smoke clouds *********************************/

raucher_t::raucher_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	assert(file->is_loading());
	ding_t::rdwr( file );
	const char *s = NULL;
	file->rdwr_str(s, "N");

	// do not remove from this position, since there will be nothing
	ding_t::set_flag(ding_t::not_on_map);
}


async_wolke_t::async_wolke_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	// not saving clouds!
	assert(file->is_loading());

	ding_t::rdwr( file );

	uint32 dummy;
	file->rdwr_long(dummy, "\n");

	// do not remove from this position, since there will be nothing
	ding_t::set_flag(ding_t::not_on_map);
}
