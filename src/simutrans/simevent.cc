/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simevent.h"
#include "sys/simsys.h"
#include "tpl/slist_tpl.h"

// system-independent event handling routines

static int cx = -1; // coordinates of last mouse click event
static int cy = -1; // initialised to "nowhere"

static bool is_dragging = false;

static int control_shift_state = 0; // none pressed
static event_t meta_event(EVENT_NONE); // for storing meta-events like double-clicks and triple-clicks
static event_class_t last_meta_class = EVENT_NONE;
static slist_tpl<event_t *> queued_events;


event_t::event_t(event_class_t event_class) :
	ev_class(event_class),
	ev_code(0),
	mx(0), my(0),
	cx(0), cy(0),
	button_state(0),
	ev_key_mod(SIM_MOD_NONE)
{
}


void event_t::move_origin(scr_coord delta)
{
	mx -= delta.x;
	cx -= delta.x;
	my -= delta.y;
	cy -= delta.y;
}


int event_get_last_control_shift()
{
	// shift = 1
	// ctrl  = 2
	return control_shift_state & 0x03;
}


event_class_t last_meta_event_get_class()
{
	return last_meta_class;
}


/**
 * each drag event contains the origin of the first click.
 * if the window is being dragged, it is convenient to change this
 * so the origin keeps pointing to the window top bar.
 *  Mainly to prevent copied, double code.
 */
void change_drag_start(scr_coord_val x, scr_coord_val y)
{
	cx += x;
	cy += y;
}


// since finger events work with absolute coordinates
void set_click_xy(scr_coord_val x, scr_coord_val y)
{
	cx = x;
	cy = y;
}


static void fill_event(event_t* const ev)
{
	// variables for detecting double-clicks and triple-clicks
	const  uint32        interval = 400;
	static unsigned int  prev_ev_class = EVENT_NONE;
	static unsigned int  prev_ev_code = 0;
	static uint32        prev_ev_time = 0;
	static unsigned char repeat_count = 0; // number of consecutive sequences of click-release

	// for autorepeat buttons we track button state, press time and a repeat time

	static int  pressed_buttons = 0; // assume: at startup no button pressed (needed for some backends)
	static uint32 lb_time = 0;
	static uint32 repeat_time = 500;

	ev->ev_class = EVENT_NONE;

	ev->mx = sys_event.mx;
	ev->my = sys_event.my;

	ev->cx = cx;
	ev->cy = cy;

	// always put key mod code into event
	ev->ev_key_mod = sys_event.key_mod;
	control_shift_state = sys_event.key_mod;

	switch (sys_event.type) {
		case SIM_KEYBOARD:
			ev->ev_class = EVENT_KEYBOARD;
			ev->ev_code  = sys_event.code;
			break;

		case SIM_STRING:
			ev->ev_class = EVENT_STRING;
			ev->ev_ptr   = sys_event.ptr;
			break;

		case SIM_MOUSE_BUTTONS:
			// press only acknowledged when no buttons are pressed
			pressed_buttons = sys_event.mb;
			switch (sys_event.code) {
				case SIM_MOUSE_LEFTBUTTON:
					ev->ev_class = EVENT_CLICK;
					pressed_buttons |= MOUSE_LEFTBUTTON;
					ev->ev_code = MOUSE_LEFTBUTTON;
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
					is_dragging = true;
					break;

				case SIM_MOUSE_RIGHTBUTTON:
					ev->ev_class = EVENT_CLICK;
					pressed_buttons |= MOUSE_RIGHTBUTTON;
					ev->ev_code = MOUSE_RIGHTBUTTON;
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
					is_dragging = true;
					break;

				case SIM_MOUSE_MIDBUTTON:
					ev->ev_class = EVENT_CLICK;
					pressed_buttons |= MOUSE_MIDBUTTON;
					ev->ev_code = MOUSE_MIDBUTTON;
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
					is_dragging = true;
					break;

				case SIM_MOUSE_WHEELUP:
					ev->ev_class = EVENT_CLICK;
					ev->ev_code = MOUSE_WHEELUP;
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
					break;

				case SIM_MOUSE_WHEELDOWN:
					ev->ev_class = EVENT_CLICK;
					ev->ev_code = MOUSE_WHEELDOWN;
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
					break;

				case SIM_MOUSE_LEFTUP:
					ev->ev_class = EVENT_RELEASE;
					ev->ev_code = MOUSE_LEFTBUTTON;
					pressed_buttons &= ~MOUSE_LEFTBUTTON;
					is_dragging = false;
					break;

				case SIM_MOUSE_RIGHTUP:
					ev->ev_class = EVENT_RELEASE;
					ev->ev_code = MOUSE_RIGHTBUTTON;
					pressed_buttons &= ~MOUSE_RIGHTBUTTON;
					is_dragging = false;
					break;

				case SIM_MOUSE_MIDUP:
					ev->ev_class = EVENT_RELEASE;
					ev->ev_code = MOUSE_MIDBUTTON;
					pressed_buttons &= ~MOUSE_MIDBUTTON;
					is_dragging = false;
					break;
			}
			break;

		case SIM_MOUSE_MOVE:
			if (sys_event.mb) { // drag
				ev->ev_class = EVENT_DRAG;
				ev->ev_code  = sys_event.mb;
				if(  !is_dragging) {
					// we just start dragging, since the click was not delivered it seems
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
					is_dragging = true;
				}
			}
			else { // move
				ev->ev_class = EVENT_MOVE;
				ev->ev_code  = 0;
				is_dragging = false;
			}
			break;

		case SIM_SYSTEM:
			ev->ev_class        = EVENT_SYSTEM;
			ev->ev_code         = sys_event.code;
			ev->new_window_size = scr_size(sys_event.new_window_size_w, sys_event.new_window_size_h);
			break;
	}

	// check for double-clicks and triple-clicks
	const uint32 curr_time = dr_time();
	if(  ev->ev_class==EVENT_CLICK  ) {
		if(  prev_ev_class==EVENT_RELEASE  &&  prev_ev_code==ev->ev_code  &&  curr_time-prev_ev_time<=interval  ) {
			// case : a mouse click which forms an unbroken sequence with the previous clicks and releases
			prev_ev_class = EVENT_CLICK;
			prev_ev_time = curr_time;
		}
		else {
			// case : initial click or broken click-release sequence -> prepare for the start of a new sequence
			prev_ev_class = EVENT_CLICK;
			prev_ev_code = ev->ev_code;
			prev_ev_time = curr_time;
			repeat_count = 0;
		}
	}
	else if(  ev->ev_class==EVENT_RELEASE  &&  prev_ev_class==EVENT_CLICK  &&  prev_ev_code==ev->ev_code  &&  curr_time-prev_ev_time<=interval  ) {
		// case : a mouse release which forms an unbroken sequence with the previous clicks and releases
		prev_ev_class = EVENT_RELEASE;
		prev_ev_time = curr_time;
		++repeat_count;

		// create meta-events where necessary
		if(  repeat_count==2  ) {
			// case : double-click
			meta_event = *ev;
			meta_event.ev_class = EVENT_DOUBLE_CLICK;
		}
		else if(  repeat_count==3  ) {
			// case : triple-click
			meta_event = *ev;
			meta_event.ev_class = EVENT_TRIPLE_CLICK;
			repeat_count = 0; // reset -> start over again
		}
	}
	else if(  ev->ev_class!=EVENT_NONE  &&  prev_ev_class!=EVENT_NONE  ) {
		// case : broken click-release sequence -> simply reset
		prev_ev_class = EVENT_NONE;
		prev_ev_code = 0;
		prev_ev_time = 0;
		repeat_count = 0;
	}

	ev->button_state = pressed_buttons;
}


void display_poll_event(event_t* const ev)
{
	if( !queued_events.empty() ) {
		// We have a queued (injected programatically) event, return it.
		event_t *elem = queued_events.remove_first();
		*ev = *elem;
		delete elem;
		return ;
	}
	// if there is any pending meta-event, consume it instead of fetching a new event from the system
	if(  meta_event.ev_class!=EVENT_NONE  ) {
		*ev = meta_event;
		last_meta_class = meta_event.ev_class;
		meta_event.ev_class = EVENT_NONE;
	}
	else {
		last_meta_class = EVENT_NONE;
		GetEvents();
		fill_event(ev);
		// prepare for next event
		sys_event.type = SIM_NOEVENT;
		sys_event.code = 0;
	}
}


void queue_event(event_t *events)
{
	queued_events.append(events);
}
