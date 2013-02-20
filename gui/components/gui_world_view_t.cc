/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "gui_world_view_t.h"
#include "../../simworld.h"
#include "../../simdings.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../vehicle/simvehikel.h"
#include "../../boden/grund.h"
#include "../../simdings.h"

#include "../../dataobj/umgebung.h"
#include "../../dataobj/koord3d.h"


karte_t *world_view_t::welt = NULL;


world_view_t::world_view_t(karte_t* w, koord size ) :
		raster(get_base_tile_raster_width())
{
	welt = w;
	set_groesse( size );
}


world_view_t::world_view_t(karte_t* w) :
		raster(get_base_tile_raster_width())
{
	welt = w;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool world_view_t::infowin_event(const event_t* ev)
{
	if(IS_LEFTRELEASE(ev)) {
		koord3d const& pos = get_location();
		if (welt->is_within_limits(pos.get_2d())) {
			welt->change_world_position(pos);
		}
	}
	return true;
}


void world_view_t::internal_draw(const koord offset, ding_t const* const ding)
{
	display_set_image_proc(false);

	const koord3d here3d    = get_location();
	const koord   here      = here3d.get_2d();
	koord         fine_here = koord(0, 0);
	sint16        y_offset  = 0;
	if(ding) { // offsets?
		fine_here = koord(tile_raster_scale_x(-ding->get_xoff(), raster), tile_raster_scale_y(-ding->get_yoff() % (OBJECT_OFFSET_STEPS * 2), raster));
		y_offset  = ding->get_yoff() / (OBJECT_OFFSET_STEPS * 2);
		if(vehikel_basis_t const* const v = ding_cast<vehikel_basis_t>(ding)) {
			int x = 0;
			int y = 0;
			v->get_screen_offset(x, y, raster);
			fine_here -= koord(x, y);
		}
	}

	grund_t const* const kb = welt->lookup_kartenboden(here);
	if(!kb) {
		return;
	}

	int hgt = tile_raster_scale_y(here3d.z * TILE_HEIGHT_STEP, raster);
	if(ding) {
		aircraft_t const* const plane = ding_cast<aircraft_t>(ding);
		if(plane) {
			hgt += tile_raster_scale_y(plane->get_flyingheight(), raster);
		}
	}

	const koord pos = get_pos() + offset + koord(1, 1);

	// do not draw outside (may happen with scroll bars)
	const clip_dimension old_clip = display_get_clip_wh();

	// something to draw?
	const koord gr = get_groesse() - koord(2, 2);
	if(  old_clip.xx <= pos.x  ||  pos.x + gr.x <= old_clip.x  ||  gr.x <= 0  ) {
		return;
	}
	if(  old_clip.yy <= pos.y  ||  pos.y + gr.y <= old_clip.y  ||  gr.y <= 0  ) {
		return;
	}

	const int clip_x = max(old_clip.x, pos.x);
	const int clip_y = max(old_clip.y, pos.y);
	display_set_clip_wh(clip_x, clip_y, min(old_clip.xx, pos.x + gr.x) - clip_x, min(old_clip.yy, pos.y + gr.y) - clip_y);

	mark_rect_dirty_wc(pos.x, pos.y, pos.x + gr.x, pos.y + gr.y);

	/* Not very elegant, but works: Fill everything with black for underground
	 * mode. */
	if(  grund_t::underground_mode  ) {
		display_fillbox_wh(pos.x, pos.y, gr.x, gr.y, COL_BLACK, true);
	}

	const sint16 yoff = ding && ding->is_moving() ?
		gr.y / 2 - raster * 3 / 4 : // align 1/4 raster from the bottom of the image
		gr.y     - raster;          // align the bottom of the image
	const koord display_off = koord((gr.x - raster) / 2, hgt + yoff) + fine_here;

	// display grounds
	FOR(vector_tpl<koord>, const& off, offsets) {
		const koord   k     = here + off + koord(y_offset, y_offset);
		const sint16  off_x = (off.x - off.y) * 32 * raster / 64 + display_off.x;

		if(  off_x + raster < 0  ||  gr.x < off_x  ||  k.x < 0  ||  k.y < 0  ) {
			continue;
		}

		const grund_t * const kb = welt->lookup_kartenboden(k);
		if(  !kb  ) {
			continue;
		}

		const sint16 yypos = display_off.y + (off.y + off.x) * 16 * raster / 64 - tile_raster_scale_y(kb->get_hoehe() * TILE_HEIGHT_STEP, raster);
		if(  gr.y < yypos  ) {
			break; // enough with grounds
		} else if (  0 <= yypos + raster  ) {
			kb->display_if_visible(pos.x + off_x, pos.y + yypos, raster);
		}
	}

	// display things
	FOR(vector_tpl<koord>, const& off, offsets) {
		const koord   k     = here + off + koord(y_offset, y_offset);
		const sint16  off_x = (off.x - off.y) * 32 * raster / 64 + display_off.x;
		if(  off_x + raster < 0  ||  gr.x < off_x  ||  k.x < 0  ||  k.y < 0  ) {
			continue;
		}

		const planquadrat_t * const plan = welt->lookup(k);
		if(!plan) {
			continue;
		}
		const grund_t * const kb = plan->get_kartenboden();
		if(!kb) {
			continue;
		}

		const sint8 h = kb->get_hoehe();
		// minimum height: ground height for overground,
		// for the definition of underground_level see grund_t::set_underground_mode
		const sint8 hmin = min(h, grund_t::underground_level);

		// maximum height: 127 for overground, undergroundlevel for sliced, ground height-1 for complete underground view
		const sint8 hmax = grund_t::underground_mode == grund_t::ugm_all ? h - !kb->ist_tunnel() : grund_t::underground_level;

		const sint16 yypos = display_off.y + (off.y + off.x) * 16 * raster / 64 - tile_raster_scale_y(h * TILE_HEIGHT_STEP, raster);
		if(  0 <= yypos + raster  &&  yypos - raster * 2 < gr.y  ) {
			plan->display_dinge(pos.x + off_x, pos.y + yypos, raster, false, hmin, hmax);
		} else if(  yypos > gr.y  ) {
			break; // now we can finish
		}
	}

	// this should only happen for airplanes: out of image, so we need to extra display them
	if(  y_offset != 0  ) {
		const grund_t * const g     = welt->lookup(ding->get_pos());
		const sint16          yypos = display_off.y - tile_raster_scale_y(2 * y_offset * 16, raster) - tile_raster_scale_y(g->get_hoehe() * TILE_HEIGHT_STEP, raster);
		g->display_dinge_all(pos.x + display_off.x, pos.y + yypos, raster, false);
	}

	display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	display_ddd_box_clip(pos.x - 1, pos.y - 1, gr.x + 2, gr.y + 2, MN_GREY0, MN_GREY4);
}


/**
 * Resize the contents of the window
 * @author prissi
 */
void world_view_t::set_groesse(koord size)
{
	gui_komponente_t::set_groesse(size);
	calc_offsets(size, 5);
}


/**
 * Recalculates the number of tiles needed
 */
void world_view_t::calc_offsets(koord size, sint16 dy_off)
{
	const sint16 max_dx = size.x/(raster/2) + 2;
	const sint16 max_dy = (size.y/(raster/2) + dy_off)&0x0FFE;

	offsets.clear();
	for(  sint16 dy = -max_dy;  dy <= 5;  ) {
		{
			for(  sint16 dx =- 2;  dx < max_dx;  dx += 2  ) {
				const koord check( (dy + dx) / 2, (dy - dx) / 2);
				offsets.append(check);
			}
		}
		dy++;
		for(  sint16 dx = -1;  dx < max_dx;  dx += 2  ) {
			const koord check( (dy + dx) / 2, (dy - dx) / 2);
			offsets.append(check);
		}
		dy++;
	}
}
