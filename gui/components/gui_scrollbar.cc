/*
 * Scrollbar class
 * Niel Roest
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../../simdebug.h"
#include "../../simcolor.h"
#include "../../simgraph.h"
#include "../../simskin.h"
#include "../../besch/skin_besch.h"
#include "action_listener.h"
#include "gui_scrollbar.h"


scrollbar_t::scrollbar_t(enum type type) :
	type(type),
	knob_offset(0),
	knob_size(10),
	knob_area(20),
	knob_scroll_amount(LINESPACE), // equals one line
	knob_scroll_discrete(true)
{

	if (type == vertical) {
		groesse = koord(button_t::gui_scrollbar_size.x,40); // 40 = default scrollbar height
		button_def[left_top_arrow_index].set_typ(button_t::arrowup);
		button_def[right_bottom_arrow_index].set_typ(button_t::arrowdown);
		button_def[knob_index].set_typ(button_t::scrollbar_vertical);
		button_def[background_index].set_typ(button_t::scrollbar_vertical);
	}
	else { // horizontal
		groesse = koord(40,button_t::gui_scrollbar_size.y); // 40 = default scrollbar length
		button_def[left_top_arrow_index].set_typ(button_t::arrowleft);
		button_def[right_bottom_arrow_index].set_typ(button_t::arrowright);
		button_def[knob_index].set_typ(button_t::scrollbar_horizontal);
		button_def[background_index].set_typ(button_t::scrollbar_horizontal);
	}

	button_def[left_top_arrow_index].set_pos(koord(0,0));
	button_def[right_bottom_arrow_index].set_pos(koord(0,0));
	reposition_buttons();
}



void scrollbar_t::set_groesse(koord groesse)
{
	if (type == vertical) {
		this->groesse.y = groesse.y;
	}
	else {
		this->groesse.x = groesse.x;
	}
	reposition_buttons();
}



void scrollbar_t::set_knob(sint32 size, sint32 area)
{
	if(size<1  ||  area<1) {
//		dbg->warning("scrollbar_t::set_knob()","size=%i, area=%i not in 1...x",size,area);
	}
	knob_size = max(1,size);
	knob_area = max(1,area);
	reposition_buttons();
}



// reset variable position and size values of the three buttons
void scrollbar_t::reposition_buttons()
{

	const sint32 area = (type == vertical ? groesse.y-button_t::gui_arrow_up_size.y-button_t::gui_arrow_down_size.y : groesse.x-button_t::gui_arrow_left_size.x-button_t::gui_arrow_right_size.x); // area will be actual area knob can move in

	// check if scrollbar is too low
	if(  knob_size+knob_offset>knob_area+(knob_scroll_discrete?(knob_scroll_amount-1):0)  ) {
		knob_offset = max(0,(knob_area-knob_size));

		if(  knob_scroll_discrete  ) {
			knob_offset -= knob_offset%knob_scroll_amount;
		}
		call_listeners((long)knob_offset);
	}

	float ratio = (float)area / (float)knob_area;
	if (knob_area < knob_size) {
		ratio = (float)area / (float)knob_size;
	}

	sint32 offset;
	if(  knob_size+knob_offset>knob_area  ) {
		offset = max(0,((sint32)( (float)(knob_area-knob_size)*ratio+.5)));
	}
	else {
		offset = (sint32)( (float)knob_offset*ratio+.5);
	}

	sint32 size = (sint32)( (float)knob_size*ratio+.5);
	if(type == vertical) {

		button_def[left_top_arrow_index].set_pos( koord( (button_t::gui_scrollbar_size.x-button_t::gui_arrow_up_size.x)/2, 0) );
		button_def[right_bottom_arrow_index].set_pos( koord( (button_t::gui_scrollbar_size.x-button_t::gui_arrow_down_size.x)/2, groesse.y-button_t::gui_arrow_down_size.y) );
		button_def[knob_index].set_pos( koord( 0, button_t::gui_arrow_up_size.y+offset ) );

		if(  button_t::scrollbar_slider_top!=IMG_LEER  ) {
			size = max( size,
			            skinverwaltung_t::window_skin->get_bild(SKIN_SCROLLBAR_V_KNOB_TOP)->get_pic()->h+
			            skinverwaltung_t::window_skin->get_bild(SKIN_SCROLLBAR_V_KNOB_BOTTOM)->get_pic()->h
			          );
		}

		button_def[knob_index].set_groesse( koord(button_t::gui_scrollknob_size.x,size) );
		button_def[background_index].set_pos( koord(0,button_t::gui_arrow_up_size.y) );
		button_def[background_index].set_groesse( koord(button_t::gui_scrollbar_size.x,groesse.y-button_t::gui_arrow_up_size.y-button_t::gui_arrow_down_size.y) );
	}
	else { // horizontal

		button_def[left_top_arrow_index].set_pos( koord(0,(button_t::gui_scrollbar_size.y-button_t::gui_arrow_left_size.y)/2) );
		button_def[right_bottom_arrow_index].set_pos( koord(groesse.x-button_t::gui_arrow_right_size.x,(button_t::gui_scrollbar_size.y-button_t::gui_arrow_right_size.y)/2) );
		button_def[knob_index].set_pos( koord(button_t::gui_arrow_left_size.x+offset,0) );

		if(  button_t::scrollbar_slider_left!=IMG_LEER  ) {
			size = max( size,
			            skinverwaltung_t::window_skin->get_bild(SKIN_SCROLLBAR_H_KNOB_LEFT)->get_pic()->w+
			            skinverwaltung_t::window_skin->get_bild(SKIN_SCROLLBAR_H_KNOB_RIGHT)->get_pic()->w
			          );
		}

		button_def[knob_index].set_groesse( koord(size,button_t::gui_scrollknob_size.y) );
		button_def[background_index].set_pos( koord(button_t::gui_arrow_left_size.x,0) );
		button_def[background_index].set_groesse( koord(groesse.x-button_t::gui_arrow_left_size.x-button_t::gui_arrow_right_size.x,button_t::gui_scrollbar_size.y) );
	}

	//button_def[knob_index].set_visible( (area > size) );
}



// signals slider drag. If slider hits end, returned amount is smaller.
sint32 scrollbar_t::slider_drag(sint32 amount)
{
	const sint32 area = (type == vertical ? groesse.y : groesse.x)-24; // area will be actual area knob can move in

	const float ratio = (float)area / (float)knob_area;
	amount = (int)( (float)amount / ratio );
	int proposed_offset = knob_offset + amount;

	// 0< possible if content is smaller than window
	const sint32 maximum = max(0,((knob_area-knob_size)+(knob_scroll_discrete?(knob_scroll_amount-1):0)));

	if (proposed_offset<0) {
		proposed_offset = 0;
	}
	if (proposed_offset>maximum) {
		proposed_offset = maximum;
	}

	if (proposed_offset != knob_offset) {
		sint32 o;
		knob_offset = proposed_offset;

		call_listeners((long)knob_offset);
		o = real_knob_position();
		reposition_buttons();
		return real_knob_position() - o;
	}
	return 0;
}



// either arrow buttons is just pressed (or long enough for a repeat event)
void scrollbar_t::button_press(sint32 number)
{
	// 0< possible if content is smaller than window
	const sint32 maximum = max(0,((knob_area-knob_size)+(knob_scroll_discrete?(knob_scroll_amount-1):0)));

	if (number == 0) {
		knob_offset -= knob_scroll_amount;
		if (knob_offset<0) {
			knob_offset = 0;
		}
	}
	else { // number == 1
		knob_offset += knob_scroll_amount;
		if (knob_offset>maximum) {
			knob_offset = maximum;
		}
	}

	if(  knob_scroll_discrete  ) {
		knob_offset -= knob_offset % knob_scroll_amount;
	}

	call_listeners((long)knob_offset);
	reposition_buttons();
}



void scrollbar_t::space_press(sint32 updown) // 0: scroll up/left, 1: scroll down/right
{
	// 0< possible if content is smaller than window
	const sint32 maximum = max(0,((knob_area-knob_size)+(knob_scroll_discrete?(knob_scroll_amount-1):0)));

	if (updown == 0) {
		knob_offset -= knob_size;
		if (knob_offset<0) {
			knob_offset = 0;
		}
	}
	else { // updown == 1
		knob_offset += knob_size;
		if (knob_offset>maximum) {
			knob_offset = maximum;
		}
	}

	if(  knob_scroll_discrete  ) {
		knob_offset -= knob_offset % knob_scroll_amount;
	}

	call_listeners((long)knob_offset);
	reposition_buttons();
}



bool scrollbar_t::infowin_event(const event_t *ev)
{
	const int x = ev->cx;
	const int y = ev->cy;
	int i;
	bool b_button_hit = false;

	// 2003-11-04 hsiegeln added wheelsupport
	// prissi: repaired it, was never doing something ...
	if (IS_WHEELUP(ev) && (type == vertical) != IS_SHIFT_PRESSED(ev)) {
		button_press(0);
	}
	else if (IS_WHEELDOWN(ev) && (type == vertical) != IS_SHIFT_PRESSED(ev)) {
		button_press(1);
	}
	else if (IS_LEFTCLICK(ev)  ||  IS_LEFTREPEAT(ev)) {
		for (i=0;i<3;i++) {
			if (button_def[i].getroffen(x, y)) {
				button_def[i].pressed = true;
				if (i<2) {
					button_press(i);
				}
				b_button_hit = true;
			}
		}

		if (!b_button_hit) {
			if (type == vertical) {
				if (y < real_knob_position()+12) {
					space_press(0);
				}
				else {
					space_press(1);
				}
			}
			else {
				if (x < real_knob_position()+12) {
					space_press(0);
				}
				else {
					space_press(1);
				}
			}
		}
	} else if (IS_LEFTDRAG(ev)) {
		if (button_def[knob_index].getroffen(x,y)) {
			sint32 delta;

			// Hajo: added vertical/horizontal check
			if(type == vertical) {
				delta = ev->my - ev->cy;
			}
			else {
				delta = ev->mx - ev->cx;
			}

			delta = slider_drag(delta);

			// Hajo: added vertical/horizontal check
			if(type == vertical) {
				change_drag_start(0, delta);
			}
			else {
				change_drag_start(delta, 0);
			}
		}
	} else if (IS_LEFTRELEASE(ev)) {
		for (i=0;i<3;i++) {
			if (button_def[i].getroffen(x, y)) {
				button_def[i].pressed = false;
			}
		}

		if(  knob_scroll_discrete  ) {
			knob_offset -= knob_offset % knob_scroll_amount;
		}
	}
	return false;
}



void scrollbar_t::zeichnen(koord pos)
{
	pos += this->pos;
/*
	// if opaque style, display GREY sliding bar backgrounds
	if (type == vertical) {
		display_fillbox_wh(pos.x, pos.y+12, BAR_WIDTH, groesse.y-24, MN_GREY1, true);
	}
	else {
		display_fillbox_wh(pos.x+12, pos.y, groesse.x-24, BAR_WIDTH, MN_GREY1, true);
	}
*/
	button_def[left_top_arrow_index].zeichnen(pos);
	button_def[right_bottom_arrow_index].zeichnen(pos);
	button_def[background_index].pressed = true;
	button_def[background_index].zeichnen(pos);
	button_def[knob_index].pressed = false;
	button_def[knob_index].zeichnen(pos);
}
