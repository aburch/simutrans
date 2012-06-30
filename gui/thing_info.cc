/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Basisklasse fuer Infofenster
 * Hj. Malthaner, 2000
 */

#include "../simcolor.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"
#include "thing_info.h"



ding_infowin_t::ding_infowin_t(const ding_t* ding) :
	gui_frame_t(translator::translate( ding->get_name() ), ding->get_besitzer()),
	view(ding, koord( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) )),
	textarea(&buf, 170 + view.get_groesse().x, view.get_groesse() + koord(10, 10))
{
	buf.clear();
	info(buf);
	textarea.recalc_size();

	KOORD_VAL width  = textarea.get_groesse().x + 20;
	KOORD_VAL height = max( textarea.get_groesse().y, view.get_groesse().y ) + 36;

	view.set_pos( koord(width - view.get_groesse().x - 10, 10) );
	add_komponente( &view );

	textarea.set_pos( koord(10, 10) );
	add_komponente( &textarea );

	set_fenstergroesse(koord(width, height));
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 */
void ding_infowin_t::zeichnen(koord pos, koord gr)
{
	set_dirty();

	buf.clear();
	info(buf);
	textarea.recalc_size();

	gui_frame_t::zeichnen( pos, gr );

	// Knightly : text may be changed and need more vertical space to display
	const sint16 current_height = max( textarea.get_groesse().y, view.get_groesse().y ) + 36;
	if(  current_height != get_fenstergroesse().y  ) {
		set_fenstergroesse( koord(get_fenstergroesse().x, current_height) );
	}
}


bool ding_infowin_t::is_weltpos()
{
	karte_t *welt = get_ding()->get_welt();
	return ( welt->get_x_off() | welt->get_y_off()) == 0  &&
		welt->get_world_position() == welt->calculate_world_position( get_ding()->get_pos() );
}
