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
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"

#include "message_info_t.h"

message_info_t::message_info_t(karte_t *welt, sint16 color, char *text, koord k, image_id id) :
	gui_frame_t("Meldung"),
	bild( id ),
	meldung(text),
	view(welt, koord3d::invalid)
{
	sint16 height = max( meldung.gib_groesse().y+16, get_tile_raster_width()+30 );

	// good coordinates?
	if(welt->lookup(k)) {
		view.set_location( welt->lookup(k)->gib_kartenboden()->gib_pos() );
		view.setze_pos( koord(230-get_tile_raster_width()-5,10) );
		view.setze_groesse( koord( get_tile_raster_width(), (get_tile_raster_width()*5)/6)  );
		add_komponente( &view );
	}
	else if(id!=IMG_LEER) {
		// then an image?
		int xoff, yoff, xw, yw;
		xoff = yw = 0;
		display_get_image_offset( id, &xoff, &yoff, &xw, &yw );
		bild.setze_pos( koord( 220-xw-xoff, get_tile_raster_width()-yw-yoff) );	// aligned to lower right corner
		add_komponente( &bild );
	}
	setze_fenstergroesse( koord(230, height) );

	meldung.setze_pos( koord(10, 10) );
	add_komponente( &meldung );

	set_title_color( color );

	setze_opaque(true);
}
