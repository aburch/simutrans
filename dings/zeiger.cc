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
#include "../boden/grund.h"
#include "zeiger.h"

zeiger_t::zeiger_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
  richtung = ribi_t::alle;

  rdwr(file);
  step_frequency = 0;
}


zeiger_t::zeiger_t(karte_t *welt, koord3d pos, spieler_t *sp) :
    ding_t(welt, pos)
{
  setze_besitzer( sp );
  setze_bild(0, 30);
  step_frequency = 0;
  richtung = ribi_t::alle;
}


zeiger_t::~zeiger_t()
{
}


void
zeiger_t::setze_pos(koord3d k)
{
//    printf("%d %d %d -> %d %d %d\n", gib_pos().x, gib_pos().y, gib_pos().z, k.x, k.y, k.z);

    if(k != gib_pos()) {
	if(welt->lookup(gib_pos().gib_2d()) &&
	   welt->lookup(gib_pos().gib_2d())->gib_kartenboden()) {
	   	welt->lookup(gib_pos().gib_2d())->gib_kartenboden()->set_flag(grund_t::world_spot_dirty);
	}

        ding_t::setze_pos(k);
    }
}


void
zeiger_t::setze_richtung(ribi_t::ribi r)
{
    if(richtung != r) {
	richtung = r;

	if(gib_yoff() == welt->Z_LINES) {
	    if(richtung == ribi_t::nord) {
		setze_xoff( 16 );
	    } else {
		setze_xoff( -16 );
	    }
	} else {
	    setze_xoff( 0 );
	}

	if(welt->lookup(gib_pos())) {
	    welt->lookup(gib_pos())->set_flag(grund_t::world_spot_dirty);
	}
    }
};
