/*
 * Scrollbar class
 * Niel Roest
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../../simdebug.h"
#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../simskin.h"
#include "../../macros.h"
#include "../../besch/skin_besch.h"
#include "action_listener.h"
#include "gui_scrollbar.h"

#include "../gui_theme.h"


scrollbar_t::scrollbar_t(type_t type) :
	type(type),
	dragging(false),
	knob_offset(0),
	knob_size(10),
	total_size(20),
	knob_scroll_amount(LINESPACE), // equals one line
	knob_scroll_discrete(false)
{
	visible_mode = show_auto;

	if (type == vertical) {
		size = scr_size(D_SCROLLBAR_WIDTH,40); // 40 = default scrollbar height
		button_def[left_top_arrow_index].set_typ(button_t::arrowup);
		button_def[right_bottom_arrow_index].set_typ(button_t::arrowdown);
	}
	else { // horizontal
		size = scr_size(40,D_SCROLLBAR_HEIGHT); // 40 = default scrollbar length
		button_def[left_top_arrow_index].set_typ(button_t::arrowleft);
		button_def[right_bottom_arrow_index].set_typ(button_t::arrowright);
	}

	button_def[left_top_arrow_index].set_pos(scr_coord(0,0));
	button_def[right_bottom_arrow_index].set_pos(scr_coord(0,0));

	reposition_buttons();
}


void scrollbar_t::set_size(scr_size size)
{
	if (type == vertical) {
		this->size.h = size.h;
	}
	else {
		this->size.w = size.w;
	}
	reposition_buttons();
}


void scrollbar_t::set_knob(sint32 new_visible_size, sint32 new_total_size)
{
	if(  new_visible_size<1  ||  new_total_size<1  ) {
//		dbg->warning("scrollbar_t::set_knob()","size=%i, area=%i not in 1...x",size,area);
	}
	if(  new_total_size != total_size  ) {
		knob_offset = (sint32)( (double)knob_offset * ( (double)new_total_size / (double) total_size ) + 0.5 );
	}
	total_size = max(1,new_total_size);
	knob_size = clamp( new_visible_size, 1, total_size );
	knob_offset = clamp( knob_offset, 0, total_size-knob_size );
	reposition_buttons();
}


// reset variable position and size values of the three buttons
void scrollbar_t::reposition_buttons()
{
	const sint32 area = (type == vertical ?
		size.h-gui_theme_t::gui_arrow_up_size.h-gui_theme_t::gui_arrow_down_size.h :
		size.w-gui_theme_t::gui_arrow_left_size.w-gui_theme_t::gui_arrow_right_size.w);
		// area will be actual area knob can move in

	knob_offset = clamp( knob_offset, 0, total_size-knob_size );

	double size_ratio = (double)knob_size / (double)total_size;
	scr_coord_val length = min( area, (scr_coord_val)( area*size_ratio+0.5) );
	if(  type == vertical  ) {
		length = max( length, D_SCROLL_MIN_HEIGHT );
	}
	else {
		length = max( length, D_SCROLL_MIN_WIDTH );
	}

	double offset_ratio = (double)knob_offset / (double)total_size;
	int offset = (int)( offset_ratio*area+0.5 );
	offset = clamp( offset, 0, area-length );

	if(type == vertical) {
		button_def[left_top_arrow_index].set_pos( scr_coord( (D_SCROLLBAR_WIDTH-gui_theme_t::gui_arrow_up_size.w)/2, 0) );
		button_def[right_bottom_arrow_index].set_pos( scr_coord( (D_SCROLLBAR_WIDTH-gui_theme_t::gui_arrow_down_size.w)/2, size.h-gui_theme_t::gui_arrow_down_size.h) );
		sliderarea.set( 0, gui_theme_t::gui_arrow_up_size.h, D_SCROLLBAR_WIDTH, area );
		knobarea.set( 0, gui_theme_t::gui_arrow_up_size.h+offset, D_SCROLLBAR_WIDTH, length );
	}
	else { // horizontal
		button_def[left_top_arrow_index].set_pos( scr_coord(0,(D_SCROLLBAR_HEIGHT-gui_theme_t::gui_arrow_left_size.h)/2) );
		button_def[right_bottom_arrow_index].set_pos( scr_coord(size.w-gui_theme_t::gui_arrow_right_size.w,(D_SCROLLBAR_HEIGHT-gui_theme_t::gui_arrow_right_size.h)/2) );
		sliderarea.set( gui_theme_t::gui_arrow_up_size.w, 0, area, D_SCROLLBAR_HEIGHT );
		knobarea.set( gui_theme_t::gui_arrow_up_size.w+offset, 0, length, D_SCROLLBAR_HEIGHT );
	}

	full = knobarea.contains( sliderarea );
	set_visible( !full );
}


// actually handles crolling, callback etc.
void scrollbar_t::scroll(sint32 updown)
{
	sint32 new_knob_offset = clamp( knob_offset+updown, 0, total_size-knob_size );
	if(  new_knob_offset != knob_offset  ) {
		knob_offset = new_knob_offset;
		call_listeners((long)knob_offset);
		reposition_buttons();
	}
}


bool scrollbar_t::infowin_event(const event_t *ev)
{
	const int x = ev->cx;
	const int y = ev->cy;
	int i;

	// 2003-11-04 hsiegeln added wheel support
	// prissi: repaired it, was never doing something ...
	if(  IS_WHEELUP(ev)  &&  (type == vertical) != IS_SHIFT_PRESSED(ev)  ) {
		scroll( -knob_scroll_amount );
	}
	else if (IS_WHEELDOWN(ev) && (type == vertical) != IS_SHIFT_PRESSED(ev)) {
		scroll( +knob_scroll_amount );
	}
	else if(  is_visible()  &&  !full ) {
		// don't respond to these messages if not visible
		if(  IS_LEFTCLICK(ev)  ||  IS_LEFTREPEAT(ev)  ) {
			if(  button_def[0].getroffen(x, y)  ) {
				button_def[0].pressed = true;
				scroll( -knob_scroll_amount );
			}
			else if(  button_def[1].getroffen(x, y)  ) {
				button_def[1].pressed = true;
				scroll( +knob_scroll_amount );
			}
			else if(  !dragging  ) {
				// click above/below the slider?
				if(  type == vertical  ) {
					if(  y < knobarea.y  ) {
						scroll( -knob_size);
					}
					else if(  y > knobarea.get_bottom()  ) {
						scroll( +knob_size );
					}
					else {
						dragging = true;
					}
				}
				else {
					if(  x < knobarea.x  ) {
						scroll( -knob_size );
					}
					else if(  x > knobarea.get_right()  ) {
						scroll( +knob_size );
					}
					else {
						dragging = true;
					}
				}
			}
		}
		else if(  IS_LEFTDRAG(ev)  ||  (dragging  &&  IS_LEFT_BUTTON_PRESSED(ev))  ) {
			// now dragging the slider ...
			if(  knobarea.contains( scr_coord(x,y) )  ||  dragging  ) {
				sint32 delta, change;

				// Hajo: added vertical/horizontal check
				if(type == vertical) {
					delta = ev->my - ev->cy;
					change = (sint32)(delta*( (double)total_size / (double) sliderarea.h ) + 0.5);
					delta = knobarea.y;
					scroll( change );
					change_drag_start( 0, knobarea.y-delta );
				}
				else {
					delta = ev->mx - ev->cx;
					change = (sint32)(delta*( (double)total_size / (double) sliderarea.w ) + 0.5);
					delta = knobarea.x;
					scroll( change );
					change_drag_start( knobarea.x-delta, 0 );
				}
				dragging = true;
			}
		}
		else if (IS_LEFTRELEASE(ev)) {
			dragging = false;
			for (i=0;i<2;i++) {
				if (button_def[i].getroffen(x, y)) {
					button_def[i].pressed = false;
				}
			}
		}
	}
	return false;
}



void scrollbar_t::draw(scr_coord pos_par)
{
	// Don't draw control if not visible
	if( (visible_mode != show_auto)  &&  (!is_visible()  ||  (visible_mode == show_never) ) ) {
		return;
	}
	pos_par += pos;

	// Draw place holder if scrollbar is full in auto mode
	if ( visible_mode == show_auto  &&  full  ) {
		// Draw place holder, might be themed...
		display_fillbox_wh(pos_par.x+2, pos_par.y+2, size.w-4, size.h-4, SYSCOL_FACE,false);
		display_ddd_box(pos_par.x+1, pos_par.y+1, size.w-2, size.h-2, SYSCOL_DISABLED_TEXT,SYSCOL_DISABLED_TEXT,false);
		return;
	}

	// Draw scrollbar as normal
	button_def[left_top_arrow_index].draw(pos_par);
	button_def[right_bottom_arrow_index].draw(pos_par);

	// now backgroudn and slider
	if(  type == vertical  ) {
		display_img_stretch( gui_theme_t::v_scroll_back_tiles, scr_rect( pos_par + sliderarea.get_pos(), sliderarea.get_size() ) );
		display_img_stretch( gui_theme_t::v_scroll_knob_tiles, scr_rect( pos_par + knobarea.get_pos(), knobarea.get_size() ) );
	}
	else {
		display_img_stretch( gui_theme_t::h_scroll_back_tiles, scr_rect( pos_par + sliderarea.get_pos(), sliderarea.get_size() ) );
		display_img_stretch( gui_theme_t::h_scroll_knob_tiles, scr_rect( pos_par + knobarea.get_pos(), knobarea.get_size() ) );
	}
}
