/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../world/simworld.h"
#include "../dataobj/translator.h"
#include "ground_info.h"



grund_info_t::grund_info_t(const grund_t* gr_) :
	gui_frame_t("", NULL),
	gr(gr_),
	view(gr_->get_pos(), scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) )),
	textarea(&buf),
	textarea2(&buf)
{
	const obj_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_owner() );
	}
	buf.clear();

	set_table_layout(1, 0);
	set_alignment(ALIGN_CENTER_H);
	add_table(2, 1)->set_alignment(ALIGN_LEFT | ALIGN_TOP);
	{
		add_component(&textarea);
		add_component(&view);
	}
	end_table();
	add_component(&textarea2);
	recalc_size();
}


void grund_info_t::recalc_size()
{
	textarea.set_buf(&buf);
	scr_size sz = textarea.get_size();
	if (sz.w == 0) {
		textarea.set_visible(false);
		textarea2.set_visible(false);
	}
	else if(sz.h==LINESPACE) {
		textarea.set_visible(false);
		textarea2.set_size(sz);
		textarea2.set_visible(true);
	}
	else {
		textarea.set_visible(true);
		textarea2.set_visible(false);
	}
	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}



/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void grund_info_t::draw(scr_coord pos, scr_size size)
{
	// update for owner and name change
	set_dirty();
	const obj_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_owner() );
	}
	gui_frame_t::set_name( translator::translate(gr->get_name()) );

	const cbuffer_t old_buf(buf);
	buf.clear();
	gr->info(buf);
	if(  strcmp( buf, old_buf )  ) {
		recalc_size();
	}

	gui_frame_t::draw(pos, size);
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
	view.map_rotate90(new_ysize);
}
