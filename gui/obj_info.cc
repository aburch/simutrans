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
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"
#include "obj_info.h"



obj_infowin_t::obj_infowin_t(const obj_t* obj) :
	gui_frame_t(translator::translate( obj->get_name() ), obj->get_besitzer()),
	view(obj, koord( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) )),
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
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void obj_infowin_t::zeichnen(koord pos, koord gr)
{
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


bool obj_infowin_t::is_weltpos()
{
	karte_t *welt = get_obj()->get_welt();
	return ( welt->get_x_off() | welt->get_y_off()) == 0  &&
		welt->get_world_position() == welt->calculate_world_position( get_obj()->get_pos() );
}
