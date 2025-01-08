/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMEVENT_H
#define SIMEVENT_H


#include "display/scr_coord.h"


/* Messageverarbeitung */

/* Event Classes */

enum event_class_t
{
	EVENT_NONE           =   0,
	EVENT_KEYBOARD       =   1,
	EVENT_STRING         =   2,  ///< instead of a single character a ev_ptr points to an utf8 string
	EVENT_CLICK          =   3,
	EVENT_DOUBLE_CLICK   =   4,  ///< 2 consecutive sequences of click-release
	EVENT_TRIPLE_CLICK   =   5,  ///< 3 consecutive sequences of click-release
	EVENT_RELEASE        =   6,
	EVENT_MOVE           =   7,
	EVENT_DRAG           =   8,
	EVENT_REPEAT         =   9,

	INFOWIN              =  10,  ///< window event, i.e. WIN_OPEN, WIN_CLOSE
	WINDOW_RESIZE        =  11,
	WINDOW_MAKE_MIN_SIZE =  12,
	WINDOW_CHOOSE_NEXT   =  13,

	EVENT_SYSTEM         = 254,
	IGNORE_EVENT         = 255
};


/* Event Codes */

#define MOUSE_LEFTBUTTON              1
#define MOUSE_RIGHTBUTTON             2
#define MOUSE_MIDBUTTON               4
#define MOUSE_WHEELUP                 8
#define MOUSE_WHEELDOWN              16

#define WIN_OPEN                      1
#define WIN_CLOSE                     2
#define WIN_TOP                       3
#define WIN_UNTOP                     4  // losing focus

#define NEXT_WINDOW                   1
#define PREV_WINDOW                   2

#define SYSTEM_QUIT                   1
#define SYSTEM_RESIZE                 2
#define SYSTEM_RELOAD_WINDOWS         3
#define SYSTEM_THEME_CHANGED          4

/* normal keys have range 0-255, special key follow above 255 */
/* other would be better for true unicode support :( */

/* control keys */
#define SIM_KEY_BACKSPACE             8
#define SIM_KEY_TAB                   9
#define SIM_KEY_ENTER                13
#define SIM_KEY_ESCAPE               27
#define SIM_KEY_SPACE                32
#define SIM_KEY_DELETE              127
#define SIM_KEY_PAUSE               279

/* arrow (direction) keys */
enum {
	SIM_KEY_NUMPAD_BASE = 280, // 0 on keypad
	SIM_KEY_DOWNLEFT,
	SIM_KEY_DOWN,
	SIM_KEY_DOWNRIGHT,
	SIM_KEY_LEFT,
	SIM_KEY_CENTER,
	SIM_KEY_RIGHT,
	SIM_KEY_UPLEFT,
	SIM_KEY_UP,
	SIM_KEY_UPRIGHT
};

/* other navigation keys */
#define SIM_KEY_HOME                275
#define SIM_KEY_END                 276
#define SIM_KEY_PGUP                277
#define SIM_KEY_PGDN                278
#define SIM_KEY_SCROLLLOCK          279

/* Function keys */
#define SIM_KEY_F1                  256
#define SIM_KEY_F2                  257
#define SIM_KEY_F3                  258
#define SIM_KEY_F4                  259
#define SIM_KEY_F5                  260
#define SIM_KEY_F6                  261
#define SIM_KEY_F7                  262
#define SIM_KEY_F8                  263
#define SIM_KEY_F9                  264
#define SIM_KEY_F10                 265
#define SIM_KEY_F11                 266
#define SIM_KEY_F12                 267
#define SIM_KEY_F13                 268
#define SIM_KEY_F14                 269
#define SIM_KEY_F15                 270

#define SIM_MOD_NONE   0
#define SIM_MOD_SHIFT  (1u << 0)
#define SIM_MOD_CTRL   (1u << 1)


/* macros */
#define IS_MOUSE(ev) ((ev)->ev_class >= EVENT_CLICK && (ev)->ev_class <= EVENT_DRAG)

#define IS_LEFTCLICK(ev)              ((ev)->ev_class == EVENT_CLICK        && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTRELEASE(ev)            ((ev)->ev_class == EVENT_RELEASE      && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTDRAG(ev)               ((ev)->ev_class == EVENT_DRAG         && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTREPEAT(ev)             ((ev)->ev_class == EVENT_REPEAT       && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTDBLCLK(ev)             ((ev)->ev_class == EVENT_DOUBLE_CLICK && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTTPLCLK(ev)             ((ev)->ev_class == EVENT_TRIPLE_CLICK && (ev)->ev_code == MOUSE_LEFTBUTTON)

#define IS_RIGHTCLICK(ev)             ((ev)->ev_class == EVENT_CLICK        && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTRELEASE(ev)           ((ev)->ev_class == EVENT_RELEASE      && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTDRAG(ev)              ((ev)->ev_class == EVENT_DRAG         && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTDBLCLK(ev)            ((ev)->ev_class == EVENT_DOUBLE_CLICK && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTTPLCLK(ev)            ((ev)->ev_class == EVENT_TRIPLE_CLICK && (ev)->ev_code == MOUSE_RIGHTBUTTON)

#define IS_WHEELUP(ev)                ((ev)->ev_class == EVENT_CLICK        && (ev)->ev_code == MOUSE_WHEELUP)
#define IS_WHEELDOWN(ev)              ((ev)->ev_class == EVENT_CLICK        && (ev)->ev_code == MOUSE_WHEELDOWN)

#define IS_WINDOW_RESIZE(ev)          ((ev)->ev_class == WINDOW_RESIZE)
#define IS_WINDOW_MAKE_MIN_SIZE(ev)   ((ev)->ev_class == WINDOW_MAKE_MIN_SIZE)
#define IS_WINDOW_CHOOSE_NEXT(ev)     ((ev)->ev_class == WINDOW_CHOOSE_NEXT)

// This macro is to determine if the event should be also handled by children of containers.
#define DOES_WINDOW_CHILDREN_NEED(ev) ((ev)->ev_class == INFOWIN || (ev)->ev_class == WINDOW_RESIZE || (ev)->ev_class == WINDOW_MAKE_MIN_SIZE )

#define IS_WINDOW_TOP(ev)             ((ev)->ev_class == INFOWIN || (ev)->ev_code == WIN_TOP)

#define IS_LEFT_BUTTON_PRESSED(ev)     ((ev)->button_state&1)
#define IS_RIGHT_BUTTON_PRESSED(ev)   (((ev)->button_state&2)>>1)
#define IS_MIDDLE_BUTTON_PRESSED(ev)  (((ev)->button_state&4)>>2)

#define IS_SHIFT_PRESSED(ev)          (((ev)->ev_key_mod&SIM_MOD_SHIFT) != 0)
#define IS_CONTROL_PRESSED(ev)        (((ev)->ev_key_mod&SIM_MOD_CTRL ) != 0)


/**
 * Slight explanation of event_t structure:
 * ev_class and ev_code speak for itself.
 * ev_class = EVENT_NONE:      nothing defined
 * ev_class = EVENT_KEYBOARD:  code = key pressed (released key is ignored)
 * ev_class = EVENT_CLICK:     mx/my/cx/cy point to mouse click place,
 *                             code = pressed mouse button
 * ev_class = EVENT_RELEASE:   cx/cy point to mouse click place,
 *                             mx/my point to mouse release place,
 *                             code = mouse release button
 * ev_class = EVENT_MOVE:      cx/cy is last click place, mx/my is to.
 * ev_class = EVENT_DRAG:      cx/cy is last click place, mx/my is to,
 *                             code = mouse button
 * ev_class = EVENT_REPEAT:    code = button pressed
 */
struct event_t
{
public:
	event_t(event_class_t event_class = EVENT_NONE);

public:
	/**
	 * Move event origin. Useful when transferring events to sub-components.
	 * @param delta position of new origin relative to the old origin.
	 */
	void move_origin(scr_coord delta);

public:
	event_class_t ev_class;
	union
	{
		unsigned int ev_code;
		void *ev_ptr;
	};

	scr_coord_val mx, my;

	/// position of last mouse click
	scr_coord_val cx, cy;

	/// new window size for SYSTEM_RESIZE
	scr_size new_window_size;

	/// current mouse button state
	int button_state;

	/// mod key (SHIFT; ALT; CTRL; etc) pressed while event as triggered
	unsigned int ev_key_mod;
};


/// Return one event. Does *not* wait.
void display_poll_event(event_t*);

/// Wait for one event, and return it.
void display_get_event(event_t*);
void change_drag_start(int x, int y);
void set_click_xy(scr_coord_val x, scr_coord_val y);

int event_get_last_control_shift();
event_class_t last_meta_event_get_class();

/// Get mouse pointer position. Implementation in simsys.cc
int get_mouse_x();
int get_mouse_y();

/// Adds new events to be processed.
void queue_event(event_t *event);

#endif
