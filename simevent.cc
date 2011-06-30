/*
 * system-independant event handling routines
 * Hj. Malthaner, Jan. 2001
 */

#include "simevent.h"
#include "simsys.h"


static int cx = -1; // coordinates of last mouse click event
static int cy = -1; // initialised to "nowhere"
static int control_shift_state = 0;	// none pressed
static event_t meta_event(EVENT_NONE);	// Knightly : for storing meta-events like double-clicks and triple-clicks


int event_get_last_control_shift(void)
{
	// shift = 1
	// ctrl  = 2
	return control_shift_state & 0x03;
}


/**
 * each drag event contains the origin of the first click.
 * if the window is being dragged, it is convenient to change this
 * so the origin keeps pointing to the window top bar.
 *  Mainly to prevent copied, double code.
 */
void change_drag_start(int x, int y)
{
	cx += x;
	cy += y;
}


static void fill_event(struct event_t *ev)
{
	// Knightly : variables for detecting double-clicks and triple-clicks
	const  unsigned long interval = 400;
	static unsigned int  prev_ev_class = EVENT_NONE;
	static unsigned int  prev_ev_code = 0;
	static unsigned long prev_ev_time = 0;
	static unsigned char repeat_count = 0;	// number of consecutive sequences of click-release

	// for autorepeat buttons we track button state, press time and a repeat time
	// code by Niels Roest and Hj. Maltahner

	static int  pressed_buttons = 0; // assume: at startup no button pressed (needed for some backends)
	static unsigned long lb_time = 0;
	static long repeat_time = 500;

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
					break;

				case SIM_MOUSE_RIGHTBUTTON:
					ev->ev_class = EVENT_CLICK;
					pressed_buttons |= MOUSE_RIGHTBUTTON;
					ev->ev_code = MOUSE_RIGHTBUTTON;
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
					break;

				case SIM_MOUSE_MIDBUTTON:
					ev->ev_class = EVENT_CLICK;
					pressed_buttons |= MOUSE_MIDBUTTON;
					ev->ev_code = MOUSE_MIDBUTTON;
					ev->cx = cx = sys_event.mx;
					ev->cy = cy = sys_event.my;
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
					break;

				case SIM_MOUSE_RIGHTUP:
					ev->ev_class = EVENT_RELEASE;
					ev->ev_code = MOUSE_RIGHTBUTTON;
					pressed_buttons &= ~MOUSE_RIGHTBUTTON;
					break;

				case SIM_MOUSE_MIDUP:
					ev->ev_class = EVENT_RELEASE;
					ev->ev_code = MOUSE_MIDBUTTON;
					pressed_buttons &= ~MOUSE_MIDBUTTON;
					break;
			}
			break;

		case SIM_MOUSE_MOVE:
			if (sys_event.mb) { // drag
				ev->ev_class = EVENT_DRAG;
				ev->ev_code  = sys_event.mb;
			} else { // move
				ev->ev_class = EVENT_MOVE;
				ev->ev_code  = 0;
			}
			break;

		case SIM_SYSTEM:
			ev->ev_class = EVENT_SYSTEM;
			ev->ev_code  = sys_event.code;
			break;
	}

	// Knightly : check for double-clicks and triple-clicks
	const unsigned long curr_time = dr_time();
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
			repeat_count = 0;	// reset -> start over again
		}
	}
	else if(  ev->ev_class!=EVENT_NONE  &&  prev_ev_class!=EVENT_NONE  ) {
		// case : broken click-release sequence -> simply reset
		prev_ev_class = EVENT_NONE;
		prev_ev_code = 0;
		prev_ev_time = 0;
		repeat_count = 0;
	}

	if (IS_LEFTCLICK(ev)) {
		// remember button press
		lb_time = curr_time;
		repeat_time = 400;
	} else if (pressed_buttons == 0) {
		lb_time = 0;
	} else { // the else is to prevent race conditions
		/* Hajo: this would transform non-left button presses always
		 * to repeat events. I need right button clicks.
		 * I have no idea how this can be done cleanly, currently just
		 * disabling the repeat feature for non-left buttons
		 */
		if (pressed_buttons == MOUSE_LEFTBUTTON) {
			if (curr_time > lb_time + repeat_time) {
				repeat_time = 100;
				lb_time = curr_time;
				ev->ev_class = EVENT_REPEAT;
				ev->ev_code = pressed_buttons;
			}
		}
	}

	ev->button_state = pressed_buttons;
}


/**
 * Holt ein Event ohne zu warten
 * @author Hj. Malthaner
 */
void display_poll_event(struct event_t *ev)
{
	// Knightly : if there is any pending meta-event, consume it instead of fetching a new event from the system
	if(  meta_event.ev_class!=EVENT_NONE  ) {
		*ev = meta_event;
		meta_event.ev_class = EVENT_NONE;
	}
	else {
		GetEventsNoWait();
		fill_event(ev);
		// prepare for next event
		sys_event.type = SIM_NOEVENT;
		sys_event.code = 0;
	}
}


/**
 * Holt ein Event mit warten
 * @author Hj. Malthaner
 */
void display_get_event(struct event_t *ev)
{
	// Knightly : if there is any pending meta-event, consume it instead of fetching a new event from the system
	if(  meta_event.ev_class!=EVENT_NONE  ) {
		*ev = meta_event;
		meta_event.ev_class = EVENT_NONE;
	}
	else {
		GetEvents();
		fill_event(ev);
		// prepare for next event
		sys_event.type = SIM_NOEVENT;
		sys_event.code = 0;
	}
}
