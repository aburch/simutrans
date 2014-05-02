/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../display/simgraph.h"

#include "../simskin.h"
#include "../besch/skin_besch.h"

#include "../dataobj/translator.h"
#include "messagebox.h"


news_window::news_window(const char* text, PLAYER_COLOR_VAL title_color) :
	base_infowin_t( translator::translate("Meldung" ) ),
	color(title_color)
{
	buf.clear();
	buf.append(translator::translate(text));

	// adjust positions, sizes, and window-size
	recalc_size();
}


fatal_news::fatal_news(const char* text) :
	news_window(text, WIN_TITEL)
{
	textarea.set_width(display_get_width()/2);
	recalc_size();
}


news_img::news_img(const char* text) :
	news_window(text, WIN_TITEL),
	bild()
{
	init(skinverwaltung_t::meldungsymbol->get_bild_nr(0));
}


news_img::news_img(const char* text, image_id id, PLAYER_COLOR_VAL color) :
	news_window(text, color),
	bild()
{
	init(id);
}


/**
 * just puts the image in top-right corner
 * only cembedded.d from constructor
 * @param id id of image
 */
void news_img::init(image_id id)
{
	if(  id!=IMG_LEER  ) {
		bild.set_image(id, true);

		scr_coord_val xoff, yoff, xw, yw;
		display_get_base_image_offset(id, &xoff, &yoff, &xw, &yw);
		bild.set_size( scr_size(xw, yw) );

		set_embedded(&bild);
	}
}


news_loc::news_loc(const char* text, koord k, PLAYER_COLOR_VAL color) :
	news_window(text, color),
	view(welt->lookup_kartenboden(k)->get_pos(), scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{
	set_embedded(&view);
}


// returns position of the location shown in the subwindow
koord3d news_loc::get_weltpos(bool)
{
	return view.get_location();
}


void news_loc::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}
