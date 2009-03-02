/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../simgraph.h"

#include "../dataobj/translator.h"
#include "messagebox.h"



news_window::news_window(const char* t, PLAYER_COLOR_VAL title_color) :
	gui_frame_t("Meldung"),
	text(strdup(translator::translate(t))),
	meldung(text),
	color(title_color)
{
	sint16 height = max( meldung.get_groesse().y+16+10+4, get_tile_raster_width()+30 );

	set_fenstergroesse( koord(230, height) );

	meldung.set_pos( koord(10, 10) );
	add_komponente( &meldung );
}


news_window::~news_window()
{
	// allocated by strdup() ...
	free( text );
}



news_img::news_img(const char* text, image_id id, PLAYER_COLOR_VAL color) :
	news_window(text, color),
	bild(id)
{
	KOORD_VAL xoff, yoff, xw, yw;
	display_get_image_offset(id, &xoff, &yoff, &xw, &yw);
	bild.set_pos(koord(220 - xw - xoff, 10 - yoff)); // aligned to upper right corner
	add_komponente(&bild);
}


news_loc::news_loc(karte_t* welt, const char* text, koord k, PLAYER_COLOR_VAL color) :
	news_window(text, color),
	view(welt, welt->lookup_kartenboden(k)->get_pos())
{
	view.set_pos(koord(230 - get_tile_raster_width() - 5, 10));
	view.set_groesse(koord(get_tile_raster_width(), get_tile_raster_width() * 5 / 6));
	add_komponente(&view);
}




void news_loc::map_rotate90( sint16 new_ysize )
{
	koord3d l = view.get_location();
	l.rotate90( new_ysize );
	view.set_location( l );
}
