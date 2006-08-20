/*
 * messagebox.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifdef _MSC_VER
#include <string.h>
#endif

#include <stdio.h>

#include "../simgraph.h"
#include "../dataobj/translator.h"

#include "messagebox.h"



nachrichtenfenster_t::nachrichtenfenster_t(karte_t */**/, const char *text, image_id id) :
	gui_frame_t("Meldung"),
	bild( id ),
	meldung( translator::translate(text) )
{
	sint16 height = max( meldung.gib_groesse().y+36, get_tile_raster_width()+30 );

	meldung.setze_pos( koord(10, 4) );
	add_komponente( &meldung );

	int xoff, yoff, xw, yw;
	xoff = yw = 0;
	display_get_image_offset( id, &xoff, &yoff, &xw, &yw );

	bild.setze_pos( koord( 220-xw-xoff, 64-yw-yoff) );	// aligned to lower right corner
	add_komponente( &bild );

	setze_fenstergroesse( koord( 230, height ) );
	setze_opaque(true);
}
