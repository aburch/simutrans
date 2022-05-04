/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Defines all button types: Normal (roundbox), Checkboxes (square), Arrows, Scrollbars
 */

#include "gui_button.h"

#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../simevent.h"
#include "../simwin.h"

#include "../../sys/simsys.h"

#include "../../dataobj/translator.h"

#include "../../simskin.h"
#include "../../descriptor/skin_desc.h"
#include "../../utils/simstring.h"

// the following are only needed for the posbutton ...
#include "../../world/simworld.h"
#include "../../ground/grund.h"
#include "../../display/viewport.h"
#include "../../obj/zeiger.h"

#include "../gui_frame.h"

#define STATE_BIT (button_t::state)
#define AUTOMATIC_BIT (button_t::automatic)

#define get_state_offset() (b_enabled ? pressed : 2)


karte_ptr_t button_t::welt;


button_t::button_t() :
	gui_component_t(true)
{
	b_no_translate = false;
	pressed = false;
	translated_tooltip = tooltip = NULL;
	background_color = color_idx_to_rgb(COL_WHITE);
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
	switch (type&TYPE_MASK) {

		case square:
			text_color = SYSCOL_CHECKBOX_TEXT;
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
			set_size( gui_theme_t::gui_pos_button_size );
			break;

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

		case box:
			text_color = SYSCOL_COLORED_BUTTON_TEXT;
			/* FALLTHROUGH */
		case roundbox:
			set_size( scr_size(gui_theme_t::gui_button_size.w, max(D_BUTTON_HEIGHT,LINESPACE)) );
			break;

		case imagebox:
			img = IMG_EMPTY;
			break;

		case sortarrow:
		{
			const uint8 block_height = 2;
			const uint8 bars_height = uint8((size.h-block_height-4)/4) * block_height*2 + block_height;
			set_size( scr_size(max(D_BUTTON_HEIGHT, (gui_theme_t::gui_color_button_text_offset.w+4)*2 + 6/*arrow width(5)+margin(1)*/+block_height + (bars_height-2)/2), max(D_BUTTON_HEIGHT, LINESPACE)) );
			b_no_translate = false;
			break;
		}

		default:
			break;
	}
	update_focusability();
}


void button_t::set_targetpos( const koord k )
{
	targetpos.x = k.x;
	targetpos.y = k.y;
	targetpos.z = welt->max_hgt( k );
}


scr_size button_t::get_max_size() const
{
	switch(type&TYPE_MASK) {
		case square:
		case box:
		case roundbox:
			return (type & flexible) ? scr_size(scr_size::inf.w, get_min_size().h) : get_min_size();

		default:
			return get_min_size();
	}
}

scr_size button_t::get_min_size() const
{

	switch (type&TYPE_MASK) {
		case arrowleft:
		case repeatarrowleft:
			return gui_theme_t::gui_arrow_left_size;

		case posbutton:
			return gui_theme_t::gui_pos_button_size;

		case arrowright:
		case repeatarrowright:
			return gui_theme_t::gui_arrow_right_size;

		case arrowup:
			return gui_theme_t::gui_arrow_up_size;

		case arrowdown:
			return gui_theme_t::gui_arrow_down_size;

		case square: {
			scr_coord_val w = translated_text ?  D_H_SPACE + proportional_string_width( translated_text ) : 0;
			return scr_size(w + gui_theme_t::gui_checkbox_size.w, max(gui_theme_t::gui_checkbox_size.h,LINESPACE));
		}
		case box:
		case roundbox: {
			scr_coord_val w = translated_text ?  2*D_H_SPACE + proportional_string_width( translated_text ) : 0;
			scr_size size(gui_theme_t::gui_button_size.w, max(D_BUTTON_HEIGHT,LINESPACE));
			size.w = max(size.w, w);
			return size;
		}

		case imagebox: {
			scr_coord_val x = 0, y = 0, w = 0, h = 0;
			display_get_image_offset(img, &x, &y, &w, &h);
			scr_size size(gui_theme_t::gui_pos_button_size);
			size.w = max(size.w, w+2);
			size.h = max(size.h, h+2);
			return size;
		}

		case sortarrow:
		{
			const uint8 block_height = 2;
			const uint8 bars_height = uint8((size.h-block_height-4)/4) * block_height*2 + block_height;
			return scr_size( max( D_BUTTON_HEIGHT, (gui_theme_t::gui_color_button_text_offset.w+4)*2 + 6/*arrow width(5)+margin(1)*/+block_height + (bars_height-2)/2 ), max(D_BUTTON_HEIGHT, LINESPACE) );
		}

		default:
			return gui_component_t::get_min_size();
	}
}

/**
 * Sets the text displayed in the button
 */
void button_t::set_text(const char * text)
{
	this->text = text;
	translated_text = b_no_translate ? text : translator::translate(text);

	if(  (type & TYPE_MASK) == square  &&  !strempty(translated_text)  ) {
		set_size( scr_size( gui_theme_t::gui_checkbox_size.w + D_H_SPACE + proportional_string_width( translated_text ), max(gui_theme_t::gui_checkbox_size.h, LINESPACE)) );
	}
}


/**
 * Sets the tooltip of this button
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
	bool hit=gui_component_t::getroffen(x, y);
	if(  pressed  &&  !hit  &&  ( (type & STATE_BIT) == 0)  ) {
		// moved away
		pressed = 0;
	}
	return hit;
}


/**
 * Event responder
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

	// we ignore resize events, they shouldn't make us pressed or unpressed
	if(!b_enabled  ||  IS_WINDOW_RESIZE(ev)) {
		return false;
	}

	// check if the initial click and the current mouse positions are within the button's boundary
	bool const cxy_within_boundary = 0 <= ev->cx && ev->cx < get_size().w && 0 <= ev->cy && ev->cy <= get_size().h;
	bool const mxy_within_boundary = 0 <= ev->mx && ev->mx < get_size().w && 0 <= ev->my && ev->my <= get_size().h;

	// update the button pressed state only when mouse positions are within boundary or when it is mouse release
	if(  (type & STATE_BIT) == 0  &&  cxy_within_boundary  &&  (  mxy_within_boundary  ||  IS_LEFTRELEASE(ev)  )  ) {
		pressed = (ev->button_state==1);
	}

	// make sure that the button will take effect only when the mouse positions are within the component's boundary
	if(  !cxy_within_boundary  ||  !mxy_within_boundary  ) {
		return false;
	}

	if(IS_LEFTRELEASE(ev)) {
		if(  (type & TYPE_MASK)==posbutton  ) {
			call_listeners( &targetpos );
			if (type == posbutton_automatic) {
				welt->get_viewport()->change_world_position( targetpos );
				welt->get_zeiger()->change_pos( targetpos );
			}
			return true;
		}
		else {
			if(  type & AUTOMATIC_BIT  ) {
				pressed = !pressed;
			}
			call_listeners( (long)0 );
			return true;
		}
	}
	else if(  (type & TYPE_MASK) >= repeatarrowleft  &&  ev->button_state==1  ) {
		uint32 cur_time = dr_time();
		if (IS_LEFTCLICK(ev)  ||  button_click_time==0) {
			button_click_time = cur_time;
		}
		else if(cur_time - button_click_time > 100) {
			// call listerner every 100 ms
			call_listeners( (long)1 );
			button_click_time = cur_time;
			return true;
		}
	}
	return false;
}


void button_t::draw_focus_rect( scr_rect r, scr_coord_val offset) {

	scr_coord_val w = ((offset-1)<<1);

	display_fillbox_wh_clip_rgb(r.x-offset+1,     r.y-1-offset+1,   r.w+w, 1, color_idx_to_rgb(COL_WHITE), false);
	display_fillbox_wh_clip_rgb(r.x-offset+1,     r.y+r.h+offset-1, r.w+w, 1, color_idx_to_rgb(COL_WHITE), false);
	display_vline_wh_clip_rgb  (r.x-offset,       r.y-offset+1,     r.h+w,    color_idx_to_rgb(COL_WHITE), false);
	display_vline_wh_clip_rgb  (r.x+r.w+offset-1, r.y-offset+1,     r.h+w,    color_idx_to_rgb(COL_WHITE), false);
}


// draw button. x,y is top left of window.
void button_t::draw(scr_coord offset)
{
	if(  !is_visible()  ) {
		return;
	}

	const scr_rect area( offset+pos, size );
	PIXVAL text_color = pressed ? SYSCOL_BUTTON_TEXT_SELECTED : this->text_color;
	text_color = b_enabled ? text_color : SYSCOL_BUTTON_TEXT_DISABLED;

	switch (type&TYPE_MASK) {

		case box: // Colored background box
			{
				display_img_stretch( gui_theme_t::button_tiles[get_state_offset()], area );
				display_img_stretch_blend( gui_theme_t::button_color_tiles[b_enabled && pressed], area, background_color | TRANSPARENT75_FLAG | OUTLINE_FLAG );
				if(  text  ) {
					text_color = pressed ? SYSCOL_COLORED_BUTTON_TEXT_SELECTED : text_color;
					// move the text to leave evt. space for a colored box top left or bottom right of it
					scr_rect area_text = area - gui_theme_t::gui_color_button_text_offset_right;
					area_text.set_pos( gui_theme_t::gui_color_button_text_offset + area.get_pos() );
					display_proportional_ellipsis_rgb( area_text, translated_text, ALIGN_CENTER_H | ALIGN_CENTER_V | DT_CLIP, text_color, true );
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
					// move the text to leave evt. space for a colored box top left or bottom right of it
					scr_rect area_text = area - gui_theme_t::gui_button_text_offset_right;
					area_text.set_pos( gui_theme_t::gui_button_text_offset + area.get_pos() );
					display_proportional_ellipsis_rgb( area_text, translated_text, ALIGN_CENTER_H | ALIGN_CENTER_V | DT_CLIP, text_color, true );
				}
				if(  win_get_focus()==this  ) {
					draw_focus_rect( area );
				}
			}
			break;

		case imagebox:
			display_img_stretch(gui_theme_t::button_tiles[get_state_offset()], area);
			display_img_stretch_blend(gui_theme_t::button_color_tiles[b_enabled && pressed], area, (pressed ? text_color: background_color) | TRANSPARENT75_FLAG | OUTLINE_FLAG);
			display_img_aligned(img, area, ALIGN_CENTER_H | ALIGN_CENTER_V, true);
			if (win_get_focus() == this) {
				draw_focus_rect(area);
			}
			break;

		case sortarrow:
			{
				display_img_stretch(gui_theme_t::button_tiles[0], area);

				const uint8 block_height = 2;
				const uint8 bars_height = uint8((size.h-block_height-4)/4)*block_height*2 + block_height;
				scr_rect area_drawing(area.x, area.y, 6/*arrow width(5)+margin(1)*/+block_height+(bars_height-2)/2, bars_height);
				area_drawing.set_pos(gui_theme_t::gui_color_button_text_offset + area.get_pos() + scr_coord(4/*left margin*/,D_GET_CENTER_ALIGN_OFFSET(bars_height,size.h)));

				// draw an arrow
				display_fillbox_wh_clip_rgb(area_drawing.x+2, area_drawing.y, 1, bars_height, SYSCOL_BUTTON_TEXT, false);
				if (pressed) {
					// desc
					display_fillbox_wh_clip_rgb(area_drawing.x+1, area_drawing.y+1, 3, 1, SYSCOL_BUTTON_TEXT, false);
					display_fillbox_wh_clip_rgb(area_drawing.x,   area_drawing.y+2, 5, 1, SYSCOL_BUTTON_TEXT, false);
					for (uint8 row=0; row*4<bars_height; row++) {
						display_fillbox_wh_clip_rgb(area_drawing.x + 6/*arrow width(5)+margin(1)*/, area_drawing.y + bars_height - block_height - row*block_height*2, block_height*(row+1), block_height, SYSCOL_BUTTON_TEXT, false);
					}
					tooltip = "hl_btn_sort_desc";
				}
				else {
					// asc
					display_fillbox_wh_clip_rgb(area_drawing.x+1, area_drawing.y+bars_height-2, 3, 1, SYSCOL_BUTTON_TEXT, false);
					display_fillbox_wh_clip_rgb(area_drawing.x,   area_drawing.y+bars_height-3, 5, 1, SYSCOL_BUTTON_TEXT, false);
					for (uint8 row=0; row*4<bars_height; row++) {
						display_fillbox_wh_clip_rgb(area_drawing.x + 6/*arrow width(5)+margin(1)*/, area_drawing.y + row*block_height*2, block_height*(row+1), block_height, SYSCOL_BUTTON_TEXT, false);
					}
					tooltip = "hl_btn_sort_asc";
				}

				if(  getroffen(get_mouse_x() - offset.x, get_mouse_y() - offset.y)  ) {
					translated_tooltip = translator::translate(tooltip);
				}
			}
			break;

		case square: // checkbox with text
			{
				display_img_aligned( gui_theme_t::check_button_img[ get_state_offset() ], area, ALIGN_CENTER_V, true );
				if(  text  ) {
					text_color = b_enabled ? this->text_color : SYSCOL_CHECKBOX_TEXT_DISABLED;
					scr_rect area_text = area;
					area_text.x += gui_theme_t::gui_checkbox_size.w + D_H_SPACE;
					area_text.w -= gui_theme_t::gui_checkbox_size.w + D_H_SPACE;
					display_proportional_ellipsis_rgb( area_text, translated_text, ALIGN_LEFT | ALIGN_CENTER_V | DT_CLIP, text_color, true );
				}
				if(  win_get_focus() == this  ) {
					draw_focus_rect( scr_rect( area.get_pos()+scr_coord(0,(area.get_size().h-gui_theme_t::gui_checkbox_size.w)/2), gui_theme_t::gui_checkbox_size ) );
				}
			}
			break;

		case posbutton:
			{
				uint8 offset = get_state_offset();
				if(  offset == 0  ) {
					if(  grund_t *gr = welt->lookup_kartenboden(targetpos.get_2d())  ) {
						offset = welt->get_viewport()->is_on_center( gr->get_pos() );
					}
				}
				display_img_aligned( gui_theme_t::pos_button_img[ offset ], area, ALIGN_CENTER_H|ALIGN_CENTER_V, true );
			}
			break;

		case arrowleft:
		case repeatarrowleft:
			display_img_aligned( gui_theme_t::arrow_button_left_img[ get_state_offset() ], area, ALIGN_CENTER_H|ALIGN_CENTER_V, true );
			break;

		case arrowright:
		case repeatarrowright:
			display_img_aligned( gui_theme_t::arrow_button_right_img[ get_state_offset() ], area, ALIGN_CENTER_H|ALIGN_CENTER_V, true );
			break;

		case arrowup:
			display_img_aligned( gui_theme_t::arrow_button_up_img[ get_state_offset() ], area, ALIGN_CENTER_H|ALIGN_CENTER_V, true );
			break;

		case arrowdown:
			display_img_aligned( gui_theme_t::arrow_button_down_img[ get_state_offset() ], area, ALIGN_CENTER_H|ALIGN_CENTER_V, true );
			break;

		default: ;
	}

	if(  translated_tooltip  &&  getroffen( get_mouse_x()-offset.x, get_mouse_y()-offset.y )  ) {
		win_set_tooltip( get_mouse_x() + TOOLTIP_MOUSE_OFFSET_X, area.get_bottom() + TOOLTIP_MOUSE_OFFSET_Y, translated_tooltip, this);
	}
}


void button_t::update_focusability()
{
	switch (type&TYPE_MASK) {

		case box:      // Old, 4-line box
		case roundbox: // New box with round corners
		case square:   // Little square in front of text (checkbox)
			set_focusable(true);
			break;

		// those cannot receive focus ...
		case imagebox:
		case sortarrow:
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
