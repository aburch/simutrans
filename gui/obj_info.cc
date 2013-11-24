/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Basic class for info window
 * Hj. Malthaner, 2000
 */

#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"
#include "obj_info.h"



obj_infowin_t::obj_infowin_t(const obj_t* obj) :
	gui_frame_t(translator::translate( obj->get_name() ), obj->get_besitzer()),
	view(obj, scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) )),
	textarea(&buf, 170 + view.get_size().w, view.get_size() + scr_size(10, 10))
{
	buf.clear();
	info(buf);
	textarea.recalc_size();

	scr_coord_val width  = textarea.get_size().w + 20;
	scr_coord_val height = max( textarea.get_size().h, view.get_size().h ) + 36;

	view.set_pos( scr_coord(width - view.get_size().w - 10, 10) );
	add_komponente( &view );

	textarea.set_pos( scr_coord(10, 10) );
	add_komponente( &textarea );

	set_windowsize(scr_size(width, height));
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void obj_infowin_t::draw(scr_coord pos, scr_size size)
{
	buf.clear();
	info(buf);
	textarea.recalc_size();

	gui_frame_t::draw( pos, size );

	// Knightly : text may be changed and need more vertical space to display
	const sint16 current_height = max( textarea.get_size().h, view.get_size().h ) + 36;
	if(  current_height != get_windowsize().h  ) {
		set_windowsize( scr_size(get_windowsize().w, current_height) );
	}
}


bool obj_infowin_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center(get_obj()->get_pos()));
}
