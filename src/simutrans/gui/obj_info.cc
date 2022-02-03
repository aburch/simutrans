/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../world/simworld.h"
#include "../dataobj/translator.h"
#include "obj_info.h"



obj_infowin_t::obj_infowin_t(const obj_t* obj) :
	base_infowin_t(translator::translate( obj->get_name() ), obj->get_owner()),
	view(obj, scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{
	textarea.set_width( textarea.get_size().w + get_base_tile_raster_width() - 64);
	fill_buffer();
	set_embedded(&view);
}


void obj_infowin_t::fill_buffer()
{
	buf.clear();
	get_obj()->info(buf);
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void obj_infowin_t::draw(scr_coord pos, scr_size size)
{
	const cbuffer_t old_buf(buf);
	fill_buffer();
	if(  strcmp( buf, old_buf )  ) {
		recalc_size();
	}

	base_infowin_t::draw( pos, size );
}


bool obj_infowin_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center(get_obj()->get_pos()));
}
