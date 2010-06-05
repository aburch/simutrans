/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "gui_world_view_t.h"
#include "../../simevent.h"
#include "../../simworld.h"
#include "../../simdings.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../vehicle/simvehikel.h"
#include "../../boden/grund.h"
#include "../../simdings.h"

#include "../../dataobj/umgebung.h"
#include "../../dataobj/koord3d.h"



world_view_t::world_view_t(karte_t* welt, koord3d location, koord size)
{
    this->location = location;
    this->ding = NULL;
    this->welt = welt;
    this->raster = get_base_tile_raster_width();

    set_groesse(size);
}


world_view_t::world_view_t(const ding_t* dt, koord size) :
	location(koord3d::invalid),
	ding(dt),
	raster(get_base_tile_raster_width()),
	welt(dt->get_welt())
{
	set_groesse(size);
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void world_view_t::infowin_event(const event_t* ev)
{
	if(IS_LEFTRELEASE(ev)) {
		const koord3d& pos = (ding != NULL ? ding->get_pos() : location);
		if (welt->ist_in_kartengrenzen(pos.get_2d())) {
			welt->change_world_position(pos);
		}
	}
}


void world_view_t::zeichnen(koord const offset)
{
	display_set_image_proc(false);

	koord3d const here3d    = (ding ? ding->get_pos() : location);
	koord   const here      = here3d.get_2d();
	koord         fine_here = koord(0, 0);
	sint16        y_offset  = 0;
	if (ding) { // offsets?
		fine_here = koord(tile_raster_scale_x(-ding->get_xoff(), raster), tile_raster_scale_x(-ding->get_yoff() % (TILE_STEPS * 2), raster));
		y_offset  = ding->get_yoff() / (TILE_STEPS * 2);
		if (vehikel_basis_t const* const v = ding_cast<vehikel_basis_t>(ding)) {
			int x = 0;
			int y = 0;
			v->get_screen_offset(x, y, raster);
			fine_here -= koord(x, y);
		}
	}

	grund_t const* const kb = welt->lookup_kartenboden(here);
	if (!kb) return;

	int hgt = tile_raster_scale_y(here3d.z * TILE_HEIGHT_STEP / Z_TILE_STEP, raster);
	if (ding) {
		aircraft_t const* const plane = ding_cast<aircraft_t>(ding);
		if (plane) hgt += tile_raster_scale_y(plane->get_flyingheight(), raster);
	}

	koord const pos = get_pos() + offset + koord(1, 1);

	// do not draw outside (may happen with scroll bars)
	clip_dimension const old_clip = display_get_clip_wh();

	// something to draw?
	koord const gr = get_groesse() - koord(2, 2);
	if (old_clip.xx <= pos.x || pos.x + gr.x <= old_clip.x || gr.x <= 0) return;
	if (old_clip.yy <= pos.y || pos.y + gr.y <= old_clip.y || gr.y <= 0) return;

	int const clip_x = max(old_clip.x, pos.x);
	int const clip_y = max(old_clip.y, pos.y);
	display_set_clip_wh(clip_x, clip_y, min(old_clip.xx, pos.x + gr.x) - clip_x, min(old_clip.yy, pos.y + gr.y) - clip_y);

	mark_rect_dirty_wc(pos.x, pos.y, pos.x + gr.x, pos.y + gr.y);

	/* Not very elegant, but works: Fill everything with black for underground
	 * mode. */
	if (grund_t::underground_mode) {
		display_fillbox_wh(pos.x, pos.y, gr.x, gr.y, COL_BLACK, true);
	}

	sint16 const yoff = ding && ding->is_moving() ?
		gr.y / 2 - raster * 3 / 4 : // align 1/4 raster from the bottom of the image
		gr.y     - raster;          // align the bottom of the image
	koord const display_off = koord((gr.x - raster) / 2, hgt + yoff) + fine_here;

	vector_tpl<koord>::const_iterator const start = offsets.begin();
	vector_tpl<koord>::const_iterator const end   = offsets.end();

	// display grounds
	for (vector_tpl<koord>::const_iterator i = start; i != end; ++i) {
		koord  const& off   = *i;
		koord  const  k     = here + off + koord(y_offset, y_offset);
		sint16 const  off_x = (off.x - off.y) * 32 * raster / 64 + display_off.x;

		if (off_x + raster < 0 || gr.x < off_x) continue;

		grund_t const* const kb = welt->lookup_kartenboden(k);
		if (!kb) return;

		sint16 const yypos = display_off.y + (off.y + off.x) * 16 * raster / 64 - tile_raster_scale_y(kb->get_hoehe() * TILE_HEIGHT_STEP / Z_TILE_STEP, raster);
		if (gr.y < yypos + raster / 4) {
			break; // enough with grounds
		} else if (0 <= yypos + raster) {
			kb->display_if_visible(pos.x + off_x, pos.y + yypos, raster);
		}
	}

	// display things
	for (vector_tpl<koord>::const_iterator i = start; i != end; ++i) {
		koord  const& off   = *i;
		koord  const  k     = here + off + koord(y_offset, y_offset);
		sint16 const  off_x = (off.x - off.y) * 32 * raster / 64 + display_off.x;
		if (off_x + raster < 0 || gr.x < off_x) continue;

		planquadrat_t const* const plan = welt->lookup(k);
		if (!plan) continue;
		grund_t const* const kb = plan->get_kartenboden();
		if (!kb) continue;

		sint8 const h = kb->get_hoehe();
		// minimum height: ground height for overground,
		// for the definition of underground_level see grund_t::set_underground_mode
		sint8 const hmin = min(h, grund_t::underground_level);

		// maximum height: 127 for overground, undergroundlevel for sliced, ground height-1 for complete underground view
		sint8 const hmax = grund_t::underground_mode == grund_t::ugm_all ? h - !kb->ist_tunnel() : grund_t::underground_level;

		sint16 const yypos = display_off.y + (off.y + off.x) * 16 * raster / 64 - tile_raster_scale_y(h * TILE_HEIGHT_STEP / Z_TILE_STEP, raster);
		if (0 <= yypos + raster && yypos - raster * 2 < gr.y) {
			plan->display_dinge(pos.x + off_x, pos.y + yypos, raster, false, hmin, hmax);
		} else if (yypos > gr.y) {
			break; // now we can finish
		}
	}

	// this should only happen for airplanes: out of image, so we need to extra display them
	if (y_offset != 0) {
		grund_t const* const g     = welt->lookup(ding->get_pos());
		sint16         const yypos = display_off.y - tile_raster_scale_y(2 * y_offset * 16, raster) - tile_raster_scale_y(g->get_hoehe() * TILE_HEIGHT_STEP / Z_TILE_STEP, raster);
		g->display_dinge_all(pos.x + display_off.x, pos.y + yypos, raster, false);
	}

	display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	display_ddd_box_clip(pos.x - 1, pos.y - 1, gr.x + 2, gr.y + 2, MN_GREY0, MN_GREY4);
}


/**
 * Resize the contents of the window
 * recalculates also the number of tiles needed
 * @author prissi
 */
void world_view_t::set_groesse(koord size)
{
	gui_komponente_t::set_groesse(size);

	const sint16 max_dx=size.x/(raster/2) + 2;
	const sint16 max_dy=(size.y/(raster/2) + 5)&0x0FFE;

	offsets.clear();
	for( sint16 dy=-max_dy;  dy<=2;  ) {
		{
		for( sint16 dx=-2;  dx<max_dx;  dx+=2  ) {
			offsets.append(koord((dy + dx)/2, (dy - dx) / 2));
//			DBG_MESSAGE("world_view_t::set_groesse()","offset %d,%d added",offsets.get(offsets.get_count()-1).x,offsets.get(offsets.get_count()-1).y );
		}
		}
		dy++;
		for( sint16 dx=-1;  dx<max_dx;  dx+=2  ) {
			offsets.append(koord((dy + dx) / 2, (dy - dx) / 2));
//			DBG_MESSAGE("world_view_t::set_groesse()","offset %d,%d added",offsets.get(offsets.get_count()-1).x,offsets.get(offsets.get_count()-1).y );
		}
		dy++;
	}
//	DBG_MESSAGE("world_view_t::set_groesse()","%d tiles added",offsets.get_count() );
}
