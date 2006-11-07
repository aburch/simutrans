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


void
zeiger_t::setze_pos(koord3d k)
{
	if(k!=gib_pos()) {
		mark_image_dirty( gib_bild(), 0 );
		set_flag(ding_t::dirty);

		if(welt->lookup(gib_pos())) {
			welt->lookup(gib_pos())->clear_flag(grund_t::marked);
			welt->lookup(gib_pos())->set_flag(grund_t::dirty);
		}
		ding_t::setze_pos(k);
		if(gib_yoff()==welt->Z_PLAN  &&  welt->lookup(k)) {
			welt->lookup(k)->set_flag(grund_t::marked);
			welt->lookup(k)->set_flag(grund_t::dirty);
		}
	}
}


void
zeiger_t::setze_richtung(ribi_t::ribi r)
{
	if(richtung != r) {
		richtung = r;
	}
};
