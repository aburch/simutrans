/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * An adapter class to display info windows for ground (floor) objects
 *
 * @author Hj. Malthaner
 * @date 20-Nov-2001
 */

#include "../simcolor.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"
#include "ground_info.h"


cbuffer_t grund_info_t::gr_info;


grund_info_t::grund_info_t(const grund_t* gr_) :
	gui_frame_t( translator::translate(gr_->get_name()), NULL),
	gr(gr_),
	view(gr_->get_welt(), gr_->get_pos(), koord( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) )),
	textarea(&gr_info, 170 + view.get_groesse().x, view.get_groesse() + koord(D_H_SPACE, D_V_SPACE))
{
	const ding_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_besitzer() );
	}

	gr_info.clear();
	gr->info(gr_info);
	textarea.recalc_size();

	KOORD_VAL width  = textarea.get_groesse().x + D_MARGIN_LEFT + D_MARGIN_RIGHT;
	KOORD_VAL height = D_TITLEBAR_HEIGHT + D_MARGIN_TOP + max( textarea.get_groesse().y, view.get_groesse().y ) + D_MARGIN_BOTTOM;

	view.set_pos( koord(width - view.get_groesse().x - D_MARGIN_RIGHT, D_MARGIN_TOP) );
	add_komponente( &view );

	textarea.set_pos( koord(D_MARGIN_LEFT, D_MARGIN_TOP) );
	add_komponente( &textarea );

	set_fenstergroesse( koord(width, height) );
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void grund_info_t::zeichnen(koord pos, koord groesse)
{
	set_dirty();
	const ding_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_besitzer() );
	}
	gui_frame_t::set_name( translator::translate(gr->get_name()) );

	gr_info.clear();
	gr->info(gr_info);
	textarea.recalc_size();

	gui_frame_t::zeichnen(pos, groesse);

	// Knightly : text may be changed and need more vertical space to display
	const sint16 current_height =  D_TITLEBAR_HEIGHT + D_MARGIN_TOP + max( textarea.get_groesse().y, view.get_groesse().y ) + D_MARGIN_BOTTOM;
	if(  current_height != get_fenstergroesse().y  ) {
		set_fenstergroesse( koord(get_fenstergroesse().x, current_height) );
	}
}


koord3d grund_info_t::get_weltpos(bool)
{
	return gr->get_pos();
}


bool grund_info_t::is_weltpos()
{
	karte_t *welt = gr->get_welt();
	return ( welt->get_x_off() | welt->get_y_off()) == 0  &&
		welt->get_world_position() == welt->calculate_world_position( gr->get_pos() );
}


void grund_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}
