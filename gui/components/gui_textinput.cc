/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "gui_textinput.h"
#include "../../simwin.h"
#include "../../simsys.h"
#include "../../dataobj/translator.h"


gui_textinput_t::gui_textinput_t() :
	gui_komponente_t(true),
	text(NULL),
	max(0),
	head_cursor_pos(0),
	tail_cursor_pos(0),
	scroll_offset(0),
	align(ALIGN_LEFT),
	textcol(COL_BLACK),
	cursor_reference_time(0),
	focus_recieved(false)
{ }


/**
 * determine new cursor position from event coordinates
 * @author Knightly
 */
size_t gui_textinput_t::calc_cursor_pos(const int x)
{
	size_t new_cursor_pos = 0;
	if (  text  ) {
		const char* tmp_text = text;
		uint8 byte_length = 0;
		uint8 pixel_width = 0;
		KOORD_VAL current_offset = 0;
		const KOORD_VAL adjusted_offset = x - 1 + scroll_offset;
		while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  adjusted_offset>(current_offset+(pixel_width>>1))  ) {
			current_offset += pixel_width;
			new_cursor_pos += byte_length;
		}
	}
	return new_cursor_pos;
}


/**
 * Remove selected text portion, if any.
 * Returns true if some selected text is actually deleted.
 * @author Knightly
 */
bool gui_textinput_t::remove_selection()
{
	if(  head_cursor_pos!=tail_cursor_pos  ) {
		size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
		size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
		tail_cursor_pos = head_cursor_pos = start_pos;
		do {
			text[start_pos++] = text[end_pos];
		} while(  text[end_pos++]!=0  );
		return true;
	}
	return false;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_textinput_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class==EVENT_KEYBOARD  ) {
		if(  text  ) {
			size_t len = strlen(text);

			switch(ev->ev_code) {
					// handled by container
				case SIM_KEY_ENTER:
					call_listeners((long)1);
				case SIM_KEY_TAB:
					// Knightly : focus is going to be lost -> reset cursor positions to select the whole text by default
					head_cursor_pos = len;
					tail_cursor_pos = 0;
				case SIM_KEY_ESCAPE:
					return false;

				case 1:
					// Knightly : if Ctrl-A -> select the whole text
					if(  IS_CONTROL_PRESSED(ev)  ) {
						head_cursor_pos = len;
						tail_cursor_pos = 0;
					}
					break;
				case 3:
					// Knightly : if Ctrl-C -> copy selected text to clipboard
					if(  IS_CONTROL_PRESSED(ev)  &&  head_cursor_pos!=tail_cursor_pos  ) {
						const size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
						const size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
						dr_copy(text + start_pos, end_pos - start_pos);
					}
					break;
				case 22:
					// Knightly : if Ctrl-V -> paste selected text to cursor position
					if(  IS_CONTROL_PRESSED(ev)  ) {
						if(  remove_selection()  ) {
							// recalculate text length after deleting selection
							len = strlen(text);
						}
						tail_cursor_pos = ( head_cursor_pos += dr_paste(text + head_cursor_pos, max - len - 1) );
					}
					break;
				case 24:
					// Knightly : if Ctrl-X -> cut and copy selected text to clipboard
					if(  IS_CONTROL_PRESSED(ev)  &&  head_cursor_pos!=tail_cursor_pos  ) {
						const size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
						const size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
						dr_copy(text + start_pos, end_pos - start_pos);
						remove_selection();
					}
					break;
				case SIM_KEY_DOWN: // down arrow
					// not used currently
					break;
				case SIM_KEY_LEFT: // left arrow
					if(  head_cursor_pos>0  ) {
						// Knightly : Ctrl key pressed -> skip over to the start of the previous word (as delimited by space(s))
						if(  IS_CONTROL_PRESSED(ev)  ) {
							const char* tmp_text = text + head_cursor_pos;
							uint8 byte_length;
							uint8 pixel_width;
							// first skip over all contiguous space characters to the left
							while(  head_cursor_pos>0  &&  get_prev_char_with_metrics(tmp_text, text, byte_length, pixel_width)==SIM_KEY_SPACE  ) {
								head_cursor_pos -= byte_length;
							}
							// revert text pointer for further processing
							if(  head_cursor_pos>0  ) {
								tmp_text += byte_length;
							}
							// then skip over all contiguous non-space characters further to the left
							while(  head_cursor_pos>0  &&  get_prev_char_with_metrics(tmp_text, text, byte_length, pixel_width)!=SIM_KEY_SPACE  ) {
								head_cursor_pos -= byte_length;
							}
						}
						else {
							head_cursor_pos = get_prev_char(text, head_cursor_pos);
						}
					}
					// Knightly : do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_RIGHT: // right arrow
					if(  head_cursor_pos<len  ) {
						// Knightly : Ctrl key pressed -> skip over to the start of the next word (as delimited by space(s))
						if(  IS_CONTROL_PRESSED(ev)  ) {
							const char* tmp_text = text + head_cursor_pos;
							uint8 byte_length;
							uint8 pixel_width;
							// first skip over all contiguous non-space characters to the right
							while(  head_cursor_pos<len  &&  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)!=SIM_KEY_SPACE  ) {
								head_cursor_pos += byte_length;
							}
							// revert text pointer for further processing
							if(  head_cursor_pos<len  ) {
								tmp_text -= byte_length;
							}
							// then skip over all contiguous space characters further to the right
							while(  head_cursor_pos<len  &&  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)==SIM_KEY_SPACE  ) {
								head_cursor_pos += byte_length;
							}
						}
						else {
							head_cursor_pos = get_next_char(text, head_cursor_pos);
						}
					}
					// Knightly : do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_UP: // up arrow
					// not used currently
					break;
				case SIM_KEY_HOME: // home
					head_cursor_pos = 0;
					// Knightly : do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_END: // end
					head_cursor_pos = len;
					// Knightly : do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_BACKSPACE:
					// backspace
					// Knightly : check and remove any selected text first
					if(  !remove_selection()  &&  head_cursor_pos>0  ) {
						if (  head_cursor_pos<len  ) {
							size_t prev_pos = head_cursor_pos;
							tail_cursor_pos = head_cursor_pos = get_prev_char(text, head_cursor_pos);
							for (  size_t pos=head_cursor_pos;  pos<=len-(prev_pos-head_cursor_pos);  pos++  ) {
								text[pos] = text[pos+(prev_pos-head_cursor_pos)];
							}
						}
						else {
							tail_cursor_pos = head_cursor_pos = get_prev_char(text, head_cursor_pos);
							text[head_cursor_pos] = 0;
						}
					}
					break;
				case SIM_KEY_DELETE:
					// delete
					// Knightly : check and remove any selected text first
					if(  !remove_selection()  &&  head_cursor_pos<=len  ) {
						size_t next_pos = get_next_char(text, head_cursor_pos);
						for(  size_t pos=head_cursor_pos;  pos<len;  pos++  ) {
							text[pos] = text[pos+(next_pos-head_cursor_pos)];
						}
					}
					break;
				default:
					if(ev->ev_code < 32) {
						// ignore special keys not handled so far
						break;
					}
					// insert letters, numbers, and special characters

					// Knightly : first check if it is necessary to remove selected text portion
					if(  remove_selection()  ) {
						// recalculate text length after deleting selection
						len = strlen(text);
					}

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
					if (head_cursor_pos < len) {
						for(  sint64 pos=len+num_letter;  pos>=(sint64)head_cursor_pos;  pos--  ) {
							text[pos] = text[pos-num_letter];
						}
						memcpy( text+head_cursor_pos, letter, num_letter );
					}
					else {
						// append to text
						memcpy( text+len, letter, num_letter );
						text[len+num_letter] = 0;
					}
					tail_cursor_pos = ( head_cursor_pos += num_letter );
					/* end default */
			}
		}
		else {
			DBG_MESSAGE("gui_textinput_t::infowin_event", "called but text is NULL");
		}
		cursor_reference_time = dr_time();	// update reference time for cursor blinking
		return true;
	}
	else if(  IS_LEFTCLICK(ev)  ) {
		// acting on release causes unwanted recalculations of cursor position for long strings and (scroll_offset>0)
		// moreover, only (click) or (release) event happened inside textinput, the other one could lie outside
		// Knightly : use mouse *click* position; update both head and tail cursors
		tail_cursor_pos = head_cursor_pos = calc_cursor_pos(ev->cx);
		cursor_reference_time = dr_time();	// update reference time for cursor blinking
		return true;
	}
	else if(  IS_LEFTDRAG(ev)  ) {
		// Knightly : use mouse *move* position; update head cursor only in order to enable text selection
		head_cursor_pos = calc_cursor_pos(ev->mx);
		cursor_reference_time = dr_time();	// update reference time for cursor blinking
		return true;
	}
	else if(  IS_LEFTDBLCLK(ev)  ) {
		// Knightly : select a word as delimited by spaces
		// for tail cursor pos -> skip over all contiguous non-space characters to the left
		const char* tmp_text = text + tail_cursor_pos;
		uint8 byte_length;
		uint8 pixel_width;
		while(  tail_cursor_pos>0  &&  get_prev_char_with_metrics(tmp_text, text, byte_length, pixel_width)!=SIM_KEY_SPACE  ) {
			tail_cursor_pos -= byte_length;
		}
		// for head cursor pos -> skip over all contiguous non-space characters to the right
		const size_t len = strlen(text);
		tmp_text = text + head_cursor_pos;
		while(  head_cursor_pos<len  &&  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)!=SIM_KEY_SPACE  ) {
			head_cursor_pos += byte_length;
		}
	}
	else if(  IS_LEFTTPLCLK(ev)  ) {
		// Knightly : select the whole text
		head_cursor_pos = strlen(text);
		tail_cursor_pos = 0;
	}
	else if(  ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_UNTOP  ) {
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
	display_with_focus( offset, (win_get_focus()==this) );
}


/**
 * Detect change of focus state and determine whether cursor should be displayed,
 * and call the function that performs the actual display
 * @author Knightly
 */
void gui_textinput_t::display_with_focus(koord offset, bool has_focus)
{
	// check if focus state has changed
	if(  focus_recieved!=has_focus  ) {
		if(  has_focus  ) {
			// update reference time for cursor blinking if focus has just been received
			cursor_reference_time = dr_time();
		}
		focus_recieved = has_focus;
	}

	display_with_cursor( offset, has_focus, (has_focus  &&  ((dr_time()-cursor_reference_time)&512ul)==0) );
}


void gui_textinput_t::display_with_cursor(koord offset, bool cursor_active, bool cursor_visible)
{
	display_fillbox_wh_clip(pos.x+offset.x+1, pos.y+offset.y+1,groesse.x-2, groesse.y-2, MN_GREY1, true);
	display_ddd_box_clip(pos.x+offset.x, pos.y+offset.y,groesse.x, groesse.y,MN_GREY0, MN_GREY4);

	if(  text  ) {
		// Knightly : recalculate scroll offset
		const KOORD_VAL text_width = proportional_string_width(text);
		const KOORD_VAL view_width = groesse.x - 3;
		const KOORD_VAL cursor_offset = cursor_active ? proportional_string_len_width(text, head_cursor_pos) : 0;
		if(  text_width<=view_width  ) {
			// case : text is shorter than displayable width of the text input
			//			-> the only case where left and right alignments differ
			//			-> pad empty spaces on the left if right-aligned
			scroll_offset = align==ALIGN_RIGHT ? text_width - view_width : 0;
		}
		else {
			if(  scroll_offset<0  ) {
				// case : if text is longer than displayable width of the text input
				//			but there is empty space to the left
				//			-> the text should move leftwards to fill up the empty space
				scroll_offset = 0;
			}
			if(  cursor_offset-scroll_offset>view_width  ) {
				// case : cursor has moved too far off the right side
				scroll_offset = cursor_offset - view_width;
			}
			else if(  cursor_offset<scroll_offset  ) {
				// case : cursor has moved past the scroll offset towards the left side
				scroll_offset = cursor_offset;
			}
			if(  scroll_offset+view_width>text_width  ) {
				// case : if text is longer than displayable width of the text input
				//			but there is empty space to the right
				//			-> the text should move rightwards to fill up the empty space
				scroll_offset = text_width - view_width;
			}
		}

		// set clipping to be within textinput button
		const clip_dimension old_clip = display_get_clip_wh();

		const int text_clip_x = pos.x + offset.x + 1;
		const int text_clip_w = groesse.x - 2;
		const int text_clip_y = pos.y + offset.y + 1;
		const int text_clip_h = groesse.y - 2;
		// something to draw?
		if (  text_clip_x>=old_clip.xx  ||  text_clip_x+text_clip_w<=old_clip.x  ||  text_clip_w<=0  ||
			  text_clip_y>=old_clip.yy  ||  text_clip_y+text_clip_h<=old_clip.y  ||  text_clip_h<=0     ) {
				return;
		}
		const int clip_x = old_clip.x>text_clip_x ? old_clip.x : text_clip_x;
		const int clip_y = old_clip.y>text_clip_y ? old_clip.y : text_clip_y;
		display_set_clip_wh( clip_x, clip_y, min(old_clip.xx, text_clip_x+text_clip_w)-clip_x, min(old_clip.yy, text_clip_y+text_clip_h)-clip_y );

		// display text
		display_proportional_clip(pos.x+offset.x+2-scroll_offset, pos.y+offset.y+1+(groesse.y-LINESPACE)/2, text, ALIGN_LEFT, textcol, true);

		if(  cursor_active  ) {
			// Knightly : display selected text block with light grey text on charcoal bounding box
			if(  head_cursor_pos!= tail_cursor_pos  ) {
				const size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
				const size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
				const KOORD_VAL start_offset = proportional_string_len_width(text, start_pos);
				const KOORD_VAL highlight_width = proportional_string_len_width(text+start_pos, end_pos-start_pos);
				display_fillbox_wh_clip(pos.x+offset.x+2-scroll_offset+start_offset, pos.y+offset.y+1, highlight_width, 11, COL_GREY2, true);
				display_text_proportional_len_clip(pos.x+offset.x+2-scroll_offset+start_offset, pos.y+offset.y+1+(groesse.y-LINESPACE)/2, text+start_pos, ALIGN_LEFT|DT_DIRTY|DT_CLIP, COL_GREY5, end_pos-start_pos);
			}

			// display blinking cursor
			if(  cursor_visible  ) {
				display_fillbox_wh_clip(pos.x+offset.x+1-scroll_offset+cursor_offset, pos.y+offset.y+1, 1, 11, COL_WHITE, true);
			}
		}

		// reset clipping
		display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	}
}



void gui_textinput_t::set_text(char *text, size_t max)
{
	this->text = text;
	this->max = max;
	// Knightly : whole text is selected by default
	head_cursor_pos = strlen(text);
	tail_cursor_pos = 0;
}



// needed to set the cursor on the right position
bool gui_hidden_textinput_t::infowin_event(const event_t *ev)
{
	if(  IS_LEFTCLICK(ev)  ) {
		// acting on release causes unwanted recalculations of cursor position for long strings and (cursor_offset>0)
		// moreover, only (click) or (release) event happened inside textinput, the other one could lie outside
		sint16 asterix_width = display_calc_proportional_string_len_width("*",1);
		head_cursor_pos = 0;
		if (  text  ) {
			head_cursor_pos = min( strlen(text), ev->cx/asterix_width );
		}
		cursor_reference_time = dr_time();	// update reference time for cursor blinking
		return true;
	}
	else {
		return gui_textinput_t::infowin_event( ev );
	}
	return false;
}



void gui_hidden_textinput_t::display_with_cursor(koord const offset, bool, bool const cursor_visible)
{
	display_fillbox_wh_clip(pos.x+offset.x+1, pos.y+offset.y+1,groesse.x-2, groesse.y-2, MN_GREY1, true);
	display_ddd_box_clip(pos.x+offset.x, pos.y+offset.y,groesse.x, groesse.y,MN_GREY0, MN_GREY4);

	if(  text  ) {
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
			if(  cursor_visible  &&  text_pos==head_cursor_pos  ) {
				display_fillbox_wh_clip( xpos, pos.y+offset.y+1, 1, 11, COL_WHITE, true);
			}
			c = utf8_to_utf16((utf8 const*)text + text_pos, &text_pos);
			if(c) {
				xpos += display_proportional_clip( xpos, pos.y+offset.y+1+(groesse.y-LINESPACE)/2, "*", ALIGN_LEFT, textcol, true);
			}
		}
		while(  text_pos<max  &&  c  );

		// reset clipping
		display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	}
}
