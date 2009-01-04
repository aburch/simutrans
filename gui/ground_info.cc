/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simcolor.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"
#include "ground_info.h"


static cbuffer_t gr_info(1024);


grund_info_t::grund_info_t(const grund_t* gr_) :
	gui_frame_t(gr_->get_name(), NULL),
	gr(gr_),
	view(gr_->get_welt(), gr_->get_pos())
{
	const ding_t* d = gr->obj_bei(0);
	if (d != NULL) {
		set_owner(d->get_besitzer());
	}
	gr_info.clear();
	gr->info(gr_info);
	sint16 height = max( count_char(gr_info, '\n')*LINESPACE+36+10, get_tile_raster_width()+30 );

	view.set_pos( koord(165,10) );
	view.set_groesse( koord( get_tile_raster_width(), (get_tile_raster_width()*5)/6)  );
	add_komponente( &view );

	set_fenstergroesse( koord(175+get_tile_raster_width(), height) );
}



/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 */
void grund_info_t::zeichnen(koord pos, koord groesse)
{
	gr_info.clear();
	gr->info(gr_info);
	gui_frame_t::zeichnen(pos,groesse);
	display_multiline_text(pos.x+10, pos.y+16+8, gr_info, COL_BLACK);
}
