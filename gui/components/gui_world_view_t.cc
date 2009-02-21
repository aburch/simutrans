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



world_view_t::world_view_t(karte_t* welt, koord3d location)
{
    this->location = location;
    this->ding = NULL;
    this->welt = welt;
    this->raster = 0;

    set_groesse(koord(64,56));
}


world_view_t::world_view_t(const ding_t* dt) :
	location(koord3d::invalid),
	ding(dt),
	raster(0),
	welt(dt->get_welt())
{
	set_groesse(koord(64,56));
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


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void
world_view_t::zeichnen(koord offset)
{
	const sint16 raster = get_tile_raster_width();

//DBG_MESSAGE("world_view_t::zeichnen()","ding %p, location %d,%d,%d",ding,location.x,location.y,location.z );
	koord here=ding!=NULL ? ding->get_pos().get_2d() : location.get_2d();
	koord fine_here=koord(0,0);
	sint16 y_offset=0;
	// offsets?
	if(ding) {
		fine_here = koord( 	tile_raster_scale_x(-ding->get_xoff(),raster), tile_raster_scale_x(-ding->get_yoff()%(TILE_STEPS*2),raster) );
		y_offset = (ding->get_yoff()/(32*TILE_STEPS/16));
		if(ding->is_moving()) {
			int x=0, y=0;
			((const vehikel_basis_t*)ding)->get_screen_offset(x, y);
			fine_here -= koord( x, y );
		}
	}

	const planquadrat_t * plan = welt->lookup(here);
	if(plan  &&  plan->get_kartenboden()) {
		const koord gr=get_groesse()-koord(2,2);
		int hgt;
		if(!ding) {
			hgt = tile_raster_scale_y( plan->get_kartenboden()->get_hoehe()*TILE_HEIGHT_STEP/Z_TILE_STEP, raster );
		} else {
			hgt = tile_raster_scale_y( ding->get_pos().z*TILE_HEIGHT_STEP/Z_TILE_STEP, raster );
			if(ding->get_typ() == ding_t::aircraft) {
				const aircraft_t *plane =  dynamic_cast <const aircraft_t *>(ding);
				hgt += tile_raster_scale_y( plane->get_flyingheight(), raster );
			}
		}
		const koord pos = get_pos()+offset+koord(1,1);

		// do not draw outside (may happen with scroll bars)
		const clip_dimension old_clip = display_get_clip_wh();
		const int clip_x =  max(old_clip.x,pos.x);
		const int clip_y =  max(old_clip.y,pos.y);
		display_set_clip_wh( clip_x, clip_y, min(old_clip.x+old_clip.w,pos.x+gr.x)-clip_x, min(old_clip.y+old_clip.h,pos.y+gr.y)-clip_y );

		mark_rect_dirty_wc( pos.x, pos.y, pos.x+gr.x, pos.y+gr.y );

		// not very elegant, but works:
		// fill everything with black for Underground mode ...
		if(grund_t::underground_mode) {
			display_fillbox_wh(pos.x, pos.y, gr.x, gr.y, COL_BLACK, true);
		}

		const koord display_off = koord( min( (gr.x-raster)/2, raster/2), hgt+gr.y-raster )+fine_here;	// we aling the bottom of the image with the small image

		// display grounds
		for(uint32 i=0; i<offsets.get_count(); i++) {
			const koord k = here + offsets[i] + koord(y_offset, y_offset);
			const sint16 off_x = (offsets[i].x - offsets[i].y) * 32 * raster / 64 + display_off.x;

			if(off_x+raster<0  ||  off_x>gr.x) {
				continue;
			}

			plan = welt->lookup(k);
			if(plan  &&  plan->get_kartenboden()) {
				const sint16 yypos = display_off.y + (offsets[i].y + offsets[i].x) * 16 * raster / 64 - tile_raster_scale_y(plan->get_kartenboden()->get_hoehe() * TILE_HEIGHT_STEP / Z_TILE_STEP, raster);
				if(yypos+(raster/4)>gr.y) {
					// enough with grounds ...
					break;
				}
				if(yypos+raster>=0) {
					plan->display_boden(pos.x + off_x, pos.y + yypos);
				}
			}
		}

		// display things
		for(uint32 i=0; i<offsets.get_count(); i++) {
			const koord k = here + offsets[i] + koord(y_offset, y_offset);
			const sint16 off_x = (offsets[i].x - offsets[i].y) * 32 * raster / 64 + display_off.x;

			if(off_x+raster<0  ||  off_x>gr.x) {
				continue;
			}

			plan = welt->lookup(k);
			if(plan  &&  plan->get_kartenboden()) {
				const sint16 yypos = display_off.y + (offsets[i].y + offsets[i].x) * 16 * raster / 64 - tile_raster_scale_y(plan->get_kartenboden()->get_hoehe() * TILE_HEIGHT_STEP / Z_TILE_STEP, raster);
				if(yypos-(raster*2)<gr.y  &&  yypos+raster>=0) {
					plan->display_dinge(pos.x+off_x,pos.y+yypos,raster,false);
				}
				else if(yypos>gr.y) {
					// now we can finish
					break;
				}
			}
		}

		// this should only happen for airplanes: out of image, so we need to extra display them
		if(y_offset!=0) {
			const sint16 yypos = display_off.y - tile_raster_scale_y((2*y_offset)*16,raster) - tile_raster_scale_y( welt->lookup(ding->get_pos())->get_hoehe()*TILE_HEIGHT_STEP/Z_TILE_STEP, raster);
			welt->lookup(ding->get_pos())->display_dinge( pos.x+display_off.x, pos.y+yypos, false);
		}

		display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
		display_ddd_box_clip(pos.x-1, pos.y-1, gr.x+2, gr.y+2, MN_GREY0, MN_GREY4);
	}
}




/**
 * Resize the contents of the window
 * recalculates also the number of tiles needed
 * @author prissi
 */
void world_view_t::set_groesse(koord size)
{
	gui_komponente_t::set_groesse(size);

	raster = get_tile_raster_width();
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
