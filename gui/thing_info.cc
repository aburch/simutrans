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
#include "../simplay.h"
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"
#include "thing_info.h"


cbuffer_t ding_infowin_t::buf (8192);



ding_infowin_t::ding_infowin_t(const ding_t* ding_) :
	gui_frame_t("", ding_->gib_besitzer()),
	view(ding_),
	ding(ding_)
{
	view.setze_pos( koord(175,10) );
	view.setze_groesse( koord( get_tile_raster_width(), (get_tile_raster_width()*5)/6)  );
	add_komponente( &view );

	buf.clear();
	info(buf);

	KOORD_VAL width  = 180 + get_tile_raster_width();
	KOORD_VAL height = max( calc_draw_info( koord::invalid, false )+20+16, get_tile_raster_width() + 30);
	setze_fenstergroesse(koord(width, height));
}



/* calculates the height of the text that flows around the world_view
 * if draw is true, it will also draw the text
 */
KOORD_VAL
ding_infowin_t::calc_draw_info( koord pos, bool draw ) const
{
	const bool unicode = translator::get_lang()->utf_encoded;
	KOORD_VAL x=0, word_x=0, y = 10;
	const KOORD_VAL view_height = view.gib_groesse().y+10;

	const utf8 *p = (const utf8 *)(const char *)buf;
	const utf8 *line_start = p;
	const utf8 *word_start = p;
	const utf8 *line_end  = p;

	// also in unicode *c==0 is end
	while(*p!=0) {

		// force at end of text or newline
		const KOORD_VAL max_width = (y<view_height) ? 155 : 155+view.gib_groesse().x;

		// smaller than the allowd width?
		do {

			// end of line?
			if(*p==0  ||  *p=='\n') {
				line_end = p;
				if(*p!=0) {
					// skip newline
					p ++;
				}
				word_start = p;
				word_x = 0;
				break;
			}
			// Space: Maybe break here
			else if(*p==' ') {
				// ignore space at start of line
				if(x>0) {
					x += (KOORD_VAL)display_get_char_width(' ');
				}
				p ++;
				word_start = p;
				word_x = 0;
			}
			else {
				// normal char: retrieve and calculate width
				int len = 0;
				int ch_width = display_get_char_width( unicode ? utf8_to_utf16((const utf8 *)p, &len) : *p++ );
				p += len;
				x += ch_width;
				word_x += ch_width;
			}
		}	while(  x<max_width  );

		// spaces at the end can be omitted
		line_end = word_start;
		if(line_end==line_start) {
			// too long word for a single line => break the word
			word_start = line_end = p;
			word_x = 0;
		}
		else if(word_start[-1]==' '  ||  word_start[-1]=='\n') {
			line_end --;
		}

		// start of new line or end of text
		if(draw  &&  (line_end-line_start)!=0) {
			display_text_proportional_len_clip( pos.x+10, pos.y+y+16, (const char *)line_start, ALIGN_LEFT, COL_BLACK, line_end - line_start);
		}
		y += LINESPACE;
		// back to start of new line
		line_start = word_start;
		x = word_x;
		word_x = 0;
	}

	return y;
}



/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 */
void ding_infowin_t::zeichnen(koord pos, koord gr)
{
	set_dirty();
	gui_frame_t::setze_name( gib_name() );
	gui_frame_t::zeichnen( pos, gr );

	buf.clear();
	info(buf);

	calc_draw_info( pos, true );
}
