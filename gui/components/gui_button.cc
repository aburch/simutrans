/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "gui_button.h"

#include "../../simdebug.h"
#include "../../simcolor.h"
#include "../../simgraph.h"
#include "../../simevent.h"
#include "../../simwin.h"

#include "../../besch/reader/obj_reader.h"

#include "../../dataobj/translator.h"

#include "../../simskin.h"
#include "../../besch/skin_besch.h"

#include "../../ifc/gui_fenster.h"

#define STATE_MASK (127)
#define AUTOMATIC_MASK (255)
static const char *empty="";

// default button codes
#define SQUARE_BUTTON 0
#define ARROW_LEFT 1
#define ARROW_RIGHT 2
#define ARROW_UP 3
#define ARROW_DOWN 4
#define SCROLL_BAR 5

// colors
#define RB_LEFT_BUTTON (201)
#define RB_BODY_BUTTON (202)
#define RB_RIGHT_BUTTON (203)


/*
 * Hajo: image numbers of button skins
 */
image_id button_t::square_button_pushed = IMG_LEER;
image_id button_t::square_button_normal = IMG_LEER;
image_id button_t::arrow_left_pushed = IMG_LEER;
image_id button_t::arrow_left_normal = IMG_LEER;
image_id button_t::arrow_right_pushed = IMG_LEER;
image_id button_t::arrow_right_normal = IMG_LEER;
image_id button_t::arrow_up_pushed = IMG_LEER;
image_id button_t::arrow_up_normal = IMG_LEER;
image_id button_t::arrow_down_pushed = IMG_LEER;
image_id button_t::arrow_down_normal = IMG_LEER;

// these are optional: buttons made out of graphics
image_id button_t::b_cap_left = IMG_LEER;
image_id button_t::b_body = IMG_LEER;
image_id button_t::b_cap_right = IMG_LEER;

image_id button_t::b_cap_left_p = IMG_LEER;
image_id button_t::b_body_p = IMG_LEER;
image_id button_t::b_cap_right_p = IMG_LEER;

// these are optional: scrollbar ids
image_id button_t::scrollbar_left = IMG_LEER;
image_id button_t::scrollbar_right = IMG_LEER;
image_id button_t::scrollbar_middle = IMG_LEER;

image_id button_t::scrollbar_slider_left = IMG_LEER;
image_id button_t::scrollbar_slider_right = IMG_LEER;
image_id button_t::scrollbar_slider_middle = IMG_LEER;

// these are optional: ... and scrollbars vertical
image_id button_t::scrollbar_top = IMG_LEER;
image_id button_t::scrollbar_bottom = IMG_LEER;
image_id button_t::scrollbar_center = IMG_LEER;

image_id button_t::scrollbar_slider_top = IMG_LEER;
image_id button_t::scrollbar_slider_bottom = IMG_LEER;
image_id button_t::scrollbar_slider_center = IMG_LEER;


/**
 * Lazy button image number init
 * @author Hj. Malthaner
 */
void button_t::init_button_images()
{
	if(skinverwaltung_t::window_skin!=NULL) {

		square_button_normal = skinverwaltung_t::window_skin->get_bild_nr(6);
		square_button_pushed = skinverwaltung_t::window_skin->get_bild_nr(7);

		arrow_left_normal = skinverwaltung_t::window_skin->get_bild_nr(8);
		arrow_left_pushed = skinverwaltung_t::window_skin->get_bild_nr(9);

		arrow_right_normal = skinverwaltung_t::window_skin->get_bild_nr(10);
		arrow_right_pushed = skinverwaltung_t::window_skin->get_bild_nr(11);

		b_cap_left = skinverwaltung_t::window_skin->get_bild_nr(12);
		b_cap_right = skinverwaltung_t::window_skin->get_bild_nr(13);
		b_body = skinverwaltung_t::window_skin->get_bild_nr(14);

		b_cap_left_p = skinverwaltung_t::window_skin->get_bild_nr(15);
		b_cap_right_p = skinverwaltung_t::window_skin->get_bild_nr(16);
		b_body_p = skinverwaltung_t::window_skin->get_bild_nr(17);

		arrow_up_normal = skinverwaltung_t::window_skin->get_bild_nr(18);
		arrow_up_pushed = skinverwaltung_t::window_skin->get_bild_nr(19);

		arrow_down_normal = skinverwaltung_t::window_skin->get_bild_nr(20);
		arrow_down_pushed = skinverwaltung_t::window_skin->get_bild_nr(21);

		// scrollbars
		scrollbar_left = skinverwaltung_t::window_skin->get_bild_nr(24);
		scrollbar_right = skinverwaltung_t::window_skin->get_bild_nr(25);
		scrollbar_middle = skinverwaltung_t::window_skin->get_bild_nr(26);

		scrollbar_slider_left = skinverwaltung_t::window_skin->get_bild_nr(27);
		scrollbar_slider_right = skinverwaltung_t::window_skin->get_bild_nr(28);
		scrollbar_slider_middle = skinverwaltung_t::window_skin->get_bild_nr(29);

		scrollbar_top = skinverwaltung_t::window_skin->get_bild_nr(30);
		scrollbar_bottom = skinverwaltung_t::window_skin->get_bild_nr(31);
		scrollbar_center = skinverwaltung_t::window_skin->get_bild_nr(32);

		scrollbar_slider_top = skinverwaltung_t::window_skin->get_bild_nr(33);
		scrollbar_slider_bottom = skinverwaltung_t::window_skin->get_bild_nr(34);
		scrollbar_slider_center = skinverwaltung_t::window_skin->get_bild_nr(35);
	}
}


/**
 * Displays the different button types
 * @author Hj. Malthaner
 */
void button_t::display_button_image(sint16 x, sint16 y, int number, bool pushed) const
{
	image_id button = IMG_LEER;

	switch (number) {
		case SQUARE_BUTTON:
			button = pushed ? square_button_pushed : square_button_normal;
			break;
		case ARROW_LEFT:
			button = pushed ? arrow_left_pushed : arrow_left_normal;
			break;
		case ARROW_RIGHT:
			button = pushed ? arrow_right_pushed : arrow_right_normal;
			break;
		case ARROW_UP:
			button = pushed ? arrow_up_pushed : arrow_up_normal;
			break;
		case ARROW_DOWN:
			button = pushed ? arrow_down_pushed : arrow_down_normal;
			break;
		case RB_LEFT_BUTTON:
			button = pushed ? b_cap_left_p : b_cap_left;
			break;
		case RB_BODY_BUTTON:
			button = pushed ? b_body_p : b_body;
			break;
		case RB_RIGHT_BUTTON:
			button = pushed ? b_cap_right_p : b_cap_right;
			break;
	}

	display_color_img(button, x, y, 0, false, true);
}



// draw a rectangular button
void button_t::draw_roundbutton(sint16 x, sint16 y, sint16 w, sint16 h, bool pressed)
{
	if(b_cap_left!=IMG_LEER  &&  h==14) {
		const sint16 lw = skinverwaltung_t::window_skin->get_bild(12)->get_pic()->w;
		const sint16 rw = skinverwaltung_t::window_skin->get_bild(13)->get_pic()->w;
		// first the center (may need extra clipping)
		if(w-lw-rw<64) {
			struct clip_dimension cl=display_get_clip_wh();
			display_set_clip_wh(cl.x, cl.y, max(0,min(x+w-rw,cl.xx)-cl.x), cl.h );
			display_button_image(x+lw, y, RB_BODY_BUTTON, pressed);
			display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
		}
		else {
			// wider buttons
			for( sint16 j=0;  j+64<w-rw-lw;  j+=64) {
				display_button_image(x+j+lw, y, RB_BODY_BUTTON, pressed);
			}
			display_button_image(x+w-rw-64, y, RB_BODY_BUTTON, pressed);
		}
		// now the begin and end ...
		display_button_image(x, y, RB_LEFT_BUTTON, pressed);
		display_button_image(x+w-rw, y, RB_RIGHT_BUTTON, pressed);
	}
	else {
		// draw the button conventionally from boxes
		// fallback, if nothing defined
		if (pressed) {
			display_fillbox_wh_clip(x, y, w, 1, MN_GREY1, true);
			display_fillbox_wh_clip(x+1, y+1, w-2, 1, COL_BLACK, true);
			display_fillbox_wh_clip(x+2, y+2, w-2, h-4, MN_GREY1, true);
			display_fillbox_wh_clip(x, y+h-2, w, 1, MN_GREY3, true);
			display_fillbox_wh_clip(x, y+h-1, w, 1, COL_WHITE, true);
			display_vline_wh_clip(x+w-2, y+1, h-2, MN_GREY4, true);
			display_vline_wh_clip(x+w-1, y+1, h-1, COL_WHITE, true);
			display_vline_wh_clip(x, y, h, MN_GREY1, true);
			display_vline_wh_clip(x+1, y+1, h-2, COL_BLACK, true);
		}
		else {
			display_fillbox_wh_clip(x, y, w, 1, COL_WHITE, true);
			display_fillbox_wh_clip(x+1, y+1, w-2, 1, MN_GREY4, true);
			display_fillbox_wh_clip(x+2, y+2, w-2, h-4, MN_GREY3, true);
			display_fillbox_wh_clip(x, y+h-2, w, 1, MN_GREY1, true);
			display_fillbox_wh_clip(x, y+h-1, w, 1, COL_BLACK, true);
			display_vline_wh_clip(x+w-2, y+1, h-2, MN_GREY1, true);
			display_vline_wh_clip(x+w-1, y+1, h-1, COL_BLACK, true);
			display_vline_wh_clip(x, y, h, COL_WHITE, true);
			display_vline_wh_clip(x+1, y+1, h-2, MN_GREY4, true);
		}
	}
}



void button_t::draw_scrollbar(sint16 x, sint16 y, sint16 w, sint16 h, bool horizontal, bool slider)
{
	if(  scrollbar_middle!=IMG_LEER  ) {
		if(  horizontal  ) {
			const int image_offset = 24 + (slider ? 3 : 0);
			const sint16 lw = skinverwaltung_t::window_skin->get_bild(image_offset)->get_pic()->w;
			const sint16 rw = skinverwaltung_t::window_skin->get_bild(image_offset+1)->get_pic()->w;
			// first the center (may need extra clipping)
			if(w-lw-rw<64) {
				struct clip_dimension cl=display_get_clip_wh();
				display_set_clip_wh(cl.x, cl.y, max(0,min(x+w-rw,cl.xx)-cl.x), cl.h );
				display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x+lw, y, 0, false, true);
				display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
			}
			else {
				// wider buttons
				for( sint16 j=0;  j+64<w-rw-lw;  j+=64) {
					display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x+j+lw, y, 0, false, true);
				}
				display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x+w-rw-64, y, 0, false, true);
			}
			// now the begin and end ...
			display_color_img( skinverwaltung_t::window_skin->get_bild_nr(image_offset+0), x, y, 0, false, true);
			display_color_img( skinverwaltung_t::window_skin->get_bild_nr(image_offset+1), x+w-rw, y, 0, false, true);
		}
		else {
			// vertical bar ...
			const int image_offset = 30 + (slider ? 3 : 0);
			const sint16 lh = skinverwaltung_t::window_skin->get_bild(image_offset)->get_pic()->h;
			const sint16 rh = skinverwaltung_t::window_skin->get_bild(image_offset+1)->get_pic()->h;
			// first the center (may need extra clipping)
			if(h-lh-rh<64) {
				struct clip_dimension cl=display_get_clip_wh();
				display_set_clip_wh(cl.x, cl.y, cl.xx, max(0,min(y+h-rh,cl.yy)-cl.y) );
				display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x, y+lh, 0, false, true);
				display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
			}
			else {
				// wider buttons
				for( sint16 j=0;  j+64<h-rh-lh;  j+=64) {
					display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x, y+lh+j, 0, false, true);
				}
				display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x, y+h-rh-64, 0, false, true);
			}
			// now the begin and end ...
			display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+0), x, y, 0, false, true);
			display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+1), x, y+h-rh, 0, false, true);
		}
	}
}

button_t::button_t()
{
	b_no_translate = false;
	translated_text = text = empty;
	pressed = false;
	type = box;
	foreground = COL_BLACK;
	translated_tooltip = tooltip = NULL;
	background = MN_GREY3;
	b_enabled = true;
}


void button_t::init(enum type typ, const char *text, koord pos, koord size)
{
	b_no_translate = typ==posbutton;
	set_typ(typ);
	set_text(text);
	set_pos(pos);
	translated_tooltip = tooltip = NULL;
	if(size != koord::invalid) {
		set_groesse(size);
	}
}


// set type. this includes size for specified buttons.
void button_t::set_typ(enum type t)
{
	type = t;
	switch (type&STATE_MASK) {
		case square:
			if(  translated_text  &&  strlen(translated_text)>0  ) {
				groesse.x = 16 + proportional_string_width( translated_text );
			}
			else {
				groesse.x = 10;
			}
			groesse.y = 10;
			break;
		case arrowleft:
		case repeatarrowleft:
		case arrowright:
		case repeatarrowright:
		case arrowup:
		case arrowdown:
		case posbutton:
			groesse.x = 10;
			groesse.y = 10;
			break;
		case roundbox:
			groesse.y = 14;
		break;
			default:
			break;
	}
}


/**
 * Setzt den im Button angezeigten Text
 * @author Hj. Malthaner
 */
void button_t::set_text(const char * text)
{
	this->text = text;
	translated_text = b_no_translate ? text : translator::translate(text);

	if(  (type&STATE_MASK)==square  &&  translated_text  &&  strlen(translated_text)>0  ) {
		groesse.x = 16 + proportional_string_width( translated_text );
	}
}



/**
 * Sets the tooltip of this button
 * @author Hj. Malthaner
 */
void button_t::set_tooltip(const char * t)
{
	if(t==NULL) {
		translated_tooltip = tooltip = NULL;
	}
	else {
		tooltip = t;
		translated_tooltip = b_no_translate ? tooltip : translator::translate(tooltip);
	}
}



bool button_t::getroffen(int x,int y) {
	bool hit=gui_komponente_t::getroffen(x, y);
	if(pressed  &&  !hit  &&  type<=STATE_MASK) {
		// moved away
		pressed = 0;
	}
	return hit;
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool button_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_OPEN) {
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
	// pressed or upressed
	if(!b_enabled  ||  IS_WINDOW_RESIZE(ev)) {
		return false;
	}

	// Knightly : check if the initial click and the current mouse positions are within the button's boundary
	const bool cxy_within_boundary = ( (ev->cx>=0 && ev->cx<get_groesse().x && ev->cy>=0 && ev->cy<get_groesse().y) ? true : false );
	const bool mxy_within_boundary = ( (ev->mx>=0 && ev->mx<get_groesse().x && ev->my>=0 && ev->my<get_groesse().y) ? true : false );

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
			// is in reality a point to koord
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



// draw button. x,y is top left of window.
void button_t::zeichnen(koord offset)
{
	if(  !is_visible()  ) {
		return;
	}

	int bx = offset.x + pos.x;
	int by = offset.y + pos.y;

	int bw = groesse.x;
	int bh = groesse.y;

	switch (type&STATE_MASK) {

		case box: // old, 4-line box
			{
				const gui_fenster_t *win = win_get_top();
				if(  win  &&  win->get_focus()==this  ) {
					// white box around
					display_fillbox_wh_clip(bx-1, by+bh, bw+2, 1, COL_WHITE, false);
				}
				if (pressed) {
					display_ddd_box_clip(bx, by, bw, bh, MN_GREY0, MN_GREY4);
					display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, background, false);
				}
				else {
					display_ddd_box_clip(bx, by, bw, bh, COL_GREY6, COL_GREY3);
					display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, background, false);
				}
				int len = proportional_string_width(translated_text);
				display_proportional_clip(bx+max((bw-len)/2,0),by+(bh-large_font_height)/2, translated_text, ALIGN_LEFT, b_enabled ? foreground : COL_GREY4, true);
			}
			break;

		case roundbox: // new box with round corners
			{
				const gui_fenster_t *win = win_get_top();
				if(  win  &&  win->get_focus()==this  ) {
					// white box around
					display_fillbox_wh_clip(bx-1, by-1, bw+2, 1, COL_WHITE, false);
					if(b_cap_left!=IMG_LEER  &&  bh==14) {
						display_fillbox_wh_clip(bx-1, by+skinverwaltung_t::window_skin->get_bild(13)->get_pic()->h, bw+2, 1, COL_WHITE, false);
					}
					else {
						display_fillbox_wh_clip(bx-1, by+bh, bw+2, 1, COL_WHITE, false);
					}
				}
				draw_roundbutton( bx, by, bw, bh, pressed );
				display_proportional_clip(bx+(bw>>1),by+(bh-large_font_height)/2, translated_text, ALIGN_MIDDLE, b_enabled ? foreground : COL_GREY4, true);
			}
			break;

		case square: // little square in front of text
			{
				const gui_fenster_t *win = win_get_top();
				if(  win  &&  win->get_focus()==this  ) {
					// white box around
					display_fillbox_wh_clip(bx+16, by+(12+large_font_height)/2-2, bw-14, 1, COL_WHITE, false);
				}
				if(  square_button_pushed!=IMG_LEER  ) {
					display_button_image(bx, by, SQUARE_BUTTON, pressed);
				}
				else {
					display_fillbox_wh_clip( bx, by, 11, 11, COL_BLACK, true );
					display_fillbox_wh_clip( bx+1, by+1, 9, 9, pressed ? MN_GREY3 : MN_GREY1, true );
				}
				display_proportional_clip(bx+16,by+(12-large_font_height)/2, translated_text, ALIGN_LEFT, b_enabled ? foreground : COL_GREY4, true);
			}
			break;

		case arrowleft:
		case repeatarrowleft:
			if(  arrow_left_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_LEFT, pressed);
			}
			else {
				display_ddd_proportional_clip( bx, by+5, 14, 0, pressed ? MN_GREY1 : MN_GREY3, COL_BLACK, "<", true );
			}
			break;

		case posbutton:
		case arrowright:
		case repeatarrowright:
			if(  arrow_right_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_RIGHT, pressed);
			}
			else {
				display_ddd_proportional_clip( bx, by+5, 14, 0, pressed ? MN_GREY1 : MN_GREY3, COL_BLACK, ">", true );
			}
			break;

		case arrowup:
			if(  arrow_up_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_UP, pressed);
			}
			else {
				display_ddd_proportional_clip( bx, by+5, 14, 0, pressed ? MN_GREY1 : MN_GREY3, COL_BLACK, "+", true );
			}
			break;

		case arrowdown:
			if(  arrow_down_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_DOWN, pressed);
			}
			else {
				display_ddd_proportional_clip( bx, by+5, 14, 0, pressed ? MN_GREY1 : MN_GREY3, COL_BLACK, "+", true );
			}
			break;

		case scrollbar_horizontal:
		case scrollbar_vertical:
			// new 3d-look scrollbar knob
			// pressed: background
			if(  scrollbar_center==IMG_LEER  ) {
				mark_rect_dirty_wc(bx, by, bx+bw-1, by+bh-1);
				// use own 3D like routines
				if (pressed) {
					// slider is pressed button ...
					display_fillbox_wh_clip(bx+2, by+2, bw-3, bh-3, MN_GREY1, false);
					display_vline_wh_clip  (bx+2, by+3, 2,   MN_GREY0, false);
					display_fillbox_wh_clip(bx+2, by+2, 3,1, MN_GREY0, false);
					display_vline_wh_clip  (bx+1, by+2, bh-3,   COL_BLACK, false);
					display_fillbox_wh_clip(bx+1, by+1, bw-2,1, COL_BLACK, false);
					display_vline_wh_clip  (bx+bw-2, by+3, bh-5,   MN_GREY2, false);
					display_fillbox_wh_clip(bx+3, by+bh-2, bw-4,1, MN_GREY2, false);
					display_vline_wh_clip  (bx, by+1, bh-2, MN_GREY0, false);
					display_fillbox_wh_clip(bx, by, bw-1,1, MN_GREY0, false);
					display_vline_wh_clip  (bx+bw-1, by, bh,   COL_WHITE, false);
					display_fillbox_wh_clip(bx, by+bh-1, bw,1, COL_WHITE, false);
				}
				else {
					display_fillbox_wh_clip(bx+1, by+1, bw-3, bh-3, MN_GREY3, false);
					display_vline_wh_clip  (bx+bw-3, by+bh-5, 2,   MN_GREY1, false);
					display_fillbox_wh_clip(bx+bw-5, by+bh-3, 3,1, MN_GREY1, false);
					display_vline_wh_clip  (bx+1, by+2, bh-5,   MN_GREY4, false);
					display_fillbox_wh_clip(bx+1, by+1, bw-4,1, MN_GREY4, false);
					display_vline_wh_clip  (bx+bw-2, by+1, bh-3,   MN_GREY0, false);
					display_fillbox_wh_clip(bx+1, by+bh-2, bw-2,1, MN_GREY0, false);
					display_vline_wh_clip  (bx, by+1, bh-2, COL_WHITE, false);
					display_fillbox_wh_clip(bx, by, bw-1,1, COL_WHITE, false);
					display_vline_wh_clip  (bx+bw-1, by, bh,   COL_BLACK, false);
					display_fillbox_wh_clip(bx, by+bh-1, bw,1, COL_BLACK, false);
				}
			}
			else {
				draw_scrollbar( bx, by, bw, bh, (type&STATE_MASK)==scrollbar_horizontal, !pressed );
			}
		break;
	}

	if(translated_tooltip &&  getroffen( get_maus_x()-offset.x, get_maus_y()-offset.y )) {
		win_set_tooltip(get_maus_x() + 16, get_maus_y() - 16, translated_tooltip );
	}
}


gui_komponente_t *button_t::get_focus() const
{
	switch (type&STATE_MASK) {

		case box: // old, 4-line box
		case roundbox: // new box with round corners
		case square: // little square in front of text
			return (gui_komponente_t *)this;

		// those cannot recieve focus ...
		case arrowleft:
		case repeatarrowleft:
		case arrowright:
		case repeatarrowright:
		case posbutton:
		case arrowup:
		case arrowdown:
		case scrollbar_horizontal:
		case scrollbar_vertical:
			break;
	}
	return NULL;
}
