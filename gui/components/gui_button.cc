/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Defines all button types: Normal (roundbox), Checkboxes (square), Arrows, Scrollbars
 */

#include "gui_button.h"

#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../simevent.h"
#include "../../gui/simwin.h"

#include "../../dataobj/translator.h"

#include "../../simskin.h"
#include "../../besch/skin_besch.h"
#include "../../utils/simstring.h"

#include "../gui_frame.h"

#define STATE_MASK (127)
#define AUTOMATIC_MASK (255)

#define get_state_offset() (b_enabled ? pressed : 2)

// default button codes
#define SQUARE_BUTTON 0
#define POS_BUTTON 1
#define ARROW_LEFT 2
#define ARROW_RIGHT 3
#define ARROW_UP 4
#define ARROW_DOWN 5
#define SCROLL_BAR 6

// colors
#define RB_LEFT_BUTTON (201)
#define RB_BODY_BUTTON (202)
#define RB_RIGHT_BUTTON (203)



button_t::button_t() :
	gui_komponente_t(true)
{
	b_no_translate = false;
	pressed = false;
	translated_tooltip = tooltip = NULL;
	background_color = SYSCOL_FACE;
	b_enabled = true;

	// By default a box button
	init(box,"");
}


void button_t::init(enum type type_par, const char *text_par, scr_coord pos_par, scr_size size_par)
{
	translated_tooltip = NULL;
	tooltip = NULL;
	b_no_translate = ( type_par==posbutton );

	set_typ(type_par);
	set_text(text_par);
	set_pos(pos_par);
	if(  size_par != scr_size::invalid  ) {
		set_size(size_par);
	}
}


// set type. this includes size for specified buttons.
void button_t::set_typ(enum type t)
{
	type = t;
	text_color = SYSCOL_BUTTON_TEXT;
	switch (type&STATE_MASK) {

		case square:
			text_color = SYSCOL_BUTTON_TEXT;
			if(  !strempty(translated_text)  ) {
				set_text(translated_text);
				set_size( scr_size( gui_theme_t::gui_checkbox_size.w + D_H_SPACE + proportional_string_width( translated_text ), max(gui_theme_t::gui_checkbox_size.h,LINESPACE)) );
			}
			else {
				set_size( scr_size( gui_theme_t::gui_checkbox_size.w, max(gui_theme_t::gui_checkbox_size.h,LINESPACE)) );
			}
			break;

		case arrowleft:
		case repeatarrowleft:
			set_size( gui_theme_t::gui_arrow_left_size );
			break;

		case posbutton:
			set_no_translate( true );
			// fallthrough
		case arrowright:
		case repeatarrowright:
			set_size( gui_theme_t::gui_arrow_right_size );
			break;

		case arrowup:
			set_size( gui_theme_t::gui_arrow_up_size );
			break;

		case arrowdown:
			set_size( gui_theme_t::gui_arrow_down_size );
			break;

		case roundbox:
		case box:
			set_size( scr_size(gui_theme_t::gui_button_size.w, max(D_BUTTON_HEIGHT,LINESPACE)) );
			break;

		default:
			break;
	}
	update_focusability();
}


/**
 * Sets the text displayed in the button
 * @author Hj. Malthaner
 */
void button_t::set_text(const char * text)
{
	this->text = text;
	translated_text = b_no_translate ? text : translator::translate(text);

	if(  (type & STATE_MASK) == square  &&  !strempty(translated_text)  ) {
		set_size( scr_size( gui_theme_t::gui_checkbox_size.w + D_H_SPACE + proportional_string_width( translated_text ), max(gui_theme_t::gui_checkbox_size.h, LINESPACE)) );
	}
}


/**
 * Sets the tooltip of this button
 * @author Hj. Malthaner
 */
void button_t::set_tooltip(const char * t)
{
	if(  t == NULL  ) {
		translated_tooltip = tooltip = NULL;
	}
	else {
		tooltip = t;
		translated_tooltip = b_no_translate ? tooltip : translator::translate(tooltip);
	}
}


bool button_t::getroffen(int x,int y)
{
	bool hit=gui_komponente_t::getroffen(x, y);
	if(  pressed  &&  !hit  &&  type <= STATE_MASK  ) {
		// moved away
		pressed = 0;
	}
	return hit;
}


/**
 * Event responder
 * @author Hj. Malthaner
 */
bool button_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_OPEN  ) {
		if(text) {
			translated_text = b_no_translate ? text : translator::translate(text);
		}
		if(tooltip) {
			translated_tooltip = b_no_translate ? tooltip : translator::translate(tooltip);
		}
	}

	if(  ev->ev_class==EVENT_KEYBOARD  ) {
		if(  ev->ev_code==32  &&  get_focus()  ) {
			// space toggles button
			call_listeners( (long)0 );
			return true;
		}
		return false;
	}

	// Hajo: we ignore resize events, they shouldn't make us
	// pressed or unpressed
	if(!b_enabled  ||  IS_WINDOW_RESIZE(ev)) {
		return false;
	}

	// Knightly : check if the initial click and the current mouse positions are within the button's boundary
	bool const cxy_within_boundary = 0 <= ev->cx && ev->cx < get_size().w && 0 <= ev->cy && ev->cy < get_size().h;
	bool const mxy_within_boundary = 0 <= ev->mx && ev->mx < get_size().w && 0 <= ev->my && ev->my < get_size().h;

	// Knightly : update the button pressed state only when mouse positions are within boundary or when it is mouse release
	if(  type<=STATE_MASK  &&  cxy_within_boundary  &&  (  mxy_within_boundary  ||  IS_LEFTRELEASE(ev)  )  ) {
		// Hajo: check button state, if we should look depressed
		pressed = (ev->button_state==1);
	}

	// Knightly : make sure that the button will take effect only when the mouse positions are within the component's boundary
	if(  !cxy_within_boundary  ||  !mxy_within_boundary  ) {
		return false;
	}

	if(  type>AUTOMATIC_MASK  &&  IS_LEFTCLICK(ev)  ) {
		pressed = !pressed;
	}

	if(IS_LEFTRELEASE(ev)) {
		if(  type==posbutton  ) {
			koord k(targetpos.x,targetpos.y);
			call_listeners( &k );
		}
		else {
			call_listeners( (long)0 );
		}
	}
	else if(IS_LEFTREPEAT(ev)) {
		if((type&STATE_MASK)>=repeatarrowleft) {
			call_listeners( (long)1 );
		}
	}
	// swallow all not handled non-keyboard events
	return (ev->ev_class != EVENT_KEYBOARD);
}


void button_t::draw_focus_rect( scr_rect r, scr_coord_val offset) {

	scr_coord_val w = ((offset-1)<<1);

	display_fillbox_wh_clip(r.x-offset+1,     r.y-1-offset+1,    r.w+w, 1, COL_WHITE, false);
	display_fillbox_wh_clip(r.x-offset+1,     r.y+r.h+offset-1,  r.w+w, 1, COL_WHITE, false);
	display_vline_wh_clip  (r.x-offset,       r.y-offset+1,      r.h+w,    COL_WHITE, false);
	display_vline_wh_clip  (r.x+r.w+offset-1, r.y-offset+1,      r.h+w,    COL_WHITE, false);
}


// draw button. x,y is top left of window.
void button_t::draw(scr_coord offset)
{
	if(  !is_visible()  ) {
		return;
	}

	const scr_rect area( offset+pos, size );
	const COLOR_VAL text_color = b_enabled ? this->text_color : SYSCOL_DISABLED_BUTTON_TEXT;

	switch (type&STATE_MASK) {

		case box: // Colored background box
			{
				display_img_stretch( gui_theme_t::button_tiles[get_state_offset()], area );
				display_img_stretch_blend( gui_theme_t::button_color_tiles[b_enabled && pressed], area, background_color | TRANSPARENT75_FLAG | OUTLINE_FLAG );
				if(  text  ) {
					// move the text to leave evt. space for a colored box top or left of it
					scr_rect area_text = area;
					area_text.set_pos( (scr_coord)gui_theme_t::gui_button_text_offset + area.get_pos() );
					display_proportional_ellipse( area_text, translated_text, ALIGN_CENTER_H | ALIGN_CENTER_V | DT_CLIP, text_color, true );
				}
				if(  win_get_focus()==this  ) {
					draw_focus_rect( area );
				}
			}
			break;

		case roundbox: // button with inside text
			{
				display_img_stretch( gui_theme_t::round_button_tiles[get_state_offset()], area );
				if(  text  ) {
					display_proportional_ellipse( area, translated_text, ALIGN_CENTER_H | ALIGN_CENTER_V | DT_CLIP, text_color, true );
				}
				if(  win_get_focus()==this  ) {
					draw_focus_rect( area );
				}
			}
			break;

		case square: // checkbox with text
			{
				display_img_aligned( gui_theme_t::check_button_img[ get_state_offset() ], area, ALIGN_CENTER_V, true );
				if(  text  ) {
					scr_rect area_text = area;
					area_text.x += gui_theme_t::gui_checkbox_size.w + D_H_SPACE;
					display_proportional_ellipse( area_text, translated_text, ALIGN_LEFT | ALIGN_CENTER_V | DT_CLIP, text_color, true );
				}
				if(  win_get_focus() == this  ) {
					draw_focus_rect( scr_rect( area.get_pos()+scr_coord(0,(area.get_size().h-gui_theme_t::gui_checkbox_size.w)/2), gui_theme_t::gui_checkbox_size ) );
				}
			}
			break;

		case posbutton:
			display_img_aligned( gui_theme_t::pos_button_img[ get_state_offset() ], area, ALIGN_CENTER_V, true );
			break;

		case arrowleft:
		case repeatarrowleft:
			display_img_aligned( gui_theme_t::arrow_button_left_img[ get_state_offset() ], area, ALIGN_CENTER_V, true );
			break;

		case arrowright:
		case repeatarrowright:
			display_img_aligned( gui_theme_t::arrow_button_right_img[ get_state_offset() ], area, ALIGN_CENTER_V, true );
			break;

		case arrowup:
			display_img_aligned( gui_theme_t::arrow_button_up_img[ get_state_offset() ], area, ALIGN_CENTER_H, true );
			break;

		case arrowdown:
			display_img_aligned( gui_theme_t::arrow_button_down_img[ get_state_offset() ], area, ALIGN_CENTER_H, true );
			break;
	}

	if(  translated_tooltip  &&  getroffen( get_maus_x()-offset.x, get_maus_y()-offset.y )  ) {
		win_set_tooltip( get_maus_x() + TOOLTIP_MOUSE_OFFSET_X, area.get_bottom() + TOOLTIP_MOUSE_OFFSET_Y, translated_tooltip, this);
	}
}


void button_t::update_focusability()
{
	switch (type&STATE_MASK) {

		case box:      // Old, 4-line box
		case roundbox: // New box with round corners
		case square:   // Little square in front of text (checkbox)
			set_focusable(true);
			break;

		// those cannot receive focus ...
		case arrowleft:
		case repeatarrowleft:
		case arrowright:
		case repeatarrowright:
		case posbutton:
		case arrowup:
		case arrowdown:
		default:
			set_focusable(false);
			break;
	}
}
