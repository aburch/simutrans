/*
 * wolke.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "wolke.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"



wolke_t::wolke_t(karte_t *welt) : ding_t (welt)
{
}


wolke_t::wolke_t(karte_t *welt, koord3d pos, int x_off, int y_off, image_id bild) :
    ding_t(welt, pos)
{
    setze_bild( 0, bild );
    setze_yoff( y_off-8 );
    setze_xoff( x_off );
    insta_zeit = 0;
}

void
wolke_t::zeige_info()
{
    // wolken sollten keine Infofenster erzeugen
}

void
wolke_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr( file );

	file->rdwr_long(insta_zeit, "\n");
	if(file->get_version()<88005) {
		insta_zeit = simrand(2500);
	}
}



sync_wolke_t::sync_wolke_t(karte_t *welt, loadsave_t *file) : wolke_t(welt)
{
//    welt->sync_add( this );	//no smoke anymore after loading ...
    rdwr(file);
}


sync_wolke_t::sync_wolke_t(karte_t *welt, koord3d pos, int x_off, int y_off, image_id bild)
 : wolke_t(welt, pos, x_off, y_off, bild)

{
    base_y_off = y_off-8;
    base_image = bild;
}

void
sync_wolke_t::rdwr(loadsave_t *file)
{
    wolke_t::rdwr( file );
    file->rdwr_short(base_y_off, "\n");
    file->rdwr_short(base_image, "\n");
}


bool
sync_wolke_t::sync_step(long delta_t)
{
	insta_zeit += delta_t;
	if(insta_zeit<2500) {
		setze_yoff(base_y_off - (insta_zeit >> 7));
		setze_bild(0, base_image+(insta_zeit >> 9));
		set_flag(ding_t::dirty);
		return true;
	}
	// delete wolke ...
	sint8 offset=(base_y_off-19)/16;;
	koord pos = gib_pos().gib_2d()+koord(offset,offset);
	if(welt->lookup(pos)) {
		welt->lookup(pos)->gib_kartenboden()->set_flag(grund_t::world_spot_dirty);
	}
	return false;
}


/**
 * Wird aufgerufen, wenn wolke entfernt wird. Entfernt wolke aus
 * der Verwaltuing synchroner Objekte.
 * @author Hj. Malthaner
 */
void sync_wolke_t::entferne(spieler_t *)
{
	koord pos = gib_pos().gib_2d()-koord(1,1);
	if(welt->lookup(pos)) {
		grund_t *gr=welt->lookup(pos)->gib_kartenboden();
		if(gr) {
			gr->set_flag(grund_t::world_spot_dirty);
		}
		//welt->lookup(pos+koord(1,0))->gib_kartenboden()->set_flag(grund_t::world_spot_dirty);
		//welt->lookup(pos+koord(0,1))->gib_kartenboden()->set_flag(grund_t::world_spot_dirty);
	}
	welt->sync_remove(this);
}


async_wolke_t::async_wolke_t(karte_t *welt, loadsave_t *file) : wolke_t(welt)
{
	rdwr(file);
	step_frequency = 1;
}


async_wolke_t::async_wolke_t(karte_t *welt, koord3d pos, int x_off, int y_off, image_id bild) :
   wolke_t(welt, pos, x_off, y_off, bild)
{
	step_frequency = 1;
}

bool
async_wolke_t::step(long delta_t)
{
	const int yoff = gib_yoff();
	insta_zeit -= delta_t;
	if(yoff>-120 &&  insta_zeit<15000 ) {
		setze_yoff( yoff - 2 );
		set_flag(ding_t::dirty);
		return true;
	}
	// remove cloud
	return false;
}
