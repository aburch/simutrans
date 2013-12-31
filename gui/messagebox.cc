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



news_window::news_window(const char* t, PLAYER_COLOR_VAL title_color) :
	gui_frame_t( translator::translate("Meldung" ) ),
	textarea(&buf, 160),
	color(title_color)
{
	buf.clear();
	buf.append(translator::translate(t));
	textarea.set_pos( scr_coord(10, 10) );
	add_komponente( &textarea );
}


// Knightly :	set component boundary and windows size, position component and add it to the component list
//				if component is NULL, the message box will contain only text
void news_window::extend_window_with_component(gui_komponente_t *const component, const scr_size size, const scr_coord offset)
{
	if(  component  ) {
		textarea.set_reserved_area( size + scr_size(10, 10) );
		textarea.set_width(textarea.get_size().w + 10 + size.w);
		textarea.recalc_size();
		const sint16 width = textarea.get_size().w + 20;
		const sint16 height = max( textarea.get_size().h, size.h ) + 36;
		set_windowsize( scr_size(width, height) );
		component->set_pos( scr_coord(width - size.w - 10 + offset.x, 10 + offset.y) );
		add_komponente(component);
	}
	else {
		textarea.set_width(textarea.get_size().w + 10 + size.w);
		textarea.recalc_size();
		set_windowsize( scr_size(textarea.get_size().w + 20, textarea.get_size().h + 36) );
	}
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
 * only called from constructor
 * @param id id of image
 */
void news_img::init(image_id id)
{
	bild.set_image(id);
	if(  id!=IMG_LEER  ) {
		scr_coord_val xoff, yoff, xw, yw;
		display_get_base_image_offset(id, &xoff, &yoff, &xw, &yw);
		extend_window_with_component(&bild, scr_size(xw, yw), scr_coord(-xoff, -yoff));
	}
	else {
		extend_window_with_component(NULL, scr_size(0, 0));
	}
}


news_loc::news_loc(const char* text, koord k, PLAYER_COLOR_VAL color) :
	news_window(text, color),
	view(welt->lookup_kartenboden(k)->get_pos(), scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{
	extend_window_with_component(&view, view.get_size());
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
