/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "../../simdebug.h"
#include "../../ifc/gui_fenster.h"
#include "gui_textinput.h"
#include "../../simevent.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"
#include "../../unicode.h"

#include "../../dataobj/translator.h"

#include "../../simgraph.h"

gui_textinput_t::gui_textinput_t() :
	text(NULL),
	max(0),
	cursor_pos(0),
	align(ALIGN_LEFT),
	textcol(COL_BLACK)
{
	set_allow_focus(true);
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_textinput_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == EVENT_KEYBOARD) {
		if(  text  ) {
			const size_t len = strlen(text);

			switch(ev->ev_code) {
					// handled by container
				case 9:
				case 13:
				case 27:
					return false;

				case SIM_KEY_DOWN: // down arrow
					// not used currently
					break;
				case SIM_KEY_LEFT: // left arrow
					if (cursor_pos > 0) {
						cursor_pos = get_prev_char(text, cursor_pos);
					}
					break;
				case SIM_KEY_RIGHT: // right arrow
					if (cursor_pos > strlen(text)) {
						cursor_pos = strlen(text);
					}
					else {
						cursor_pos = get_next_char(text, cursor_pos);
					}
					break;
				case SIM_KEY_UP: // rightarrow
					// not used currently
					break;
				case SIM_KEY_HOME: // home
					cursor_pos = 0;
					break;
				case SIM_KEY_END: // end
					cursor_pos = strlen(text);
					break;
				case 8:
					// backspace
					if(cursor_pos > 0) {
						if (cursor_pos < len) {
							size_t prev_pos = cursor_pos;
							cursor_pos = get_prev_char(text, cursor_pos);
							for (size_t pos = cursor_pos; pos <= len-(prev_pos-cursor_pos); pos++) {
								text[pos] = text[pos+(prev_pos-cursor_pos)];
							}
						}
						else {
							cursor_pos = get_prev_char(text, cursor_pos);
							text[cursor_pos] = 0;
						}
					}
					break;
				case 127:
					// delete
					if (cursor_pos <= len) {
							size_t next_pos = get_next_char(text, cursor_pos);
							for (size_t pos = cursor_pos; pos < len; pos++) {
								text[pos] = text[pos+(next_pos-cursor_pos)];
							}
					}
					break;
				default:
					if(ev->ev_code < 32) {
						// ignore special keys not handled so far
						break;
					}
					// insert letters, numbers, and special characters

					// test, if we have top convert letter
					char letter[8];

					if(ev->ev_code>=128) {
						sprintf( letter, "CHR%X", ev->ev_code );
			//DBG_MESSAGE( "gui_textinput_t::gui_textinput_t()","%i=%s",ev->ev_code,letter);
						const char *more_letter=translator::translate(letter);
						// could not convert ...
						if(letter==more_letter) {
							if(translator::get_lang()->utf_encoded) {
								char *out=letter;
								out[ utf16_to_utf8(ev->ev_code, (utf8 *)out) ] = 0;
							}
							else {
								// guess some east european letter
								uint8 new_char = ev->ev_code>255 ? unicode_to_latin2( ev->ev_code ) : ev->ev_code;
								if(  new_char==0  ) {
									// >255 but no translation => assume extended code page
									new_char = (ev->ev_code & 0x7F) | 0x80;
								}
								letter[0] = new_char;
								letter[1] = 0;
							}
						}
						else {
							// successful converted letter
							strcpy( letter, more_letter );
						}
					}
					else {
						letter[0] = ev->ev_code;
						letter[1] = 0;
					}

					size_t num_letter = strlen(letter);

					if(len+num_letter>=max) {
						// too many chars ...
						break;
					}

					// insert into text?
					if (cursor_pos < len) {
						for (sint64 pos = len+num_letter; pos >= (sint64)cursor_pos; pos--) {
							text[pos] = text[pos-num_letter];
						}
						memcpy( text+cursor_pos, letter, num_letter );
					}
					else {
						// append to text
						memcpy( text+len, letter, num_letter );
						text[len+num_letter] = 0;
					}
					cursor_pos += num_letter;
					/* end default */
			}
		}
		else {
			DBG_MESSAGE("gui_textinput_t::infowin_event", "called but text is NULL");
		}
		return true;
	} else if ( IS_LEFTCLICK(ev) ) 	{
		// acting on release causes unwanted recalculations of cursor position for long strings and (cursor_offset>0)
		// moreover, only (click) or (release) event happened inside textinput, the other one could lie outside
		cursor_pos = 0;
		if (text) {
			for( size_t i=strlen(text); i>0;  i-- ) {
				if(ev->cx+cursor_offset > display_calc_proportional_string_len_width(text,i)) {
					cursor_pos = i;
					break;
				}
			}
		}
DBG_DEBUG("gui_textinput_t::gui_textinput_t()","cursor_pos=%i, cx=%i",cursor_pos,ev->cx);
		return true;
	}
	else if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_UNTOP  ) {
		call_listeners((long)0);
		return true;
	}
	return false;
}


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_textinput_t::zeichnen(koord offset)
{
	const gui_fenster_t *win = win_get_top();
	zeichnen_mit_cursor( offset, (win  &&  win->get_focus()==this) );
}



void gui_textinput_t::zeichnen_mit_cursor(koord offset,bool show_cursor)
{
	display_fillbox_wh_clip(pos.x+offset.x+1, pos.y+offset.y+1,groesse.x-2, groesse.y-2, MN_GREY1, true);
	display_ddd_box_clip(pos.x+offset.x, pos.y+offset.y,groesse.x, groesse.y,MN_GREY0, MN_GREY4);

	if(text) {
		// align_offset = len of str for right align, otherwise 0
		int align_offset = (align == ALIGN_RIGHT) ? proportional_string_width(text) : 0;

		cursor_offset = proportional_string_len_width(text, cursor_pos);
		if (cursor_offset+2 > groesse.x) {
			cursor_offset -= (groesse.x - 3);
		}
		else {
			cursor_offset = 0;
			if (align == ALIGN_RIGHT){
				cursor_offset = align_offset-(groesse.x - 3);
				if (cursor_offset > 0) {
					cursor_offset = 0;
				}
			}
		}

		// set clipping to be within textinput button
		const clip_dimension old_clip = display_get_clip_wh();

		int text_clip_x = pos.x+offset.x + 1, text_clip_w = groesse.x - 2;
		// something to draw?
		if (text_clip_x >= old_clip.xx || text_clip_x+text_clip_w <= old_clip.x || text_clip_w<=0) {
				return;
		}
		const int clip_x =  old_clip.x > text_clip_x ? old_clip.x : text_clip_x;
		display_set_clip_wh( clip_x, old_clip.y, min(old_clip.xx, text_clip_x+text_clip_w)-clip_x, old_clip.h);

		// display text
		display_proportional_clip(pos.x+offset.x+2-cursor_offset+align_offset, pos.y+offset.y+1+(groesse.y-large_font_height)/2, text, align, textcol, true);

		if(  show_cursor  ) {
			// cursor must been shown, if textinput has focus!
			display_fillbox_wh_clip(pos.x+offset.x+1+proportional_string_len_width(text, cursor_pos)-cursor_offset, pos.y+offset.y+1, 1, 11, COL_WHITE, true);
		}

		// reset clipping
		display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	}
}



void gui_textinput_t::set_text(char *text, size_t max)
{
	this->text = text;
	this->max = max;
	cursor_pos = strlen(text);
}



// needed to set the cursor on the right position
bool gui_hidden_textinput_t::infowin_event(const event_t *ev)
{
	if ( IS_LEFTCLICK(ev) ) 	{
		// acting on release causes unwanted recalculations of cursor position for long strings and (cursor_offset>0)
		// moreover, only (click) or (release) event happened inside textinput, the other one could lie outside
		sint16 asterix_width = display_calc_proportional_string_len_width("*",1);
		cursor_pos = 0;
		if (text) {
			cursor_pos = min( strlen(text), ev->cx/asterix_width );
		}
		return true;
DBG_DEBUG("gui_textinput_t::gui_textinput_t()","cursor_pos=%i, cx=%i",cursor_pos,ev->cx);
	}
	else {
		return gui_textinput_t::infowin_event( ev );
	}
	return false;
}



void gui_hidden_textinput_t::zeichnen(koord offset)
{
	display_fillbox_wh_clip(pos.x+offset.x+1, pos.y+offset.y+1,groesse.x-2, groesse.y-2, MN_GREY1, true);
	display_ddd_box_clip(pos.x+offset.x, pos.y+offset.y,groesse.x, groesse.y,MN_GREY0, MN_GREY4);

	if(text) {
		// the text will be all asterics, thus we draw them letter by letter

		// set clipping to be within textinput button
		const clip_dimension old_clip = display_get_clip_wh();

		int text_clip_x = pos.x+offset.x + 1, text_clip_w = groesse.x - 2;
		// something to draw?
		if (text_clip_x >= old_clip.xx || text_clip_x+text_clip_w <= old_clip.x || text_clip_w<=0) {
				return;
		}
		const int clip_x =  old_clip.x > text_clip_x ? old_clip.x : text_clip_x;
		display_set_clip_wh( clip_x, old_clip.y, min(old_clip.xx, text_clip_x+text_clip_w)-clip_x, old_clip.h);

		size_t text_pos=0;
		sint16 xpos = pos.x+offset.x+2;
		utf16  c = 0;
		do {
			// cursor?
			if(  text_pos==cursor_pos  ) {
				display_fillbox_wh_clip( xpos, pos.y+offset.y+1, 1, 11, COL_WHITE, true);
			}
			c = utf8_to_utf16((utf8 const*)text + text_pos, &text_pos);
			if(c) {
				xpos += display_proportional_clip( xpos, pos.y+offset.y+1+(groesse.y-large_font_height)/2, "*", ALIGN_LEFT, textcol, true);
			}
		}
		while(  text_pos<max  &&  c  );

		// reset clipping
		display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	}
}
