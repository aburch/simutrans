/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * Basisklasse fuer Infofenster
 * Hj. Malthaner, 2000
 */

#include "../simcolor.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../simplay.h"

#include "../dataobj/translator.h"

#include "ground_info.h"

cbuffer_t grund_info_t::gr_info(1024);


grund_info_t::grund_info_t(const grund_t* gr_) :
	gui_frame_t(gr_->gib_name(), NULL),
	gr(gr_),
	view(gr_->gib_welt(), gr_->gib_pos())
{
	if(gr->obj_bei(0)!=NULL) {
		set_owner( gr->obj_bei(0)->gib_besitzer() );
	}
	gr_info.clear();
	gr->info(gr_info);
	sint16 height = max( count_char(gr_info, '\n')*LINESPACE+36+10, get_tile_raster_width()+30 );

	view.setze_pos( koord(230-get_tile_raster_width()-5,10) );
	view.setze_groesse( koord( get_tile_raster_width(), (get_tile_raster_width()*5)/6)  );
	add_komponente( &view );

	setze_fenstergroesse( koord(230, height) );

	setze_opaque(true);
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
