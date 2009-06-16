/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>

#include "simworld.h"
#include "simview.h"
#include "simgraph.h"

#include "simticker.h"
#include "simdebug.h"
#include "simdings.h"
#include "simconst.h"
#include "simplan.h"
#include "simmenu.h"
#include "player/simplay.h"
#include "besch/grund_besch.h"
#include "boden/wasser.h"
#include "dataobj/umgebung.h"
#include "dings/zeiger.h"


karte_ansicht_t::karte_ansicht_t(karte_t *welt)
{
    this->welt = welt;
}

static const sint8 hours2night[] =
{
    4,4,4,4,4,4,4,4,
    4,4,4,4,3,2,1,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,1,
    2,3,4,4,4,4,4,4
};



void
karte_ansicht_t::display(bool force_dirty)
{
	const sint16 disp_width = display_get_width();
	const sint16 disp_real_height = display_get_height();
	const sint16 menu_height = werkzeug_t::toolbar_tool[0]->iconsize.y;

	const sint16 disp_height = display_get_height() - 16 - (!ticker::empty() ? 16 : 0);
	display_set_clip_wh( 0, menu_height, disp_width, disp_height-menu_height );

	// zuerst den boden zeichnen
	// denn der Boden kann kein Objekt verdecken
	force_dirty = force_dirty || welt->ist_dirty();
	welt->set_dirty_zurueck();

	const sint16 IMG_SIZE = get_tile_raster_width();

	const int dpy_width = disp_width/IMG_SIZE + 2;
	const int dpy_height = (disp_real_height*4)/IMG_SIZE;

	const int i_off = welt->get_world_position().x - disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE;
	const int j_off = welt->get_world_position().y + disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE;
	const int const_x_off = welt->get_x_off();
	const int const_y_off = welt->get_y_off();

	// these are the values needed to go directly from a tile to the display
	welt->set_ansicht_ij_offset(
		koord( - disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE,
					disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE	)
	);

	// change to night mode?
	// images will be recalculated only, when there has been a change, so we set always
	if(grund_t::underground_mode) {
		display_day_night_shift(0);
	}
	else if(!umgebung_t::night_shift) {
		display_day_night_shift(umgebung_t::daynight_level);
	}
	else {
		// calculate also days if desired
		uint32 month = welt->get_last_month();
		const uint32 ticks_this_month = welt->get_zeit_ms() % welt->ticks_per_tag;
		uint32 stunden2;
		if(umgebung_t::show_month>1) {
			static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
			stunden2 = (((sint64)ticks_this_month*tage_per_month[month]) >> (welt->ticks_bits_per_tag-17));
			stunden2 = ((stunden2*3) / 8192) % 48;
		}
		else {
			stunden2 = ( (ticks_this_month * 3) >> (welt->ticks_bits_per_tag-4) )%48;
		}
		display_day_night_shift(hours2night[stunden2]+umgebung_t::daynight_level);
	}

	// not very elegant, but works:
	// fill everything with black for Underground mode ...
	if(grund_t::underground_mode) {
		display_fillbox_wh(0, 32, disp_width, disp_height-menu_height, COL_BLACK, force_dirty);
	}
	// to save calls to grund_t::get_disp_height
	// gr->get_disp_height() == min(gr->get_hoehe(), hmax_ground)
	const sint8 hmax_ground = (grund_t::underground_mode==grund_t::ugm_level) ? grund_t::underground_level : 127;

	// first display ground
	int	y;
	for(y=-dpy_height; y<dpy_height+dpy_width; y++) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(sint16 x=-dpy_width-(y & 1); x<=dpy_width+dpy_height; x+=2) {

			const sint16 i = ((y+x) >> 1) + i_off;
			const sint16 j = ((y-x) >> 1) + j_off;
			const sint16 xpos = x*(IMG_SIZE/2) + const_x_off;

			if(xpos+IMG_SIZE>0  &&  xpos<disp_width) {
				const planquadrat_t *plan=welt->lookup(koord(i,j));
				if(plan  &&  plan->get_kartenboden()) {
					sint16 yypos = ypos - tile_raster_scale_y( min(plan->get_kartenboden()->get_hoehe(), hmax_ground)*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE<disp_height  &&  yypos+IMG_SIZE>menu_height) {
						plan->display_boden(xpos, yypos);
					}
				}
				else {
					// outside ...
					display_img(grund_besch_t::ausserhalb->get_bild(hang_t::flach), xpos,ypos - tile_raster_scale_y( welt->get_grundwasser()*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE ), force_dirty);
				}
			}
		}
	}

	// and then things (and other ground)
	// especially necessary for vehicles
	for(y=-dpy_height; y<dpy_height+dpy_width; y++) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(sint16 x=-dpy_width-(y & 1); x<=dpy_width+dpy_height; x+=2) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*(IMG_SIZE/2) + const_x_off;

			if(xpos+IMG_SIZE>0  &&  xpos<disp_width) {
				const planquadrat_t *plan=welt->lookup(koord(i,j));
				if(plan  &&  plan->get_kartenboden()) {
					const grund_t *gr = plan->get_kartenboden();
					// minimum height: ground height for overground,
					// for the definition of underground_level see grund_t::set_underground_mode
					const sint8 hmin = min(gr->get_hoehe(), grund_t::underground_level);

					// maximum height: 127 for overground, undergroundlevel for sliced, ground height-1 for complete underground view
					const sint8 hmax = grund_t::underground_mode==grund_t::ugm_all ? gr->get_hoehe()-(!gr->ist_tunnel()) : grund_t::underground_level;

					/* long version
					switch(grund_t::underground_mode) {
						case ugm_all:
							hmin = -128;
							hmax = gr->get_hoehe()-(!gr->ist_tunnel());
							underground_level = -128;
							break;
						case ugm_level:
							hmin = min(gr->get_hoehe(), underground_level);
							hmax = underground_level;
							underground_level = level;
							break;
						case ugm_none:
							hmin = gr->get_hoehe();
							hmax = 127;
							underground_level = 127;
					} */
					sint16 yypos = ypos - tile_raster_scale_y( min(gr->get_hoehe(),hmax_ground)*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE*2<disp_height  &&  yypos+IMG_SIZE>menu_height) {
						plan->display_dinge(xpos, yypos, IMG_SIZE, true, hmin, hmax);
					}
				}
			}
		}
	}

	// and finally overlays (station coverage and signs)
	for(y=-dpy_height; y<dpy_height+dpy_width; y++) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(sint16 x=-dpy_width-(y & 1); x<=dpy_width+dpy_height; x+=2) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*(IMG_SIZE/2) + const_x_off;

			if(xpos+IMG_SIZE>0  &&  xpos<disp_width) {
				const planquadrat_t *plan=welt->lookup(koord(i,j));
				if(plan  &&  plan->get_kartenboden()) {
					const grund_t *gr = plan->get_kartenboden();
					// minimum height: ground height for overground,
					// for the definition of underground_level see grund_t::set_underground_mode
					const sint8 hmin = min(gr->get_hoehe(), grund_t::underground_level);

					// maximum height: 127 for overground, undergroundlevel for sliced, ground height-1 for complete underground view
					const sint8 hmax = grund_t::underground_mode==grund_t::ugm_all ? gr->get_hoehe()-(!gr->ist_tunnel()) : grund_t::underground_level;

					sint16 yypos = ypos - tile_raster_scale_y( min(gr->get_hoehe(),hmax_ground)*TILE_HEIGHT_STEP/Z_TILE_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE<disp_height  &&  yypos+IMG_SIZE>menu_height) {
						plan->display_overlay(xpos, yypos, hmin, hmax);
					}
				}
			}
		}
	}
	ding_t *zeiger = welt->get_zeiger();
	if(zeiger) {
		// better not try to twist your brain to follow the retransformation ...
		const sint16 rasterweite=get_tile_raster_width();
		const koord diff = zeiger->get_pos().get_2d()-welt->get_world_position()-welt->get_ansicht_ij_offset();

		const sint16 x = welt->get_x_off() + (diff.x-diff.y)*(rasterweite/2);
		const sint16 y = welt->get_y_off() + (diff.x+diff.y)*(rasterweite/4) + tile_raster_scale_y( -zeiger->get_pos().z*TILE_HEIGHT_STEP/Z_TILE_STEP, rasterweite) + ((display_get_width()/rasterweite)&1)*(rasterweite/4);
		// mark the cursor position for all tools (except lower/raise)
		if(zeiger->get_yoff()==Z_PLAN) {
			grund_t *gr = welt->lookup( zeiger->get_pos() );
			if(gr && gr->is_visible()) {
				const PLAYER_COLOR_VAL transparent = TRANSPARENT25_FLAG|OUTLINE_FLAG| umgebung_t::cursor_overlay_color;
				if(  gr->get_bild()==IMG_LEER  ) {
					if(  gr->hat_wege()  ) {
						display_img_blend( gr->obj_bei(0)->get_bild(), x, y, transparent, 0, true );
					}
					else {
						display_img_blend( grund_besch_t::get_ground_tile(0,gr->get_hoehe()), x, y, transparent, 0, true );
					}
				}
				else {
					display_img_blend( gr->get_bild(), x, y, transparent, 0, true );
				}
			}
		}
		zeiger->display( x + tile_raster_scale_x( zeiger->get_xoff(), rasterweite), y + tile_raster_scale_y( zeiger->get_yoff(), rasterweite), true );
		zeiger->clear_flag(ding_t::dirty);
	}

	if(welt) {
		// finally update the ticker
		for(int x=0; x<MAX_PLAYER_COUNT; x++) {
			if(  welt->get_spieler(x)  ) {
				welt->get_spieler(x)->display_messages();
			}
		}
	}

	if(force_dirty) {
		mark_rect_dirty_wc( 0, 0, display_get_width(), display_get_height() );
	}
}
