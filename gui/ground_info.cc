/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
	textarea(&gr_info, 170 + view.get_groesse().x, view.get_groesse() + koord(10, 10))
{
	const ding_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_besitzer() );
	}

	gr_info.clear();
	gr->info(gr_info);
	textarea.recalc_size();

	sint16 width  = textarea.get_groesse().x + 20;
	sint16 height = max( textarea.get_groesse().y, view.get_groesse().y ) + 36;

	view.set_pos( koord(width - view.get_groesse().x - 10, 10) );
	add_komponente( &view );

	textarea.set_pos( koord(10, 10) );
	add_komponente( &textarea );

	set_fenstergroesse( koord(width, height) );
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
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
	const sint16 current_height = max( textarea.get_groesse().y, view.get_groesse().y ) + 36;
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
