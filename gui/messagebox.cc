/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifdef _MSC_VER
#include <string.h>
#endif

#include "../simworld.h"
#include "../simgraph.h"

#include "../dataobj/translator.h"
#include "messagebox.h"



nachrichtenfenster_t::nachrichtenfenster_t(karte_t *welt, const char *text, image_id id, koord k, PLAYER_COLOR_VAL title_color) :
	gui_frame_t("Meldung"),
	bild( id ),
	meldung( translator::translate(text) ),
	view(welt, koord3d::invalid),
	color( title_color )
{
	sint16 height = max( meldung.gib_groesse().y+16+10+4, get_tile_raster_width()+30 );

	// good coordinates?
	const planquadrat_t* p = welt->lookup(k);
	if (p) {
		view.set_location(p->gib_kartenboden()->gib_pos());
		view.setze_pos( koord(230-get_tile_raster_width()-5,10) );
		view.setze_groesse( koord( get_tile_raster_width(), (get_tile_raster_width()*5)/6)  );
		add_komponente( &view );
	}
	else if(id!=IMG_LEER) {
		// then an image?
		int xoff, yoff, xw, yw;
		xoff = yw = 0;
		display_get_image_offset( id, &xoff, &yoff, &xw, &yw );
		bild.setze_pos( koord( 220-xw-xoff, 10-yoff) );	// aligned to upper right corner
		add_komponente( &bild );
	}
	setze_fenstergroesse( koord(230, height) );

	meldung.setze_pos( koord(10, 10) );
	add_komponente( &meldung );
	setze_opaque(true);
}
