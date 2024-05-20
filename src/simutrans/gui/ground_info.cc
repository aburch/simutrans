/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../world/simworld.h"
#include "../dataobj/translator.h"
#include "ground_info.h"


grund_info_t::grund_info_t(const grund_t* _gr) :
	base_infowin_t("",NULL),
	gr(_gr),
	lview(_gr->get_pos(), scr_size(max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8)))
{
	fill_buffer();
	set_embedded(&lview);
	textarea.set_width(textarea.get_size().w + get_base_tile_raster_width() - 64);
	recalc_size();
}


void grund_info_t::fill_buffer()
{
	const cbuffer_t old_buf(buf);
	buf.clear();
	gr->info(buf);
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void grund_info_t::draw(scr_coord pos, scr_size size)
{
	scr_size old_t_size = textarea.get_size();
	fill_buffer();

	// update for owner and name change
	set_dirty();
	const obj_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_owner() );
	}

	gui_frame_t::set_name(translator::translate(gr->get_name()));

	gui_frame_t::draw(pos, size);

	set_embedded(&lview);
	if (old_t_size != textarea.get_size()) {
		recalc_size();
	}
}


koord3d grund_info_t::get_weltpos(bool)
{
	return gr->get_pos();
}


bool grund_info_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center( gr->get_pos() ) );
}


void grund_info_t::map_rotate90( sint16 new_ysize )
{
	lview.map_rotate90(new_ysize);
}
