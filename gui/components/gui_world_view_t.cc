/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "gui_world_view_t.h"
#include "../../simworld.h"
#include "../../display/simview.h"
#include "../../display/viewport.h"
#include "../../simobj.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../../vehicle/simvehikel.h"
#include "../../boden/grund.h"

#include "../../dataobj/environment.h"
#include "../../dataobj/koord3d.h"



world_view_t::world_view_t(scr_size size ) :
		raster(get_base_tile_raster_width())
{
	set_size( size );
}


world_view_t::world_view_t() :
		raster(get_base_tile_raster_width())
{
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
			welt->get_viewport()->change_world_position(pos);
		}
	}
	return true;
}


void world_view_t::internal_draw(const scr_coord offset, obj_t const* const obj)
{
	display_set_image_proc(false);

	const koord3d here3d    = get_location();
	const koord   here      = here3d.get_2d();
	scr_coord     fine_here = scr_coord(0, 0);
	sint16        y_offset  = 0;
	if(obj) { // offsets?
		fine_here = scr_coord(tile_raster_scale_x(-obj->get_xoff(), raster), tile_raster_scale_y(-obj->get_yoff() % (OBJECT_OFFSET_STEPS * 2), raster));
		y_offset  = obj->get_yoff() / (OBJECT_OFFSET_STEPS * 2);
		if(vehikel_basis_t const* const v = obj_cast<vehikel_basis_t>(obj)) {
			int x = 0;
			int y = 0;
			v->get_screen_offset(x, y, raster);
			fine_here -= scr_coord(x, y);
		}
	}

	grund_t const* const kb = welt->lookup_kartenboden(here);
	if(!kb) {
		return;
	}

	int hgt = tile_raster_scale_y(here3d.z * TILE_HEIGHT_STEP, raster);
	if(obj) {
		aircraft_t const* const plane = obj_cast<aircraft_t>(obj);
		if(plane) {
			hgt += tile_raster_scale_y(plane->get_flyingheight(), raster);
		}
	}

	const scr_coord pos = get_pos() + offset + scr_coord(1, 1);

	// do not draw outside (may happen with scroll bars)
	const clip_dimension old_clip = display_get_clip_wh();

	// something to draw?
	const scr_size size = get_size() - scr_size(2, 2);
	if(  old_clip.xx <= pos.x  ||  pos.x + size.w <= old_clip.x  ||  size.w <= 0  ) {
		return;
	}
	if(  old_clip.yy <= pos.y  ||  pos.y + size.h <= old_clip.y  ||  size.h <= 0  ) {
		return;
	}

	const int clip_x = max(old_clip.x, pos.x);
	const int clip_y = max(old_clip.y, pos.y);
	display_set_clip_wh(clip_x, clip_y, min(old_clip.xx, pos.x + size.w) - clip_x, min(old_clip.yy, pos.y + size.h) - clip_y);

	mark_rect_dirty_wc(pos.x, pos.y, pos.x + size.w, pos.y + size.h);

	/* Not very elegant, but works: Fill everything with black for underground
	 * mode. */
	if(  grund_t::underground_mode  ) {
		display_fillbox_wh(pos.x, pos.y, size.w, size.h, COL_BLACK, true);
	}
	else {
		welt->get_view()->display_background(pos.x, pos.y, size.w, size.h, true);
	}

	const sint16 yoff = obj && obj->is_moving() ?
		size.h / 2 - raster * 3 / 4 : // align 1/4 raster from the bottom of the image
		size.h     - raster;          // align the bottom of the image
	const scr_coord display_off = scr_coord((size.w - raster) / 2, hgt + yoff) + fine_here;

	// display grounds
	FOR(vector_tpl<koord>, const& off, offsets) {
		const koord   k     = here + off + koord(y_offset, y_offset);
		const sint16  off_x = (off.x - off.y) * 32 * raster / 64 + display_off.x;

		if(  off_x + raster < 0  ||  size.w < off_x  ||  k.x < 0  ||  k.y < 0  ) {
			continue;
		}

		grund_t *kb = welt->lookup_kartenboden(k);
		if(  !kb  ) {
			continue;
		}

		const sint16 yypos = display_off.y + (off.y + off.x) * 16 * raster / 64 - tile_raster_scale_y(kb->get_disp_height() * TILE_HEIGHT_STEP, raster);
		if(  size.h < yypos  ) {
			break; // enough with grounds
		}
		else if(  0 <= yypos + raster  ) {
#ifdef MULTI_THREAD
			kb->display_if_visible( pos.x + off_x, pos.y + yypos, raster, 0 );
#else
			kb->display_if_visible( pos.x + off_x, pos.y + yypos, raster );
#endif
		}
	}

	// display things
	FOR(vector_tpl<koord>, const& off, offsets) {
		const koord   k     = here + off + koord(y_offset, y_offset);
		const sint16  off_x = (off.x - off.y) * 32 * raster / 64 + display_off.x;
		if(  off_x + raster < 0  ||  size.w < off_x  ||  k.x < 0  ||  k.y < 0  ) {
			continue;
		}

		const planquadrat_t * const plan = welt->access(k);
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

		// maximum height: 127 for overground, underground level for sliced, ground height-1 for complete underground view
		const sint8 hmax = grund_t::underground_mode == grund_t::ugm_all ? h - !kb->ist_tunnel() : grund_t::underground_level;

		const sint16 yypos = display_off.y + (off.y + off.x) * 16 * raster / 64 - tile_raster_scale_y(kb->get_disp_height() * TILE_HEIGHT_STEP, raster);
		if(  0 <= yypos + raster  &&  yypos - raster * 2 < size.h  ) {
#ifdef MULTI_THREAD
			plan->display_obj( pos.x + off_x, pos.y + yypos, raster, false, hmin, hmax, 0 );
#else
			plan->display_obj( pos.x + off_x, pos.y + yypos, raster, false, hmin, hmax );
#endif
		}
		else if(  yypos > size.h  ) {
			break; // now we can finish
		}
	}

	// this should only happen for airplanes: out of image, so we need to extra display them
	if(  y_offset != 0  ) {
		const grund_t * const g     = welt->lookup(obj->get_pos());
		const sint16          yypos = display_off.y - tile_raster_scale_y(2 * y_offset * 16, raster) - tile_raster_scale_y(g->get_disp_height() * TILE_HEIGHT_STEP, raster);
#ifdef MULTI_THREAD
		g->display_obj_all( pos.x + display_off.x, pos.y + yypos, raster, false, 0 );
#else
		g->display_obj_all( pos.x + display_off.x, pos.y + yypos, raster, false );
#endif
	}

	display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	display_ddd_box_clip(pos.x - 1, pos.y - 1, size.w + 2, size.h + 2, MN_GREY0, MN_GREY4);
}


/**
 * Resize the contents of the window
 * @author prissi
 */
void world_view_t::set_size(scr_size size)
{
	gui_komponente_t::set_size(size);
	calc_offsets(size, 5);
}


/**
 * Recalculates the number of tiles needed
 */
void world_view_t::calc_offsets(scr_size size, sint16 dy_off)
{
	const sint16 max_dx = size.w/(raster/2) + 2;
	const sint16 max_dy = (size.h/(raster/2) + dy_off)&0x0FFE;

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
