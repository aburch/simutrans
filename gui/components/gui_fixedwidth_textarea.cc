/*
 * Copyright (c) 1997 - 2010 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "gui_fixedwidth_textarea.h"
#include "../../dataobj/translator.h"



gui_fixedwidth_textarea_t::gui_fixedwidth_textarea_t(const char *const text_, const sint16 width, const koord reserved_area_)
{
 	set_text(text_);
	set_width(width);
	set_reserved_area(reserved_area_);
}



void gui_fixedwidth_textarea_t::recalc_size()
{
	calc_display_text(koord::invalid, false);
}



void gui_fixedwidth_textarea_t::set_width(const sint16 width)
{
	if(  width>0  ) {
		// height is simply reset to 0 as it requires recalculation anyway
		gui_komponente_t::set_groesse( koord(width, 0) );
	}
}



void gui_fixedwidth_textarea_t::set_text(const char *const text_)
{
	if(  text_  ) {
		text = text_;
	}
	else {
		text = "";
	}
}



void gui_fixedwidth_textarea_t::set_reserved_area(const koord area)
{
	if(  area.x>=0  &&  area.y>=0  ) {
		reserved_area = area;
	}
}



void gui_fixedwidth_textarea_t::set_groesse(koord groesse)
{
	// y-component (height) in groesse is deliberately ignored
	set_width(groesse.x);
}



/* calculates the height of the text that flows around the world_view
 * if draw is true, it will also draw the text
 * borrowed from ding_infowin_t::calc_draw_info() with adaptation
 */
void gui_fixedwidth_textarea_t::calc_display_text(const koord offset, const bool draw)
{
	const bool unicode = translator::get_lang()->utf_encoded;
	KOORD_VAL x=0, word_x=0, y = 0;

	const utf8 *p = (const utf8 *)text;
	const utf8 *line_start = p;
	const utf8 *word_start = p;
	const utf8 *line_end  = p;

	// also in unicode *c==0 is end
	while(*p!=0  ||  p!=line_end) {

		// force at end of text or newline
		const KOORD_VAL max_width = ( y<reserved_area.y ) ? get_groesse().x-reserved_area.x : get_groesse().x;

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
				size_t len = 0;
				int ch_width = display_get_char_width( unicode ? utf8_to_utf16(p, &len) : *p++ );
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
			display_text_proportional_len_clip( offset.x, offset.y+y, (const char *)line_start, ALIGN_LEFT | DT_DIRTY | DT_CLIP, COL_BLACK, (size_t)(line_end - line_start) );
		}
		y += LINESPACE;
		// back to start of new line
		line_start = word_start;
		x = word_x;
		word_x = 0;
	}

	// reset component height where necessary
	if(  y!=get_groesse().y  ) {
		gui_komponente_t::set_groesse( koord(get_groesse().x, y) );
	}
}



void gui_fixedwidth_textarea_t::zeichnen(koord offset)
{
	calc_display_text(offset + get_pos(), true);
}
