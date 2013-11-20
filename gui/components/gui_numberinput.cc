/*
 * Copyright (c) 2008 Dwachs
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * An input field for integer numbers (with arrow buttons for dec/inc)
 * @author Dwachs 2008
 */

#include "../gui_frame.h"
#include "gui_numberinput.h"
#include "../../gui/simwin.h"
#include "../../display/simgraph.h"
#include "../../macros.h"
#include "../../dataobj/translator.h"

#define ARROW_GAP (1)

char gui_numberinput_t::tooltip[256];

gui_numberinput_t::gui_numberinput_t() :
	gui_komponente_t(true)
{
	bt_left.set_typ(button_t::repeatarrowleft );
	bt_left.add_listener(this );

	textinp.set_alignment( ALIGN_RIGHT );
	textinp.set_color( SYSCOL_TEXT_HIGHLIGHT );
	textinp.add_listener( this );

	bt_right.set_typ(button_t::repeatarrowright );
	bt_right.add_listener(this );

	set_limits(0, 9999);
	textbuffer[0] = 0;	// start with empty buffer
	textinp.set_text(textbuffer, 20);
	set_increment_mode( 1 );
	wrap_mode( true );
	b_enabled = true;

	set_size( scr_size( D_BUTTON_WIDTH, D_EDIT_HEIGHT ) );
}

void gui_numberinput_t::set_size(scr_size size_par) {

	gui_komponente_t::set_size(size_par);

	textinp.set_size( scr_size( size_par.w - bt_left.get_size().w - bt_right.get_size().w, size_par.h) );
	bt_left.align_to(&textinp, ALIGN_CENTER_V);
	textinp.align_to(&bt_left, ALIGN_EXTERIOR_H | ALIGN_LEFT);
	bt_right.align_to(&textinp, ALIGN_CENTER_V | ALIGN_EXTERIOR_H | ALIGN_LEFT);

}


void gui_numberinput_t::set_value(sint32 new_value)
{	// range check
	value = clamp( new_value, min_value, max_value );
	gui_frame_t *win = win_get_top();
	if(  win  &&  win->get_focus()!=this  ) {
		// final value should be correct, but during editing wrong values are allowed
		new_value = value;
	}
	// To preserve cursor position if text was edited, only set new text if changed (or empty before)
	if(  textbuffer[0]<32  ||  new_value != get_text_value()  ) {
		sprintf(textbuffer, "%d", new_value);
		textinp.set_text(textbuffer, 20);
	}
	textinp.set_color( value == new_value ? (b_enabled ? SYSCOL_TEXT_HIGHLIGHT : COL_GREY3) : COL_RED );
	value = new_value;
}


sint32 gui_numberinput_t::get_text_value()
{
	return (sint32)atol( textinp.get_text() );
}


sint32 gui_numberinput_t::get_value()
{
	return clamp( value, min_value, max_value );
}



bool gui_numberinput_t::check_value(sint32 _value)
{
	return (_value >= min_value)  &&  (_value <= max_value);
}


void gui_numberinput_t::set_limits(sint32 _min, sint32 _max)
{
	min_value = _min;
	max_value = _max;
}


bool gui_numberinput_t::action_triggered( gui_action_creator_t *komp, value_t /* */)
{
	if(  komp == &textinp  ) {
		// .. if enter / esc pressed
		set_value( get_text_value() );
		if(check_value(value)) {
			call_listeners(value_t(value));
		}
	}
	else if(  komp == &bt_left  ||  komp == &bt_right  ) {
		// value changed and feasible
		sint32 new_value = (komp == &bt_left) ? get_prev_value() : get_next_value();
		if(  new_value!=value  ) {
			set_value( new_value );
			if(check_value(new_value)) {
				// check for valid change - call listeners
				call_listeners(value_t(value));
			}
		}
	}
	return false;
}


sint8 gui_numberinput_t::percent[7] = { 0, 1, 5, 10, 20, 50, 100 };

sint32 gui_numberinput_t::get_next_value()
{
	if(  value>=max_value  ) {
		// turn over
		return (wrapping  &&  value==max_value) ? min_value : max_value;
	}

	switch( step_mode ) {
		// automatic linear
		case AUTOLINEAR:
		{
			sint64 diff = (sint64)max_value - (sint64)min_value;
			sint32 one_percent = (sint32) (diff / 100l);
			return clamp( value+max(1,one_percent), min_value, max_value );
		}
		// power of 2
		case POWER2:
		{
			sint32 new_value=1;
			for( int i=0;  i<32;  i++  ) {
				if(  value<(new_value<<i)  ) {
					return clamp( (new_value<<i), min_value, max_value );
				}
			}
			return max_value;
		}
		// progressive (used for loading bars)
		case PROGRESS:
		{
			sint64 diff = (sint64)max_value - (sint64)min_value;
			for( int i=0;  i<7;  i++  ) {
				if(  value-min_value < ((diff*(sint64)percent[i])/100l)  ) {
					return min_value+(sint32)((diff*percent[i])/100l);
				}
			}
			return max_value;
		}
		// default value is step size
		default:
			return clamp( ((value+step_mode)/step_mode)*step_mode, min_value, max_value );
	}
}


sint32 gui_numberinput_t::get_prev_value()
{
	if(  value<=min_value  ) {
		// turn over
		return (wrapping  &&  value==min_value) ? max_value : min_value;
	}

	switch( step_mode ) {
		// automatic linear
		case AUTOLINEAR:
		{
			sint64 diff = (sint64)max_value - (sint64)min_value;
			sint32 one_percent = (sint32) (diff / 100ll);
			return clamp( value-max(1,one_percent), min_value, max_value );
		}
		// power of 2
		case POWER2:
		{
			sint32 new_value=1;
			for( int i=30;  i>=0;  i--  ) {
				if(  value>(new_value<<i)  ) {
					return clamp( (new_value<<i), min_value, max_value );
				}
			}
			return min_value;
		}
		// progressive (used for loading bars)
		case PROGRESS:
		{
			sint64 diff = (sint64)max_value-(sint64)min_value;
			for( int i=6;  i>=0;  i--  ) {
				if(  value-min_value > ((diff*percent[i])/100l)  ) {
					return min_value+(sint32)((diff*percent[i])/100l);
				}
			}
			return min_value;
		}
		// default value is step size
		default:
			return clamp( value-step_mode, min_value, max_value );
	}
}


// all init in one ...
void gui_numberinput_t::init( sint32 value, sint32 min, sint32 max, sint32 mode, bool wrap )
{
	set_limits( min, max );
	set_value( value );
	set_increment_mode( mode );
	wrap_mode( wrap );
}


bool gui_numberinput_t::infowin_event(const event_t *ev)
{
	// buttons pressed
	if(  bt_left.getroffen(ev->cx, ev->cy)  &&  ev->ev_code == MOUSE_LEFTBUTTON  ) {
		event_t ev2 = *ev;
		translate_event(&ev2, -bt_left.get_pos().x, -bt_left.get_pos().y);
		return bt_left.infowin_event(&ev2);
	}
	else if(  bt_right.getroffen(ev->cx, ev->cy)  &&  ev->ev_code == MOUSE_LEFTBUTTON  ) {
		event_t ev2 = *ev;
		translate_event(&ev2, -bt_right.get_pos().x, -bt_right.get_pos().y);
		return bt_right.infowin_event(&ev2);
	}
	else {
		// since button have different callback ...
		bool result = false;
		sint32 new_value = value;

		// mouse wheel -> fast increase / decrease
		if(IS_WHEELUP(ev)){
			new_value = get_next_value();
		}
		else if(IS_WHEELDOWN(ev)){
			new_value = get_prev_value();
		}

		// catch non-number keys
		if(  ev->ev_class == EVENT_KEYBOARD  ||  value==new_value  ) {
			// assume false input
			bool call_textinp = ev->ev_class != EVENT_KEYBOARD;
			// editing keys, arrows, hom/end
			switch (ev->ev_code) {
				case '-':
					call_textinp = min_value <0;
					break;
				case 1:		// allow Ctrl-A (select all text) to function
				case 3:		// allow Ctrl-C (copy text to clipboard)
				case 8:
				case 9:		// allow text input to handle unfocus event
				case 22:	// allow Ctrl-V (paste text from clipboard)
				case 24:	// allow Ctrl-X (cut text and copy to clipboard)
				case 127:
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case SIM_KEY_LEFT:
				case SIM_KEY_RIGHT:
				case SIM_KEY_HOME:
				case SIM_KEY_END:
					call_textinp = true;
					break;
				case SIM_KEY_UP:
				case SIM_KEY_DOWN:
						// next/previous choice
						new_value = (ev->ev_code==SIM_KEY_DOWN) ? get_prev_value() : get_next_value();
			}
			if(  call_textinp  ) {
				event_t ev2 = *ev;
				translate_event(&ev2, -textinp.get_pos().x, -textinp.get_pos().y);
				result = textinp.infowin_event(&ev2);
				new_value = get_text_value();
			}
		}

		// value changed and feasible
		if(  new_value!=value  ) {
			set_value( new_value );
			if(check_value(new_value)) {
				// check for valid change - call listeners
				call_listeners(value_t(value));
				result = true;
			}
		}

		return result;
	}

	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_UNTOP  ) {
		// loosing focus ...
		set_value( get_text_value() );
		// just to be sure: call listener (value may be same)
		call_listeners(value_t(value));
	}

	return false;
}


/**
 * Draw the component
 * @author Dwachs
 */
void gui_numberinput_t::draw(scr_coord offset)
{
	scr_coord new_offset = pos+offset;

	bt_left.draw(new_offset);
	textinp.display_with_focus( new_offset, (win_get_focus()==this) );
	bt_right.draw(new_offset);

	if(getroffen( get_maus_x()-offset.x, get_maus_y()-offset.y )) {
		sprintf( tooltip, translator::translate("enter a value between %i and %i"), min_value, max_value );
		win_set_tooltip(get_maus_x() + TOOLTIP_MOUSE_OFFSET_X, new_offset.y + size.h + TOOLTIP_MOUSE_OFFSET_Y, tooltip, this);
	}
}
