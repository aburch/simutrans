/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "gui_textinput.h"
#include "../simwin.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"
#include "../../sys/simsys.h"


gui_textinput_t::gui_textinput_t() :
	gui_component_t(true),
	text(NULL),
	composition(),
	composition_target_start(0),
	composition_target_length(0),
	max(0),
	head_cursor_pos(0),
	tail_cursor_pos(0),
	scroll_offset(0),
	align(ALIGN_LEFT),
	textcol(SYSCOL_EDIT_TEXT),
	text_dirty(false),
	cursor_reference_time(0),
	focus_received(false)
{ }


scr_size gui_textinput_t::get_min_size() const
{
	return scr_size(4*LINESPACE, ::max(LINESPACE+4, D_EDIT_HEIGHT) );
}


scr_size gui_textinput_t::get_max_size() const
{
	return scr_size( scr_size::inf.w, ::max(LINESPACE+4, D_EDIT_HEIGHT) );
}


/**
 * determine new cursor position from event coordinates
 */
size_t gui_textinput_t::calc_cursor_pos(const int x)
{
	size_t new_cursor_pos = 0;
	if(  text  ) {

		const char* tmp_text = text;
		uint8 byte_length = 0;
		uint8 pixel_width = 0;
		scr_coord_val current_offset = 0;
		const scr_coord_val adjusted_offset = x - 1 + scroll_offset;
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
 */
bool gui_textinput_t::remove_selection()
{
	if(  head_cursor_pos!=tail_cursor_pos  ) {
		size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
		size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
		tail_cursor_pos = head_cursor_pos = start_pos;
		do {
			text_dirty = true;
			text[start_pos++] = text[end_pos];
		} while(  text[end_pos++]!=0  );
		return true;
	}
	return false;
}


void gui_textinput_t::set_composition_status( char *c, int start, int length )
{
	composition.clear();
	if(  win_get_focus()==this  ) {
		if(  c && c[0]!='\0' ) {
			if(  head_cursor_pos!=tail_cursor_pos  ) {
				remove_selection();
			}
			composition.clear();
			composition.append( (char *)c );
			composition_target_start = start;
			composition_target_length = length;

			scr_coord gui_xy = win_get_pos( win_get_top() );
			int offset_to_target = proportional_string_len_width( composition.get_str(), composition_target_start );
			int x = pos.x + gui_xy.x + get_current_cursor_x() + offset_to_target;
			int y = pos.x + gui_xy.y + D_TITLEBAR_HEIGHT;
			dr_notify_input_pos( x, y );
		}
	}
}


/**
 * Events werden hiermit an die GUI-components gemeldet
 */
bool gui_textinput_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class==EVENT_KEYBOARD  ) {
		if(  text  ) {
			size_t len = strlen(text);

			switch(ev->ev_code) {
					// handled by container
				case SIM_KEY_ENTER:
					if(  text_dirty  ) {
						text_dirty = false;
						call_listeners((long)1);
					}
					/* FALLTHROUGH */
				case SIM_KEY_TAB:
					// focus is going to be lost -> reset cursor positions to select the whole text by default
					head_cursor_pos = len;
					tail_cursor_pos = 0;
					/* FALLTHROUGH */
				case SIM_KEY_ESCAPE:
					return false;

				case 1:
					// if Ctrl-A -> select the whole text
					if(  IS_CONTROL_PRESSED(ev)  ) {
						head_cursor_pos = len;
						tail_cursor_pos = 0;
					}
					break;
				case 3:
					// if Ctrl-C -> copy selected text to clipboard
					if(  IS_CONTROL_PRESSED(ev)  &&  head_cursor_pos!=tail_cursor_pos  ) {
						const size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
						const size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
						dr_copy(text + start_pos, end_pos - start_pos);
					}
					break;
				case 22:
					// if Ctrl-V -> paste selected text to cursor position
					if(  IS_CONTROL_PRESSED(ev)  ) {
						if(  remove_selection()  ) {
							// recalculate text length after deleting selection
							len = strlen(text);
						}
						tail_cursor_pos = ( head_cursor_pos += dr_paste(text + head_cursor_pos, max - len - 1) );
						text_dirty = true;
					}
					break;
				case 24:
					// if Ctrl-X -> cut and copy selected text to clipboard
					if(  IS_CONTROL_PRESSED(ev)  &&  head_cursor_pos!=tail_cursor_pos  ) {
						const size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
						const size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
						dr_copy(text + start_pos, end_pos - start_pos);
						remove_selection();
						text_dirty = true;
					}
					break;
				case SIM_KEY_DOWN: // down arrow
					// not used currently
					break;
				case SIM_KEY_LEFT: // left arrow
					if(  head_cursor_pos>0  ) {
						// Ctrl key pressed -> skip over to the start of the previous word (as delimited by space(s))
						if(  IS_CONTROL_PRESSED(ev)  ) {
							const char* tmp_text = text + head_cursor_pos;
							uint8 byte_length = 0;
							uint8 pixel_width = 0;
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
					// do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_RIGHT: // right arrow
					if(  head_cursor_pos<len  ) {
						// Ctrl key pressed -> skip over to the start of the next word (as delimited by space(s))
						if(  IS_CONTROL_PRESSED(ev)  ) {
							const char* tmp_text = text + head_cursor_pos;
							uint8 byte_length = 0;
							uint8 pixel_width = 0;
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
					// do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_UP: // up arrow
					// not used currently
					break;
				case SIM_KEY_HOME: // home
					head_cursor_pos = 0;
					// do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_END: // end
					head_cursor_pos = len;
					// do not update tail cursor if SHIFT key is pressed -> enables text selection
					if(  !IS_SHIFT_PRESSED(ev)  ) {
						tail_cursor_pos = head_cursor_pos;
					}
					break;
				case SIM_KEY_BACKSPACE:
					// backspace
					// check and remove any selected text first
					text_dirty |= len>0;
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
						text_dirty = true;
					}
					break;
				case SIM_KEY_DELETE:
					// delete
					// check and remove any selected text first
					text_dirty |= len>0;
					if(  !remove_selection()  &&  head_cursor_pos<=len  ) {
						size_t next_pos = get_next_char(text, head_cursor_pos);
						for(  size_t pos=head_cursor_pos;  pos<len;  pos++  ) {
							text[pos] = text[pos+(next_pos-head_cursor_pos)];
						}
						text_dirty = true;
					}
					break;
				default:
					if(ev->ev_code < 32) {
						// ignore special keys not handled so far
						break;
					}
					else if (ev->ev_code>=SIM_KEY_NUMPAD_BASE  &&  ev->ev_code<=SIM_KEY_NUMPAD_BASE+9) {
						// ignore numpad keys if numlock is off
						// Could also return false here to move the map diagonally but this does not work for 2/4/6/8
						// (SIM_KEY_LEFT/_RIGHT moves the cursor already)
						break;
					}

					// insert letters, numbers, and special characters

					// first check if it is necessary to remove selected text portion
					if(  remove_selection()  ) {
						// recalculate text length after deleting selection
						len = strlen(text);
					}
					text_dirty = true;

					// test, if we have top convert letter
					char letter[16];

					if(ev->ev_code>=128) {
						sprintf( letter, "CHR%X", ev->ev_code );
			//DBG_MESSAGE( "gui_textinput_t::gui_textinput_t()","%i=%s",ev->ev_code,letter);
						const char *more_letter=translator::translate(letter);
						// could not convert ...
						if(letter==more_letter) {
							char *out=letter;
							out[ utf16_to_utf8(ev->ev_code, (utf8 *)out) ] = 0;
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

					const size_t num_letter = strlen(letter);

					if(len+num_letter>=max) {
						// too many chars ...
						break;
					}

					// insert into text?
					if (head_cursor_pos < len) {
						// copy the trailing '\0' too
						for(  sint64 pos=len;  pos>=(sint64)head_cursor_pos;  pos--  ) {
							text[pos+num_letter] = text[pos];
						}
						memcpy( text+head_cursor_pos, letter, num_letter );
					}
					else {
						// append to text
						memcpy( text+len, letter, num_letter );
						text[len+num_letter] = 0;
					}
					text_dirty = true;
					tail_cursor_pos = ( head_cursor_pos += num_letter );
					/* end default */
			}
		}
		else {
			DBG_MESSAGE("gui_textinput_t::infowin_event", "called but text is NULL");
		}
		cursor_reference_time = dr_time(); // update reference time for cursor blinking
		return true;
	}
	else if(  ev->ev_class==EVENT_STRING  ) {
		composition.clear();

		// UTF8 multi-byte sequence
		if(  text  &&  ev->ev_ptr  ) {
			size_t len = strlen(text);
			utf8 *in = (utf8 *)ev->ev_ptr;
			size_t in_pos = 0;

			// first check if it is necessary to remove selected text portion
			if(  remove_selection()  ) {
				// recalculate text length after deleting selection
				len = strlen(text);
			}

			while(  *in  ) {
				utf32 const uc = utf8_decoder_t::decode(in, in_pos);

				text_dirty = true;

				size_t num_letter = in_pos;
				char letter[16];

				// test, if we have top convert letter
				sprintf( letter, "CHR%X", uc );
				const char *more_letter = translator::translate(letter);
				// could not convert ...
				if(  letter == more_letter  ) {
					tstrncpy( letter, (const char *)in, in_pos+1 );
				}
				else {
					// successful converted letter
					strcpy( letter, more_letter );
					num_letter = strlen(more_letter);
				}
				in += in_pos;

				if(  len+num_letter >= max  ) {
					// too many chars ...
					break;
				}

				// insert into text?
				if(  len>0  &&  head_cursor_pos < len  ) {
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
				text_dirty = true;
				tail_cursor_pos = ( head_cursor_pos += num_letter );
				len += num_letter;
			} /* while still characters in string */
		}
	}
	else if(  IS_LEFTCLICK(ev)  ) {
		// since now the focus could be received while the mouse  no there, we must release it
		scr_rect this_comp( get_size() );
		if(  !this_comp.contains(scr_coord(ev->cx,ev->cy) )  ) {
			// not us, just in old focus from previous selection or tab
			return false;
		}
		// acting on release causes unwanted recalculations of cursor position for long strings and (scroll_offset>0)
		// moreover, only (click) or (release) event happened inside textinput, the other one could lie outside
		// use mouse *click* position; update both head and tail cursors
		tail_cursor_pos = 0;
		if(  text  ) {
			tail_cursor_pos = head_cursor_pos = display_fit_proportional( text, ev->cx - 2 + scroll_offset );
		}
		cursor_reference_time = dr_time(); // update reference time for cursor blinking
		return true;
	}
	else if(  IS_LEFTDRAG(ev)  ) {
		// since now the focus could be received while the mouse  no there, we must release it
		scr_rect this_comp( get_size() );
		if(  !this_comp.contains(scr_coord(ev->cx,ev->cy) )  ) {
			// not us, just in old focus from previous selection or tab
			return false;
		}
		// use mouse *move* position; update head cursor only in order to enable text selection
		head_cursor_pos = 0;
		if(  text  ) {
			head_cursor_pos = display_fit_proportional( text, ev->mx - 1 + scroll_offset );
		}
		cursor_reference_time = dr_time(); // update reference time for cursor blinking
		return true;
	}
	else if(  IS_LEFTDBLCLK(ev)  ) {
		// since now the focus could be received while the mouse  no there, we must release it
		scr_rect this_comp( get_size() );
		if(  !this_comp.contains(scr_coord(ev->cx,ev->cy) )  ) {
			// not us, just in old focus from previous selection or tab
			return false;
		}
		// select a word as delimited by spaces
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
		// since now the focus could be received while the mouse  no there, we must release it
		scr_rect this_comp( get_size() );
		if(  !this_comp.contains(scr_coord(ev->cx,ev->cy) )  ) {
			// not us, just in old focus from previous selection or tab
			return false;
		}
		// select the whole text
		head_cursor_pos = strlen(text);
		tail_cursor_pos = 0;
	}
	else if(  ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_UNTOP  ) {
		if(  text_dirty  ) {
			call_listeners((long)0);
		}
		return true;
	}
	else if(  ev->ev_class == INFOWIN   &&  ev->ev_code == WIN_CLOSE  &&  focus_received  ) {
		// release focus on close and close keyboard
		dr_stop_textinput();
		focus_received = false;
	}
	return false;
}


/**
 * Draw the component
 */
void gui_textinput_t::draw(scr_coord offset)
{
	display_with_focus( offset, (win_get_focus()==this) );
}


/**
 * Detect change of focus state and determine whether cursor should be displayed,
 * and call the function that performs the actual display
 */
void gui_textinput_t::display_with_focus(scr_coord offset, bool has_focus)
{
	// check if focus state has changed
	if(  focus_received!=has_focus  ) {
		if(  has_focus  ) {
			// update reference time for cursor blinking if focus has just been received
			cursor_reference_time = dr_time();

			dr_start_textinput();

			scr_coord gui_xy = win_get_pos( win_get_top() );
			int x = pos.x + gui_xy.x + get_current_cursor_x();
			int y = pos.x + gui_xy.y + D_TITLEBAR_HEIGHT;
			dr_notify_input_pos( x, y );
		}
		else {
			dr_stop_textinput();
		}
		focus_received = has_focus;
	}

	display_with_cursor( offset, has_focus, (has_focus  &&  ((dr_time()-cursor_reference_time)&512ul)==0) );
}


void gui_textinput_t::display_with_cursor(scr_coord offset, bool cursor_active, bool cursor_visible)
{
	display_img_stretch( gui_theme_t::editfield, scr_rect( pos+offset, size ) );

	if(  text  ) {
		// recalculate scroll offset
		const int text_width = proportional_string_width(text);
		const scr_coord_val view_width = size.w - 3;
		const int cursor_offset = cursor_active ? proportional_string_len_width(text, head_cursor_pos) : 0;
		if(  text_width<=view_width  ) {
			// case : text is shorter than displayable width of the text input
			//        -> the only case where left and right alignments differ
			//        -> pad empty spaces on the left if right-aligned
			scroll_offset = align==ALIGN_RIGHT ? text_width - view_width : 0;
		}
		else {
			if(  scroll_offset<0  ) {
				// case : if text is longer than displayable width of the text input
				//        but there is empty space to the left
				//        -> the text should move leftwards to fill up the empty space
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
				//        but there is empty space to the right
				//        -> the text should move rightwards to fill up the empty space
				scroll_offset = text_width - view_width;
			}
		}

		// set clipping to be within textinput button
		const int text_clip_x = pos.x + offset.x + 1;
		const int text_clip_w = size.w - 2;
		const int text_clip_y = pos.y + offset.y + 1;
		const int text_clip_h = size.h - 2;

		PUSH_CLIP_FIT(text_clip_x, text_clip_y, text_clip_w, text_clip_h);

		const int x_base_offset = pos.x+offset.x+2-scroll_offset;
		const int y_offset = pos.y+offset.y+D_GET_CENTER_ALIGN_OFFSET(LINESPACE,size.h);

		// display text (before composition)
		display_text_proportional_len_clip_rgb(x_base_offset, y_offset, text, ALIGN_LEFT | DT_CLIP, textcol, true, head_cursor_pos);
		int x_offset = proportional_string_len_width(text, head_cursor_pos);

		// IME text to display?
		if(  composition.len()  ) {
//			assert(head_cursor_pos==tail_cursor_pos);

			display_proportional_clip_rgb(x_base_offset+x_offset, y_offset, composition.get_str(), ALIGN_LEFT | DT_CLIP, textcol, true);

			// draw underline
			int composition_width = proportional_string_width(composition.get_str());
			display_direct_line_rgb(x_base_offset+x_offset, y_offset+LINESPACE, x_base_offset+x_offset+composition_width, y_offset+LINESPACE-1, textcol);

			// mark targeted part in a similar manner to selected text
			int start_offset = proportional_string_len_width(composition.get_str(), composition_target_start);
			int highlight_width = proportional_string_len_width(composition.get_str()+composition_target_start, composition_target_length);
			display_fillbox_wh_clip_rgb(x_base_offset+x_offset+start_offset, y_offset, highlight_width, LINESPACE, SYSCOL_EDIT_BACKGROUND_SELECTED, true);
			display_text_proportional_len_clip_rgb(x_base_offset+x_offset+start_offset, y_offset, composition.get_str()+composition_target_start, ALIGN_LEFT|DT_CLIP, SYSCOL_EDIT_TEXT_SELECTED, false, composition_target_length);

			x_offset += composition_width;
		}

		// display text (after composition)
		display_proportional_clip_rgb(x_base_offset+x_offset, y_offset, text+head_cursor_pos, ALIGN_LEFT | DT_CLIP, textcol, true);

		if(  cursor_active  ) {
			// display selected text block with light grey text on charcoal bounding box
			if(  head_cursor_pos!= tail_cursor_pos  ) {
				const size_t start_pos = min(head_cursor_pos, tail_cursor_pos);
				const size_t end_pos = ::max(head_cursor_pos, tail_cursor_pos);
				const scr_coord_val start_offset = proportional_string_len_width(text, start_pos);
				const scr_coord_val highlight_width = proportional_string_len_width(text+start_pos, end_pos-start_pos);
				display_fillbox_wh_clip_rgb(x_base_offset+start_offset, y_offset, highlight_width, LINESPACE, SYSCOL_EDIT_BACKGROUND_SELECTED, true);
				display_text_proportional_len_clip_rgb(x_base_offset+start_offset, y_offset, text+start_pos, ALIGN_LEFT|DT_CLIP, SYSCOL_EDIT_TEXT_SELECTED, false, end_pos-start_pos);
			}

			// display blinking cursor
			if(  cursor_visible  ) {
				display_fillbox_wh_clip_rgb(x_base_offset+cursor_offset-1, y_offset, 1, LINESPACE, SYSCOL_CURSOR_BEAM, true);
			}
		}

		// reset clipping
		POP_CLIP();
	}
}



void gui_textinput_t::set_text(char *text, size_t max)
{
	this->text = text;
	this->max = max;
	// whole text is selected by default
	head_cursor_pos = strlen(text);
	tail_cursor_pos = 0;
	text_dirty = false;
}



// needed to set the cursor on the right position
bool gui_hidden_textinput_t::infowin_event(const event_t *ev)
{
	if(  IS_LEFTRELEASE(ev)  ) {
		// since now the focus could be received while the mouse  no there, we must release it
		scr_rect this_comp( get_size() );
		if(  !this_comp.contains(scr_coord(ev->cx,ev->cy) )  ) {
			// not us, just in old focus from previous selection or tab
			return false;
		}
		// acting on release causes unwanted recalculations of cursor position for long strings and (cursor_offset>0)
		// moreover, only (click) or (release) event happened inside textinput, the other one could lie outside
		sint16 asterix_width = display_calc_proportional_string_len_width("*",1);
		head_cursor_pos = 0;
		if (  text  ) {
			head_cursor_pos = min( strlen(text), ev->cx/asterix_width );
		}
		cursor_reference_time = dr_time(); // update reference time for cursor blinking
		return true;
	}
	else {
		return gui_textinput_t::infowin_event( ev );
	}
}



void gui_hidden_textinput_t::display_with_cursor(scr_coord const offset, bool, bool const cursor_visible)
{
	display_img_stretch( gui_theme_t::editfield, scr_rect( pos+offset, size ) );

	if(  text  ) {
		// the text will be all asterisk, thus we draw them letter by letter

		// set clipping to be within textinput button
		const clip_dimension old_clip = display_get_clip_wh();

		int text_clip_x = pos.x+offset.x + 1, text_clip_w = size.w - 2;
		// something to draw?
		if (text_clip_x >= old_clip.xx || text_clip_x+text_clip_w <= old_clip.x || text_clip_w<=0) {
				return;
		}
		const int clip_x =  old_clip.x > text_clip_x ? old_clip.x : text_clip_x;
		display_set_clip_wh( clip_x, old_clip.y, min(old_clip.xx, text_clip_x+text_clip_w)-clip_x, old_clip.h);

		utf8 const *text_pos = (utf8 const*)text;
		utf8 const *end = (utf8 const*)text + max;
		sint16 xpos = pos.x+offset.x+2;
		utf32  c = 0;
		do {
			// cursor?
			if(  cursor_visible  &&  text_pos == (utf8 const*)text + head_cursor_pos  ) {
				display_fillbox_wh_clip_rgb( xpos, pos.y+offset.y+1+(size.h-LINESPACE)/2, 1, LINESPACE, SYSCOL_CURSOR_BEAM, true);
			}
			c = utf8_decoder_t::decode((utf8 const *&)text_pos);
			if(c) {
				xpos += display_proportional_clip_rgb( xpos, pos.y+offset.y+1+(size.h-LINESPACE)/2, "*", ALIGN_LEFT | DT_CLIP, textcol, true);
			}
		}
		while(  text_pos < end  &&  c != UNICODE_NUL  );

		// reset clipping
		display_set_clip_wh(old_clip.x, old_clip.y, old_clip.w, old_clip.h);
	}
}
