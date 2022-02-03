/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "gui_world_view.h"
#include "../../world/simworld.h"
#include "../../display/viewport.h"
#include "../../obj/simobj.h"
#include "../../obj/zeiger.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../../ground/grund.h"

#include "../../dataobj/environment.h"
#include "../../dataobj/koord3d.h"

#include "../../vehicle/air_vehicle.h"


vector_tpl<world_view_t *> world_view_t::view_list;


void world_view_t::invalidate_all()
{
	world_view_t *const *const endpointer = world_view_t::view_list.end();
	for (world_view_t *const *pointer = world_view_t::view_list.begin() ; pointer != endpointer ; pointer++) {
		(*pointer)->prepared_rect.discard_area();
	}
}

world_view_t::world_view_t(scr_size size ) :
	prepared_rect(),
	display_rect(),
	raster(get_base_tile_raster_width())
{
	set_size( size );
	min_size = size;

	world_view_t::view_list.append(this);
}


world_view_t::world_view_t() :
	prepared_rect(),
	display_rect(),
	raster(get_base_tile_raster_width())
{
	world_view_t::view_list.append(this);
}

world_view_t::~world_view_t()
{
	world_view_t::view_list.remove(this);
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 */
bool world_view_t::infowin_event(const event_t* ev)
{
	if(IS_LEFTRELEASE(ev)) {
		koord3d const& pos = get_location();
		if (welt->is_within_limits(pos.get_2d())) {
			welt->get_viewport()->change_world_position(pos);
			welt->get_zeiger()->change_pos( pos );
		}
		return true;
	}
	return false;
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
		if(vehicle_base_t const* const v = obj_cast<vehicle_base_t>(obj)) {
			int x = 0;
			int y = 0;
			v->get_screen_offset(x, y, raster);
			fine_here -= scr_coord(x, y);
		}
	}
	koord const height_offset(y_offset, y_offset);

	grund_t const* const kb = welt->lookup_kartenboden(here);
	if(!kb) {
		return;
	}

	int hgt = tile_raster_scale_y(here3d.z * TILE_HEIGHT_STEP, raster);
	if(obj) {
		air_vehicle_t const* const plane = obj_cast<air_vehicle_t>(obj);
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

	// prepare view
	rect_t const world_rect(koord(0, 0), welt->get_size());

	rect_t view_rect = display_rect;
	view_rect.origin+= here + height_offset;
	view_rect.mask(world_rect);

	if (view_rect != prepared_rect) {
		welt->prepare_tiles(view_rect, prepared_rect);
		prepared_rect = view_rect;
	}

	// prepare clip area
	const int clip_x = max(old_clip.x, pos.x);
	const int clip_y = max(old_clip.y, pos.y);
	display_set_clip_wh(clip_x, clip_y, min(old_clip.xx, pos.x + size.w) - clip_x, min(old_clip.yy, pos.y + size.h) - clip_y);

	mark_rect_dirty_wc(pos.x, pos.y, pos.x + size.w, pos.y + size.h);

	/* Not very elegant, but works: Fill everything with black for underground
	 * mode. */
	if(  grund_t::underground_mode  ) {
		display_fillbox_wh_clip_rgb(pos.x, pos.y, size.w, size.h, color_idx_to_rgb(COL_BLACK), true);
	}
	else {
		display_fillbox_wh_clip_rgb(pos.x, pos.y, size.w, size.h, env_t::background_color, true);
	}

	const sint16 yoff = obj && obj->is_moving() ?
		size.h / 2 - raster * 3 / 4 : // align 1/4 raster from the bottom of the image
		size.h     - raster;          // align the bottom of the image
	const scr_coord display_off = scr_coord((size.w - raster) / 2, hgt + yoff) + fine_here;

	// display grounds
	FOR(vector_tpl<koord>, const& off, offsets) {
		const koord   k     = here + off + height_offset;
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
		const koord   k     = here + off + height_offset;
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
	display_ddd_box_clip_rgb(pos.x - 1, pos.y - 1, size.w + 2, size.h + 2, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));
}


/**
 * Resize the contents of the window
 */
void world_view_t::set_size(scr_size size)
{
	gui_component_t::set_size(size);
	calc_offsets(size, 5);
}


/**
 * Recalculates the number of tiles needed
 */
void world_view_t::calc_offsets(scr_size size, sint16 dy_off)
{
	const sint16 max_dx = size.w/(raster/2) + 2;
	const sint16 max_dy = (size.h/(raster/2) + dy_off)&0x0FFE;

	// build offset list
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

	// determine preparation extents
	koord offset_extent_min(0, 0);
	koord offset_extent_max(0, 0);
	koord const *const iter_end = offsets.end();
	for (koord const *iter = offsets.begin() ; iter != iter_end ; iter++) {
		sint16 const x = iter->x;
		sint16 const y = iter->y;
		if (x < offset_extent_min.x) {
			offset_extent_min.x = x;
		}else if (x > offset_extent_max.x) {
			offset_extent_max.x = x;
		}
		if (y < offset_extent_min.y) {
			offset_extent_min.y = y;
		}else if (y > offset_extent_max.y) {
			offset_extent_max.y = y;
		}
	}

	display_rect = rect_t(offset_extent_min, offset_extent_max - offset_extent_min + koord(1, 1));
}
