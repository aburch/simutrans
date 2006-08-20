/*
 * infowin.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


/* infowin.cc
 *
 * Basisklasse fuer Infofenster
 * Hj. Malthaner, 2000
 */


#include "../simcolor.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"
#include "thing_info.h"

karte_t * ding_infowin_t::welt = NULL;
cbuffer_t ding_infowin_t::buf (8192);


ding_infowin_t::ding_infowin_t(karte_t *welt,ding_t *ding) :
	gui_frame_t(""),
	view(welt, koord3d::invalid)
{
	this->welt = welt;
	this->ding = ding;
	setze_opaque(true);

	view.setze_pos( koord(175,10) );
	view.setze_groesse( koord( get_tile_raster_width(), (get_tile_raster_width()*5)/6)  );

	add_komponente( &view );
	setze_fenstergroesse( koord(180+get_tile_raster_width(), calc_fensterhoehe_aus_info()) );
}



int
ding_infowin_t::calc_fensterhoehe_aus_info() const
{
	buf.clear();
	info(buf);
	return max(count_char(buf, '\n')*LINESPACE+36+10, get_tile_raster_width()+30 );
}




/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 */
void ding_infowin_t::zeichnen(koord pos, koord gr)
{
	view.set_location(gib_pos());

	gui_frame_t::setze_name( gib_name() );
	gui_frame_t::set_title_color( gib_besitzer() ? gib_besitzer()->get_player_color() : COL_ORANGE );

	gui_frame_t::zeichnen(pos,gr);

	buf.clear();
	info(buf);
	display_multiline_text(pos.x+10, pos.y+16+8, buf, COL_BLACK);
}
