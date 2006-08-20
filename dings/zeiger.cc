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
	if(k!=gib_pos()) {
		// mark the region after the image as dirty
		// better not try to twist your brain to follow the retransformation ...
		const koord diff=gib_pos().gib_2d()-welt->gib_ij_off();
		const sint16 rasterweite=get_tile_raster_width();
		const sint16 x=(diff.x-diff.y)*(rasterweite/2) + welt->gib_x_off() + (display_get_width()/2) + tile_raster_scale_x(gib_xoff(), rasterweite);
		const sint16 y=16+((display_get_width()/rasterweite)&1)*(rasterweite/4)+(diff.x+diff.y)*(rasterweite/4)+welt->gib_y_off()+tile_raster_scale_x(gib_yoff(), rasterweite)-tile_raster_scale_y(gib_pos().z, rasterweite);
		display_mark_img_dirty( gib_bild(), x, y );
		set_flag(ding_t::dirty);

		if(welt->lookup(gib_pos().gib_2d())  &&  welt->lookup(gib_pos().gib_2d())->gib_kartenboden()) {
			welt->lookup(gib_pos().gib_2d())->gib_kartenboden()->clear_flag(grund_t::marked);
			welt->lookup(gib_pos().gib_2d())->gib_kartenboden()->set_flag(grund_t::dirty);
		}
		ding_t::setze_pos(k);
		if(gib_yoff()==welt->Z_PLAN  &&  welt->lookup(k.gib_2d())  &&  welt->lookup(k.gib_2d())->gib_kartenboden()) {
			welt->lookup(k.gib_2d())->gib_kartenboden()->set_flag(grund_t::marked);
			welt->lookup(gib_pos().gib_2d())->gib_kartenboden()->set_flag(grund_t::dirty);
		}
	}
}


void
zeiger_t::setze_richtung(ribi_t::ribi r)
{
	if(richtung != r) {
/*
		// mark the region after the image as dirty
		// better not try to twist your brain to follow the retransformation ...
		const koord diff=gib_pos().gib_2d()-welt->gib_ij_off();
		const sint16 rasterweite=get_tile_raster_width();
		const sint16 x=(diff.x-diff.y)*(rasterweite/2) + welt->gib_x_off() + (display_get_width()/2) + tile_raster_scale_x(gib_xoff(), rasterweite);
		const sint16 y=16+((display_get_width()/rasterweite)&1)*(rasterweite/4)+(diff.x+diff.y)*(rasterweite/4)+welt->gib_y_off()+tile_raster_scale_x(gib_yoff(), rasterweite)-tile_raster_scale_y(gib_pos().z, rasterweite);
		display_mark_img_dirty( gib_bild(), x, y );
		set_flag(ding_t::dirty);
*/
		richtung = r;

		if(gib_yoff()==welt->Z_LINES) {
			if(richtung == ribi_t::nord) {
				setze_xoff( 16 );
			}
			else {
				setze_xoff( -16 );
			}
		}
		else {
			setze_xoff( 0 );
		}
	}
};
