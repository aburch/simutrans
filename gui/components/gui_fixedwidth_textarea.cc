/*
 * Copyright (c) 1997 - 2010 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "gui_fixedwidth_textarea.h"
#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"



gui_fixedwidth_textarea_t::gui_fixedwidth_textarea_t(cbuffer_t* buf_, const sint16 width)
{
	buf = buf_;
	set_width(width);
}



void gui_fixedwidth_textarea_t::recalc_size()
{
	scr_size newsize = calc_display_text(scr_coord::invalid, false);
	if (newsize.h != size.h) {
		gui_komponente_t::set_size( newsize );
	}
}



void gui_fixedwidth_textarea_t::set_width(const sint16 width)
{
	if(  width>0  ) {
		// height is simply reset to 0 as it requires recalculation anyway
		gui_komponente_t::set_size( scr_size(width, 0) );
	}
}



void gui_fixedwidth_textarea_t::set_reserved_area(const scr_size area)
{
	if(  area.w>=0  &&  area.h>=0  ) {
		reserved_area = area;
	}
}



void gui_fixedwidth_textarea_t::set_size(scr_size size)
{
	// y-component (height) in size is deliberately ignored
	set_width(size.w);
}



/* calculates the height of the text that flows around the world_view
 * if draw is true, it will also draw the text
 * borrowed from ding_infowin_t::calc_draw_info() with adaptation
 */
scr_size gui_fixedwidth_textarea_t::calc_display_text(const scr_coord offset, const bool draw)
{
	const bool unicode = translator::get_lang()->utf_encoded;
	scr_coord_val x=0, word_x=0, y = 0;

	const char* text(*buf);
	const utf8 *p = (const utf8 *)text;
	const utf8 *line_start = p;
	const utf8 *word_start = p;
	const utf8 *line_end  = p;

	// also in unicode *c==0 is end
	while(*p!=0  ||  p!=line_end) {

		// force at end of text or newline
		const scr_coord_val max_width = ( y<reserved_area.h ) ? get_size().w-reserved_area.w : get_size().w;

		// smaller than the allowed width?
		do {

			// end of line?
			size_t len = 0;
			uint16 next_char = unicode ? utf8_to_utf16(p, &len) : *p++;
			p += len;

			if(next_char==0  ||  next_char=='\n') {
				line_end = p-1;
				if(  next_char == 0  ) {
					p--;
				}
				word_start = p;
				word_x = 0;
				break;
			}
			// Space: Maybe break here
			else if(  next_char==' '  ||  (next_char >= 0x3000  &&   next_char<0xFE70)  ) {
				// ignore space at start of line
				if(next_char!=' '  ||  x>0) {
					x += (scr_coord_val)display_get_char_width( next_char );
				}
				word_start = p;
				word_x = 0;
			}
			else {
				// normal char: retrieve and calculate width
				int ch_width = display_get_char_width( next_char );
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
			display_text_proportional_len_clip( offset.x, offset.y+y, (const char *)line_start, ALIGN_LEFT | DT_CLIP, COL_BLACK, true, (size_t)(line_end - line_start) );
		}
		y += LINESPACE;
		// back to start of new line
		line_start = word_start;
		x = word_x;
		word_x = 0;
	}

	// reset component height where necessary
	return scr_size(get_size().w, y);
}


void gui_fixedwidth_textarea_t::draw(scr_coord offset)
{
	calc_display_text(offset + get_pos(), true);
}
