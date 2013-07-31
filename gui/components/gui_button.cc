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
#include "../../simgraph.h"
#include "../../simevent.h"
#include "../../simwin.h"

#include "../../dataobj/translator.h"

#include "../../simskin.h"
#include "../../besch/skin_besch.h"
#include "../../utils/simstring.h"

#include "../gui_frame.h"

#define STATE_MASK (127)
#define AUTOMATIC_MASK (255)

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

COLOR_VAL button_t::button_color_text = COL_BLACK;
COLOR_VAL button_t::button_color_disabled_text = MN_GREY0;

/**
 * Max Kielland
 * These are the built in default theme element sizes and
 * are overriden by the PAK file if a new image is defined.
 */
koord button_t::gui_button_size        = koord(92,14);
koord button_t::gui_checkbox_size      = koord(10,10);
koord button_t::gui_arrow_left_size    = koord(14,14);
koord button_t::gui_arrow_right_size   = koord(14,14);
koord button_t::gui_arrow_up_size      = koord(14,14);
koord button_t::gui_arrow_down_size    = koord(14,14);
koord button_t::gui_scrollbar_size     = koord(14,14);
koord button_t::gui_scrollknob_size    = koord(14,14);
koord button_t::gui_indicator_box_size = koord(10, 6);

/*
 * Hajo: image numbers of button skins
 * Max Kielland: These are also optional now.
 */
image_id button_t::square_button_pushed    = IMG_LEER;
image_id button_t::square_button_normal    = IMG_LEER;
image_id button_t::arrow_left_pushed       = IMG_LEER;
image_id button_t::arrow_left_normal       = IMG_LEER;
image_id button_t::arrow_right_pushed      = IMG_LEER;
image_id button_t::arrow_right_normal      = IMG_LEER;
image_id button_t::arrow_up_pushed         = IMG_LEER;
image_id button_t::arrow_up_normal         = IMG_LEER;
image_id button_t::arrow_down_pushed       = IMG_LEER;
image_id button_t::arrow_down_normal       = IMG_LEER;

// these are optional: buttons made out of graphics
image_id button_t::b_cap_left              = IMG_LEER;
image_id button_t::b_body                  = IMG_LEER;
image_id button_t::b_cap_right             = IMG_LEER;

image_id button_t::b_cap_left_p            = IMG_LEER;
image_id button_t::b_body_p                = IMG_LEER;
image_id button_t::b_cap_right_p           = IMG_LEER;

// these are optional: scrollbar ids
image_id button_t::scrollbar_left          = IMG_LEER;
image_id button_t::scrollbar_right         = IMG_LEER;
image_id button_t::scrollbar_middle        = IMG_LEER;

image_id button_t::scrollbar_slider_left   = IMG_LEER;
image_id button_t::scrollbar_slider_right  = IMG_LEER;
image_id button_t::scrollbar_slider_middle = IMG_LEER;

// these are optional: ... and scrollbars vertical
image_id button_t::scrollbar_top           = IMG_LEER;
image_id button_t::scrollbar_bottom        = IMG_LEER;
image_id button_t::scrollbar_center        = IMG_LEER;

image_id button_t::scrollbar_slider_top    = IMG_LEER;
image_id button_t::scrollbar_slider_bottom = IMG_LEER;
image_id button_t::scrollbar_slider_center = IMG_LEER;


button_t::button_t() :
	gui_komponente_t(true)
{
	b_no_translate = false;
	pressed = false;
	translated_tooltip = tooltip = NULL;
	background = SYSCOL_FACE;
	b_enabled = true;

	// By default a box button
	init(box,"");
}


void button_t::init(enum type type_par, const char *text_par, koord pos_par, koord size_par)
{
	translated_tooltip = NULL;
	tooltip = NULL;
	b_no_translate = ( type_par==posbutton );

	set_typ(type_par);
	set_text(text_par);
	set_pos(pos_par);
	if(  size_par != koord::invalid  ) {
		set_groesse(size_par);
	}
}


/**
 * Lazy button image number init
 * @author Hj. Malthaner
 */
void button_t::init_button_images() {

	const bild_t *image;

	if(skinverwaltung_t::window_skin!=NULL) {

		square_button_normal    = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_CHECKBOX               );
		square_button_pushed    = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_CHECKBOX_CHECKED       );
		arrow_left_normal       = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_LEFT             );
		arrow_left_pushed       = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_LEFT_PRESSED     );
		arrow_right_normal      = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_RIGHT            );
		arrow_right_pushed      = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_RIGHT_PRESSED    );
		b_cap_left              = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_SIDE_LEFT              );
		b_cap_right             = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_SIDE_RIGHT             );
		b_body                  = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_BODY                   );
		b_cap_left_p            = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_SIDE_LEFT_PRESSED      );
		b_cap_right_p           = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_SIDE_RIGHT_PRESSED     );
		b_body_p                = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_BODY_PRESSED           );
		arrow_up_normal         = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_UP               );
		arrow_up_pushed         = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_UP_PRESSED       );
		arrow_down_normal       = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_DOWN             );
		arrow_down_pushed       = skinverwaltung_t::window_skin->get_bild_nr( SKIN_BUTTON_ARROW_DOWN_PRESSED     );
		scrollbar_left          = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_H_BACKGROUND_LEFT   );
		scrollbar_right         = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_H_BACKGROUND_RIGHT  );
		scrollbar_middle        = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_H_BACKGROUND        );
		scrollbar_slider_left   = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_H_KNOB_LEFT         );
		scrollbar_slider_right  = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_H_KNOB_RIGHT        );
		scrollbar_slider_middle = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_H_KNOB_BODY         );
		scrollbar_top           = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_V_BACKGROUND_TOP    );
		scrollbar_bottom        = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_V_BACKGROUND_BOTTOM );
		scrollbar_center        = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_V_BACKGROUND        );
		scrollbar_slider_top    = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_V_KNOB_TOP          );
		scrollbar_slider_bottom = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_V_KNOB_BOTTOM       );
		scrollbar_slider_center = skinverwaltung_t::window_skin->get_bild_nr( SKIN_SCROLLBAR_V_KNOB_BODY         );

		// Calculate checkbox size
		if(  square_button_normal != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_CHECKBOX )->get_pic();
			gui_checkbox_size = koord(image->w,image->h);
		}

		// Calculate arrow left size
		if(  arrow_left_normal != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_ARROW_LEFT )->get_pic();
			gui_arrow_left_size = koord(image->w,image->h);
		}

		// Calculate arrow right size
		if(  arrow_right_normal != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_ARROW_RIGHT )->get_pic();
			gui_arrow_right_size = koord(image->w,image->h);
		}

		// Calculate arrow up size
		if(  arrow_up_normal != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_ARROW_UP )->get_pic();
			gui_arrow_up_size = koord(image->w,image->h);
		}

		// Calculate arrow down size
		if(  arrow_down_normal != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_ARROW_DOWN )->get_pic();
			gui_arrow_down_size = koord(image->w,image->h);
		}

		// Calculate round button size
		if(  (b_cap_left != IMG_LEER)  &&  (b_cap_right != IMG_LEER)  &&  (b_body != IMG_LEER)  ) {

			// calculate button width
			scr_coord_val button_width = skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_SIDE_LEFT  )->get_pic()->w +
			                             skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_SIDE_RIGHT )->get_pic()->w +
			                             skinverwaltung_t::window_skin->get_bild( SKIN_BUTTON_BODY       )->get_pic()->w;
			gui_button_size.x = max(gui_button_size.x,button_width);

			// calculate button height
			// gui_button_size.y = max(gui_button_size.y, skinverwaltung_t::window_skin->get_bild(SKIN_BUTTON_BODY)->get_pic()->h);
			gui_button_size.y = max(gui_button_size.y, skinverwaltung_t::window_skin->get_bild(SKIN_BUTTON_BODY)->get_pic()->h);
		}

		// Calculate V scrollbar size
		image = NULL;
		if(  scrollbar_center != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_SCROLLBAR_V_BACKGROUND )->get_pic();
		}
		else if(  scrollbar_slider_center != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_SCROLLBAR_V_KNOB_BODY )->get_pic();
		}
		gui_scrollbar_size.x = (image) ? image->w : gui_arrow_up_size.x;

		if(  scrollbar_slider_center != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_SCROLLBAR_V_KNOB_BODY )->get_pic();
			gui_scrollknob_size.x = image->w;
		}
		else {
			gui_scrollknob_size.x = gui_scrollbar_size.x;
		}

		// Calculate H scrollbar size
		image = NULL;
		if(  scrollbar_middle != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_SCROLLBAR_H_BACKGROUND )->get_pic();
		}
		else if(  scrollbar_slider_middle != IMG_LEER  ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_SCROLLBAR_H_KNOB_BODY )->get_pic();
		}
		gui_scrollbar_size.y = (image) ? image->h : gui_arrow_left_size.y;

		if ( scrollbar_slider_middle != IMG_LEER ) {
			image = skinverwaltung_t::window_skin->get_bild( SKIN_SCROLLBAR_H_KNOB_BODY )->get_pic();
			gui_scrollknob_size.y = image->h;
		}
		else {
			gui_scrollknob_size.y = gui_scrollbar_size.y;
		}

	}

}


/**
 * Displays the different button types
 * @author Hj. Malthaner
 */
void button_t::display_button_image(scr_coord_val x, scr_coord_val y, int number, bool pushed) const
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
void button_t::draw_roundbutton(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h, bool pressed)
{
	if(  (h == gui_button_size.y) && (
		                               ( !pressed && (b_cap_left   != IMG_LEER) && (b_cap_right   != IMG_LEER) && (b_body   != IMG_LEER)) ||
		                               (  pressed && (b_cap_left_p != IMG_LEER) && (b_cap_right_p != IMG_LEER) && (b_body_p != IMG_LEER))
		                             )
	  ) {

		const scr_coord_val lw = skinverwaltung_t::window_skin->get_bild(SKIN_BUTTON_SIDE_LEFT  + (3*pressed))->get_pic()->w;
		const scr_coord_val rw = skinverwaltung_t::window_skin->get_bild(SKIN_BUTTON_SIDE_RIGHT + (3*pressed))->get_pic()->w;
		const scr_coord_val cw = skinverwaltung_t::window_skin->get_bild(SKIN_BUTTON_BODY       + (3*pressed))->get_pic()->w;

		// first the center (may need extra clipping)
		if(  w-lw-rw < cw  ) {
			clip_dimension const cl = display_get_clip_wh();
			display_set_clip_wh(cl.x, cl.y, max(0,min(x+w-rw,cl.xx)-cl.x), cl.h );
			display_button_image(x+lw, y, RB_BODY_BUTTON, pressed);
			display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
		}
		else {
			// Buttons in a differents width than original skin image
			for(  scr_coord_val j=0;  j+cw < w-rw-lw;  j += cw  ) {
				display_button_image(x+j+lw, y, RB_BODY_BUTTON, pressed);
			}
			display_button_image(x+w-rw-cw, y, RB_BODY_BUTTON, pressed);
		}

		// now the begin and end ...
		display_button_image(x, y, RB_LEFT_BUTTON, pressed);
		display_button_image(x+w-rw, y, RB_RIGHT_BUTTON, pressed);
	}

	else {
		// draw the button conventionally from boxes
		// fallback, if nothing defined
		if(pressed) {
			display_fillbox_wh_clip(x, y, w, 1, MN_GREY1, true);
			display_fillbox_wh_clip(x+1, y+1, w-2, 1, COL_BLACK, true);
			display_fillbox_wh_clip(x+2, y+2, w-2, h-4, MN_GREY1, true);
			display_fillbox_wh_clip(x, y+h-2, w, 1, MN_GREY3, true);
			display_fillbox_wh_clip(x, y+h-1, w, 1, COL_WHITE, true);
			display_vline_wh_clip(x+w-2, y+1, h-2, SYSCOL_HIGHLIGHT, true);
			display_vline_wh_clip(x+w-1, y+1, h-1, COL_WHITE, true);
			display_vline_wh_clip(x, y, h, MN_GREY1, true);
			display_vline_wh_clip(x+1, y+1, h-2, COL_BLACK, true);
		}
		else {
			display_fillbox_wh_clip(x, y, w, 1, COL_WHITE, true);
			display_fillbox_wh_clip(x+1, y+1, w-2, 1, SYSCOL_HIGHLIGHT, true);
			display_fillbox_wh_clip(x+2, y+2, w-2, h-4, MN_GREY3, true);
			display_fillbox_wh_clip(x, y+h-2, w, 1, MN_GREY1, true);
			display_fillbox_wh_clip(x, y+h-1, w, 1, COL_BLACK, true);
			display_vline_wh_clip(x+w-2, y+1, h-2, MN_GREY1, true);
			display_vline_wh_clip(x+w-1, y+1, h-1, COL_BLACK, true);
			display_vline_wh_clip(x, y, h, COL_WHITE, true);
			display_vline_wh_clip(x+1, y+1, h-2, SYSCOL_HIGHLIGHT, true);
		}
	}
}



void button_t::draw_scrollbar(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h, bool horizontal, bool slider)
{
	if(  scrollbar_middle!=IMG_LEER  ) {
		if(  horizontal  ) {
			const int image_offset = (slider ? SKIN_SCROLLBAR_H_KNOB_LEFT : SKIN_SCROLLBAR_H_BACKGROUND_LEFT);
			const scr_coord_val lw = skinverwaltung_t::window_skin->get_bild(image_offset)->get_pic()->w;
			const scr_coord_val rw = skinverwaltung_t::window_skin->get_bild(image_offset+1)->get_pic()->w;
			// first the center (may need extra clipping)
			if(w-lw-rw<64) {
				clip_dimension const cl = display_get_clip_wh();
				display_set_clip_wh(cl.x, cl.y, max(0,min(x+w-rw,cl.xx)-cl.x), cl.h );
				display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x+lw, y, 0, false, true);
				display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
			}
			else {
				// wider buttons
				for( scr_coord_val j=0;  j+64<w-rw-lw;  j+=64) {
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
			const int image_offset = (slider ? SKIN_SCROLLBAR_V_KNOB_TOP : SKIN_SCROLLBAR_V_BACKGROUND_TOP);
			const scr_coord_val lh = skinverwaltung_t::window_skin->get_bild(image_offset)->get_pic()->h;
			const scr_coord_val rh = skinverwaltung_t::window_skin->get_bild(image_offset+1)->get_pic()->h;
			// first the center (may need extra clipping)
			if(h-lh-rh<64) {
				clip_dimension const cl = display_get_clip_wh();
				display_set_clip_wh(cl.x, cl.y, cl.w, max(0,min(y+h-rh,cl.yy)-cl.y) );
				display_color_img(skinverwaltung_t::window_skin->get_bild_nr(image_offset+2), x, y+lh, 0, false, true);
				display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
			}
			else {
				// wider buttons
				for( scr_coord_val j=0;  j+64<h-rh-lh;  j+=64) {
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


// set type. this includes size for specified buttons.
void button_t::set_typ(enum type t)
{
	type = t;
	foreground = SYSCOL_BUTTON_TEXT;
	switch (type&STATE_MASK) {

		case square:
			foreground = SYSCOL_BUTTON_TEXT;
			if(  !strempty(translated_text)  ) {
				set_text(translated_text);
				set_groesse ( koord( gui_checkbox_size.x + D_H_SPACE + proportional_string_width( translated_text ), max(gui_checkbox_size.y,LINESPACE)) );
			}
			else {
				set_groesse( koord( gui_checkbox_size.x, max(gui_checkbox_size.y,LINESPACE)) );
			}
			break;

		case arrowleft:
		case repeatarrowleft:
			set_groesse( gui_arrow_left_size );
			break;

		case posbutton:
		case arrowright:
		case repeatarrowright:
			set_groesse( gui_arrow_right_size );
			break;

		case arrowup:
			set_groesse( gui_arrow_up_size );
			break;

		case arrowdown:
			set_groesse( gui_arrow_down_size );
			break;

		case roundbox:
		case box:
			set_groesse( koord (gui_button_size.x, max(gui_button_size.y,LINESPACE)) );
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
		set_groesse ( koord( gui_checkbox_size.x + D_H_SPACE + proportional_string_width( translated_text ), max(gui_checkbox_size.y,LINESPACE)) );
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
	bool const cxy_within_boundary = 0 <= ev->cx && ev->cx < get_groesse().x && 0 <= ev->cy && ev->cy < get_groesse().y;
	bool const mxy_within_boundary = 0 <= ev->mx && ev->mx < get_groesse().x && 0 <= ev->my && ev->my < get_groesse().y;

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


void button_t::draw_focus_rect(koord xy, koord wh, scr_coord_val offset) {

	scr_coord_val w = ((offset-1)<<1);

	display_fillbox_wh_clip(xy.x-offset+1,      xy.y-1-offset+1,    wh.x+w, 1, COL_WHITE, false);
	display_fillbox_wh_clip(xy.x-offset+1,      xy.y+wh.y+offset-1, wh.x+w, 1, COL_WHITE, false);
	display_vline_wh_clip  (xy.x-offset,        xy.y-offset+1,      wh.y+w,    COL_WHITE, false);
	display_vline_wh_clip  (xy.x+wh.x+offset-1, xy.y-offset+1,      wh.y+w,    COL_WHITE, false);
}


// draw button. x,y is top left of window.
void button_t::zeichnen(koord offset)
{
	if(  !is_visible()  ) {
		return;
	}

	// Helpers
	const scr_coord_val bx = offset.x + pos.x;
	const scr_coord_val by = offset.y + pos.y;
	const scr_coord_val bw = groesse.x;
	const scr_coord_val bh = groesse.y;

	// Offset to center text relative to the button's height
	const scr_coord_val y_text_offset = D_GET_CENTER_ALIGN_OFFSET(LINESPACE,max(groesse.y,LINESPACE));

	switch (type&STATE_MASK) {

		case box: // old, 4-line box
			{
				if (pressed) {
					display_ddd_box_clip(bx, by, bw, bh, SYSCOL_SHADOW, SYSCOL_HIGHLIGHT);
				}
				else {
					display_ddd_box_clip(bx, by, bw, bh, COL_GREY6, COL_GREY3);
				}
				display_fillbox_wh_clip(bx+1, by+1, bw-2, bh-2, background, false);

				size_t idx = display_fit_proportional(translated_text, bw-3, translator::get_lang()->eclipse_width );
				if(  translated_text[idx]==0  ) {
					display_proportional_clip( bx+bw/2, by+y_text_offset, translated_text, ALIGN_CENTER_H, b_enabled ? foreground : SYSCOL_DISABLED_TEXT, true);
				}
				else {
					scr_coord_val w = display_text_proportional_len_clip( bx+1, by+y_text_offset, translated_text, ALIGN_LEFT | DT_DIRTY | DT_CLIP, b_enabled ? foreground : SYSCOL_DISABLED_BUTTON_TEXT, idx );
					display_proportional_clip( bx+1+w, by+y_text_offset, translator::translate("..."), ALIGN_LEFT | DT_DIRTY | DT_CLIP, b_enabled ? foreground : SYSCOL_DISABLED_BUTTON_TEXT, true );
				}

				if(  win_get_focus()==this  ) {
					draw_focus_rect(koord(bx,by), groesse);
				}
			}
			break;

		case roundbox: // button with round corners
			{
				draw_roundbutton( bx, by, bw, bh, pressed );

				size_t idx = display_fit_proportional(translated_text, bw-3, translator::get_lang()->eclipse_width );
				if(  translated_text[idx]==0  ) {
					display_proportional_clip( bx+bw/2, by+y_text_offset, translated_text, ALIGN_CENTER_H, b_enabled ? foreground : SYSCOL_DISABLED_BUTTON_TEXT, true);
				}
				else {
					scr_coord_val w = display_text_proportional_len_clip( bx+1, by+y_text_offset, translated_text, ALIGN_LEFT | DT_DIRTY | DT_CLIP, b_enabled ? foreground : SYSCOL_DISABLED_BUTTON_TEXT, idx );
					display_proportional_clip( bx+1+w, by+y_text_offset, translator::translate("..."), ALIGN_LEFT | DT_DIRTY | DT_CLIP, b_enabled ? foreground : SYSCOL_DISABLED_BUTTON_TEXT, true );
				}

				if(  win_get_focus()==this  ) {
					draw_focus_rect(koord(bx,by), groesse);
				}

			}
			break;

		case square: // checkbox with text
			{
				scr_coord_val box_y_offset = D_GET_CENTER_ALIGN_OFFSET(gui_checkbox_size.y,groesse.y);
				if(  square_button_pushed != IMG_LEER  ) {
					display_button_image(bx, by+box_y_offset, SQUARE_BUTTON, pressed);
				}
				else {
					scr_coord_val width = display_get_char_width('X');
					display_fillbox_wh_clip( bx, by+box_y_offset, gui_checkbox_size.x, gui_checkbox_size.y, COL_BLACK, true );
					display_fillbox_wh_clip( bx+1, by+1+box_y_offset, gui_checkbox_size.x-2, gui_checkbox_size.y-2, pressed ? SYSCOL_HIGHLIGHT : MN_GREY1, true );
					if(pressed) {
						display_proportional_clip( bx+1+D_GET_CENTER_ALIGN_OFFSET(width,gui_checkbox_size.x),by+D_GET_CENTER_ALIGN_OFFSET(LINESPACE,groesse.y) , "X", ALIGN_LEFT, b_enabled ? foreground : SYSCOL_DISABLED_BUTTON_TEXT, true);
					}
				}

				if(  text  ) {
					size_t idx = display_fit_proportional( translated_text, bw-D_H_SPACE-gui_checkbox_size.x+1, translator::get_lang()->eclipse_width );
					if(  translated_text[idx]==0  ) {
						display_proportional_clip( bx+gui_checkbox_size.x+D_H_SPACE, by+y_text_offset, translated_text, ALIGN_LEFT, b_enabled ? foreground : SYSCOL_DISABLED_TEXT, true);
					}
					else {
						scr_coord_val w = display_text_proportional_len_clip( bx+gui_checkbox_size.x+D_H_SPACE, by+y_text_offset, translated_text, ALIGN_LEFT | DT_DIRTY | DT_CLIP, b_enabled ? foreground : SYSCOL_DISABLED_TEXT, idx );
						display_proportional_clip( bx+gui_checkbox_size.x+D_H_SPACE+w, by+y_text_offset, translator::translate("..."), ALIGN_LEFT | DT_DIRTY | DT_CLIP, b_enabled ? foreground : SYSCOL_DISABLED_TEXT, true );
					}
				}

				if(  win_get_focus() == this  ) {
					draw_focus_rect(koord(bx,by+box_y_offset), gui_checkbox_size );
				}
			}
			break;

		case arrowleft:
		case repeatarrowleft:
			if(  arrow_left_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_LEFT, pressed);
			}
			else {
				scr_coord_val width = display_get_char_width('<');
				display_ddd_box_clip( bx, by, gui_arrow_left_size.x, gui_arrow_left_size.y, pressed ? COL_GREY3 : COL_GREY6, pressed ? COL_GREY6 : COL_GREY3 );
				display_proportional_clip(bx+1+D_GET_CENTER_ALIGN_OFFSET(width,gui_arrow_left_size.x),by+D_GET_CENTER_ALIGN_OFFSET(LINESPACE,groesse.y) , "<", ALIGN_LEFT, COL_BLACK, true);

			}
			break;

		case posbutton:
		case arrowright:
		case repeatarrowright:
			if(  arrow_right_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_RIGHT, pressed);
			}
			else {
				scr_coord_val width = display_get_char_width('>');
				display_ddd_box_clip( bx, by, gui_arrow_right_size.x, gui_arrow_right_size.y, pressed ? COL_GREY3 : COL_GREY6, pressed ? COL_GREY6 : COL_GREY3 );
				display_proportional_clip(bx+1+D_GET_CENTER_ALIGN_OFFSET(width,gui_arrow_right_size.x),by+D_GET_CENTER_ALIGN_OFFSET(LINESPACE,groesse.y) , ">", ALIGN_LEFT, COL_BLACK, true);
			}
			break;

		case arrowup:
			if(  arrow_up_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_UP, pressed);
			}
			else {
				scr_coord_val width = display_get_char_width('^');
				display_ddd_box_clip( bx, by, gui_arrow_up_size.x, gui_arrow_up_size.y, pressed ? COL_GREY3 : COL_GREY6, pressed ? COL_GREY6 : COL_GREY3 );
				display_proportional_clip(bx+1+D_GET_CENTER_ALIGN_OFFSET(width,gui_arrow_up_size.x),by+D_GET_CENTER_ALIGN_OFFSET(LINESPACE,groesse.y) , "^", ALIGN_LEFT, COL_BLACK, true);
			}
			break;

		case arrowdown:
			if(  arrow_down_pushed!=IMG_LEER  ) {
				display_button_image(bx, by, ARROW_DOWN, pressed);
			}
			else {
				scr_coord_val width = display_get_char_width('v');
				display_ddd_box_clip( bx, by, gui_arrow_down_size.x, gui_arrow_down_size.y, pressed ? COL_GREY3 : COL_GREY6, pressed ? COL_GREY6 : COL_GREY3 );
				display_proportional_clip(bx+1+D_GET_CENTER_ALIGN_OFFSET(width,gui_arrow_down_size.x),by+D_GET_CENTER_ALIGN_OFFSET(LINESPACE,groesse.y) , "v", ALIGN_LEFT, COL_BLACK, true);
			}
			break;

		case scrollbar_horizontal:
		case scrollbar_vertical:
			// new 3d-look scrollbar knob
			// pressed: background
			if(  scrollbar_center!=IMG_LEER  ) {
				draw_scrollbar( bx, by, bw, bh, (type&STATE_MASK)==scrollbar_horizontal, !pressed );
			}
			else {
				mark_rect_dirty_wc(bx, by, bx+bw-1, by+bh-1);
				// use own 3D like routines
				if (pressed) {
					// slider is pressed button ...
					display_fillbox_wh_clip(bx+2, by+2, bw-3, bh-3, MN_GREY1, false);
					display_vline_wh_clip  (bx+2, by+3, 2,   SYSCOL_SHADOW, false);
					display_fillbox_wh_clip(bx+2, by+2, 3,1, SYSCOL_SHADOW, false);
					display_vline_wh_clip  (bx+1, by+2, bh-3,   COL_BLACK, false);
					display_fillbox_wh_clip(bx+1, by+1, bw-2,1, COL_BLACK, false);
					display_vline_wh_clip  (bx+bw-2, by+3, bh-5,   SYSCOL_FACE, false);
					display_fillbox_wh_clip(bx+3, by+bh-2, bw-4,1, SYSCOL_FACE, false);
					display_vline_wh_clip  (bx, by+1, bh-2, SYSCOL_SHADOW, false);
					display_fillbox_wh_clip(bx, by, bw-1,1, SYSCOL_SHADOW, false);
					display_vline_wh_clip  (bx+bw-1, by, bh,   COL_WHITE, false);
					display_fillbox_wh_clip(bx, by+bh-1, bw,1, COL_WHITE, false);
				}
				else {
					display_fillbox_wh_clip(bx+1, by+1, bw-3, bh-3, MN_GREY3, false);
					display_vline_wh_clip  (bx+bw-3, by+bh-5, 2,   MN_GREY1, false);
					display_fillbox_wh_clip(bx+bw-5, by+bh-3, 3,1, MN_GREY1, false);
					display_vline_wh_clip  (bx+1, by+2, bh-5,   SYSCOL_HIGHLIGHT, false);
					display_fillbox_wh_clip(bx+1, by+1, bw-4,1, SYSCOL_HIGHLIGHT, false);
					display_vline_wh_clip  (bx+bw-2, by+1, bh-3,   SYSCOL_SHADOW, false);
					display_fillbox_wh_clip(bx+1, by+bh-2, bw-2,1, SYSCOL_SHADOW, false);
					display_vline_wh_clip  (bx, by+1, bh-2, COL_WHITE, false);
					display_fillbox_wh_clip(bx, by, bw-1,1, COL_WHITE, false);
					display_vline_wh_clip  (bx+bw-1, by, bh,   COL_BLACK, false);
					display_fillbox_wh_clip(bx, by+bh-1, bw,1, COL_BLACK, false);
				}
			}

		break;
	}

	if(  translated_tooltip  &&  getroffen( get_maus_x()-offset.x, get_maus_y()-offset.y )  ) {
		win_set_tooltip(get_maus_x() + TOOLTIP_MOUSE_OFFSET_X, by + bh + TOOLTIP_MOUSE_OFFSET_Y, translated_tooltip, this);
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
		case scrollbar_horizontal:
		case scrollbar_vertical:
		default:
			set_focusable(false);
			break;
	}
}
