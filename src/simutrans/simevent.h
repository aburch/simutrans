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
	EVENT_KEYDOWN        =   1,
	EVENT_KEYUP          =   2,
	EVENT_STRING         =   3,  ///< instead of a single character a ev_ptr points to an utf8 string
	EVENT_CLICK          =   4,
	EVENT_DOUBLE_CLICK   =   5,  ///< 2 consecutive sequences of click-release
	EVENT_TRIPLE_CLICK   =   6,  ///< 3 consecutive sequences of click-release
	EVENT_RELEASE        =   7,
	EVENT_MOVE           =   8,
	EVENT_DRAG           =   9,

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
enum sim_keycode_t
{
	SIM_KEYCODE_BACKSPACE   =   8,
	SIM_KEYCODE_TAB         =   9,
	SIM_KEYCODE_ENTER       =  13,
	SIM_KEYCODE_ESCAPE      =  27,
	SIM_KEYCODE_SPACE       =  32,
	SIM_KEYCODE_DELETE      = 127,

	/* Function keys */
	SIM_KEYCODE_F1          = 256,
	SIM_KEYCODE_F2          = 257,
	SIM_KEYCODE_F3          = 258,
	SIM_KEYCODE_F4          = 259,
	SIM_KEYCODE_F5          = 260,
	SIM_KEYCODE_F6          = 261,
	SIM_KEYCODE_F7          = 262,
	SIM_KEYCODE_F8          = 263,
	SIM_KEYCODE_F9          = 264,
	SIM_KEYCODE_F10         = 265,
	SIM_KEYCODE_F11         = 266,
	SIM_KEYCODE_F12         = 267,
	SIM_KEYCODE_F13         = 268,
	SIM_KEYCODE_F14         = 269,
	SIM_KEYCODE_F15         = 270,

	/* other navigation keys */
	SIM_KEYCODE_HOME        = 275,
	SIM_KEYCODE_END         = 276,
	SIM_KEYCODE_PGUP        = 277,
	SIM_KEYCODE_PGDN        = 278,
	SIM_KEYCODE_SCROLLLOCK  = 279,

	SIM_KEYCODE_PAUSE       = 279,

	/* arrow (direction) keys */
	SIM_KEYCODE_NUMPAD_BASE = 280, // 0 on keypad
	SIM_KEYCODE_DOWNLEFT,
	SIM_KEYCODE_DOWN,
	SIM_KEYCODE_DOWNRIGHT,
	SIM_KEYCODE_LEFT,
	SIM_KEYCODE_CENTER,
	SIM_KEYCODE_RIGHT,
	SIM_KEYCODE_UPLEFT,
	SIM_KEYCODE_UP,
	SIM_KEYCODE_UPRIGHT,
};


#define SIM_MOD_NONE   0
#define SIM_MOD_SHIFT  (1u << 0)
#define SIM_MOD_CTRL   (1u << 1)


/**
 * Slight explanation of event_t structure:
 * ev_class and ev_code speak for itself.
 * ev_class = EVENT_NONE:      nothing defined
 * ev_class = EVENT_KEYBOARD:  code = key pressed (released key is ignored)
 * ev_class = EVENT_CLICK:     mouse_pos/click_pos point to mouse click place,
 *                             code = pressed mouse button
 * ev_class = EVENT_RELEASE:   click_pos points to mouse click place,
 *                             mouse_pos points to mouse release place,
 *                             code = mouse release button
 * ev_class = EVENT_MOVE:      click_pos is last click place, mouse_pos is to.
 * ev_class = EVENT_DRAG:      click_pos is last click place, mouse_pos is to,
 *                             code = mouse button
 * ev_class = EVENT_REPEAT:    code = button pressed
 */
struct event_t
{
public:
	explicit event_t(event_class_t event_class = EVENT_NONE);

public:
	/**
	 * Move event origin. Useful when transferring events to sub-components.
	 * @param delta position of new origin relative to the old origin.
	 */
	void move_origin(scr_coord delta);

public:
	event_class_t ev_class = EVENT_NONE;
	union
	{
		unsigned int ev_code;
		void *ev_ptr;
	};

	// Mouse position
	scr_coord mouse_pos = { 0, 0 };

	/// position of last mouse click
	scr_coord click_pos = { 0, 0 };

	/// new window size for SYSTEM_RESIZE
	scr_size new_window_size = { 0, 0 };

	/// bitset indicating which mouse buttons are pressed
	int mouse_button_state = 0;

	/// mod key (SHIFT, CTRL etc) pressed while event as triggered
	unsigned int ev_key_mod = SIM_MOD_NONE;
};


static inline bool IS_KEYDOWN    (const event_t *ev) { return ev->ev_class == EVENT_KEYDOWN;  }
static inline bool IS_KEYUP      (const event_t *ev) { return ev->ev_class == EVENT_KEYUP;    }
static inline bool IS_KEYBOARD   (const event_t *ev) { return IS_KEYDOWN(ev) || IS_KEYUP(ev); }

static inline bool IS_MOUSE(const event_t *ev) { return ev->ev_class >= EVENT_CLICK && ev->ev_class <= EVENT_DRAG; }

static inline bool IS_LEFTCLICK  (const event_t *ev) { return ev->ev_class == EVENT_CLICK        && ev->ev_code == MOUSE_LEFTBUTTON; }
static inline bool IS_LEFTRELEASE(const event_t *ev) { return ev->ev_class == EVENT_RELEASE      && ev->ev_code == MOUSE_LEFTBUTTON; }
static inline bool IS_LEFTDRAG   (const event_t *ev) { return ev->ev_class == EVENT_DRAG         && ev->ev_code == MOUSE_LEFTBUTTON; }
static inline bool IS_LEFTDBLCLK (const event_t *ev) { return ev->ev_class == EVENT_DOUBLE_CLICK && ev->ev_code == MOUSE_LEFTBUTTON; }
static inline bool IS_LEFTTPLCLK (const event_t *ev) { return ev->ev_class == EVENT_TRIPLE_CLICK && ev->ev_code == MOUSE_LEFTBUTTON; }

static inline bool IS_RIGHTCLICK  (const event_t *ev) { return ev->ev_class == EVENT_CLICK        && ev->ev_code == MOUSE_RIGHTBUTTON; }
static inline bool IS_RIGHTRELEASE(const event_t *ev) { return ev->ev_class == EVENT_RELEASE      && ev->ev_code == MOUSE_RIGHTBUTTON; }
static inline bool IS_RIGHTDRAG   (const event_t *ev) { return ev->ev_class == EVENT_DRAG         && ev->ev_code == MOUSE_RIGHTBUTTON; }
static inline bool IS_RIGHTDBLCLK (const event_t *ev) { return ev->ev_class == EVENT_DOUBLE_CLICK && ev->ev_code == MOUSE_RIGHTBUTTON; }
static inline bool IS_RIGHTTPLCLK (const event_t *ev) { return ev->ev_class == EVENT_TRIPLE_CLICK && ev->ev_code == MOUSE_RIGHTBUTTON; }

static inline bool IS_WHEELUP     (const event_t *ev) { return ev->ev_class == EVENT_CLICK        && ev->ev_code == MOUSE_WHEELUP;   }
static inline bool IS_WHEELDOWN   (const event_t *ev) { return ev->ev_class == EVENT_CLICK        && ev->ev_code == MOUSE_WHEELDOWN; }

static inline bool IS_WINDOW_RESIZE       (const event_t *ev) { return ev->ev_class == WINDOW_RESIZE;        }
static inline bool IS_WINDOW_MAKE_MIN_SIZE(const event_t *ev) { return ev->ev_class == WINDOW_MAKE_MIN_SIZE; }
static inline bool IS_WINDOW_CHOOSE_NEXT  (const event_t *ev) { return ev->ev_class == WINDOW_CHOOSE_NEXT;   }

// This function is to determine if the event should be also handled by children of containers.
static inline bool DOES_WINDOW_CHILDREN_NEED(const event_t *ev) { return ev->ev_class == INFOWIN || ev->ev_class == WINDOW_RESIZE || ev->ev_class == WINDOW_MAKE_MIN_SIZE; }

static inline bool IS_WINDOW_TOP(const event_t *ev) { return ev->ev_class == INFOWIN || ev->ev_code == WIN_TOP; }

static inline bool IS_LEFT_BUTTON_PRESSED  (const event_t *ev) { return ev->mouse_button_state & MOUSE_LEFTBUTTON;  }
static inline bool IS_RIGHT_BUTTON_PRESSED (const event_t *ev) { return ev->mouse_button_state & MOUSE_RIGHTBUTTON; }
static inline bool IS_MIDDLE_BUTTON_PRESSED(const event_t *ev) { return ev->mouse_button_state & MOUSE_MIDBUTTON;   }

static inline bool IS_SHIFT_PRESSED  (const event_t *ev) { return ev->ev_key_mod & SIM_MOD_SHIFT; }
static inline bool IS_CONTROL_PRESSED(const event_t *ev) { return ev->ev_key_mod & SIM_MOD_CTRL;  }


/// Return one event. Does *not* wait.
void display_poll_event(event_t *event);

void change_drag_start(scr_coord offset);
void set_click_xy(scr_coord_val x, scr_coord_val y);

int event_get_last_control_shift();
event_class_t last_meta_event_get_class();

/// Get mouse pointer position. Implementation in simsys.cc
scr_coord get_mouse_pos();

/// Adds new events to be processed.
void queue_event(event_t *event);

#endif
