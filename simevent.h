/*
 * Header for system independant event handling routines
 *
 * Hj. Malthaner
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef simevent_h
#define simevent_h

/* Messageverarbeitung */

/* Event Classes */

#define EVENT_NONE                    0
#define EVENT_KEYBOARD                1
#define EVENT_CLICK                   2
#define EVENT_RELEASE                 3
#define EVENT_MOVE                    4
#define EVENT_DRAG                    5
#define EVENT_REPEAT                  6
#define EVENT_DOUBLE_CLICK            7  // Knightly: 2 consecutive sequences of click-release
#define EVENT_TRIPLE_CLICK            8  // Knightly: 3 consecutive sequences of click-release

#define INFOWIN                       9  // Hajo: window event, i.e. WIN_OPEN, WIN_CLOSE
#define WINDOW_RESIZE                10  // 19-may-02	markus weber   added
#define WINDOW_MAKE_MIN_SIZE         11  // 11-mar-03	(Mathew Hounsell) Added
#define WINDOW_CHOOSE_NEXT           12	 // @author Volker Meyer @date  11.06.2003

#define EVENT_SYSTEM                254
#define IGNORE_EVENT                255

/* Event Codes */

#define MOUSE_LEFTBUTTON              1
#define MOUSE_RIGHTBUTTON             2
#define MOUSE_MIDBUTTON               4
#define MOUSE_WHEELUP                 8  // hsiegeln 2003-11-04 added
#define MOUSE_WHEELDOWN              16  // hsiegeln 2003-11-04 added

#define WIN_OPEN                      1
#define WIN_CLOSE                     2
#define WIN_TOP                       3
#define WIN_UNTOP                     4  // loosing focus

#define NEXT_WINDOW                   1
#define PREV_WINDOW                   2

#define SYSTEM_QUIT                   1
#define SYSTEM_RESIZE                 2
#define SYSTEM_RELOAD_WINDOWS         3

/* normal keys have range 0-255, special key follow above 255 */
/* other would be better for true unicode support :( */

/* control keys */
#define SIM_KEY_BACKSPACE             8
#define SIM_KEY_TAB                   9
#define SIM_KEY_ENTER                13
#define SIM_KEY_ESCAPE               27
#define SIM_KEY_SPACE                32
#define SIM_KEY_DELETE              127

/* arrow (direction) keys */
#define SIM_KEY_UP                  273
#define SIM_KEY_DOWN                274
#define SIM_KEY_RIGHT               275
#define SIM_KEY_LEFT                276

/* other navigation keys */
#define	SIM_KEY_HOME			    278
#define SIM_KEY_END				    279
#define SIM_KEY_PGUP                 62
#define SIM_KEY_PGDN                 60

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


/* macros */

#define IS_LEFTCLICK(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTRELEASE(ev) ((ev)->ev_class == EVENT_RELEASE && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTDRAG(ev) ((ev)->ev_class == EVENT_DRAG && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTREPEAT(ev) ((ev)->ev_class == EVENT_REPEAT && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTDBLCLK(ev) ((ev)->ev_class == EVENT_DOUBLE_CLICK && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTTPLCLK(ev) ((ev)->ev_class == EVENT_TRIPLE_CLICK && (ev)->ev_code == MOUSE_LEFTBUTTON)

#define IS_RIGHTCLICK(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTRELEASE(ev) ((ev)->ev_class == EVENT_RELEASE && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTDRAG(ev) ((ev)->ev_class == EVENT_DRAG && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTDBLCLK(ev) ((ev)->ev_class == EVENT_DOUBLE_CLICK && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTTPLCLK(ev) ((ev)->ev_class == EVENT_TRIPLE_CLICK && (ev)->ev_code == MOUSE_RIGHTBUTTON)

#define IS_WINDOW_RESIZE(ev) ((ev)->ev_class == WINDOW_RESIZE) //19-may-02	markus weber	added
#define IS_WINDOW_MAKE_MIN_SIZE(ev) ((ev)->ev_class == WINDOW_MAKE_MIN_SIZE) // 11-Mar-03 (Mathew Hounsell) Added
#define IS_WINDOW_CHOOSE_NEXT(ev) ((ev)->ev_class == WINDOW_CHOOSE_NEXT) // 11-Mar-03 (Mathew Hounsell) Added

#define IS_WHEELUP(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_WHEELUP)
#define IS_WHEELDOWN(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_WHEELDOWN)

// This macro is to determine if the event should be also handled by children of containers.
#define DOES_WINDOW_CHILDREN_NEED(ev) ((ev)->ev_class == INFOWIN || (ev)->ev_class == WINDOW_RESIZE || (ev)->ev_class == WINDOW_MAKE_MIN_SIZE ) // 11-Mar-03 (Mathew Hounsell) Added

#define IS_WINDOW_TOP(ev) ((ev)->ev_class == INFOWIN || (ev)->ev_code == WIN_TOP)

#define IS_LEFT_BUTTON_PRESSED(ev) ((ev)->button_state&1)
#define IS_RIGHT_BUTTON_PRESSED(ev) (((ev)->button_state&2)>>1)
#define IS_MIDDLE_BUTTON_PRESSED(ev) (((ev)->button_state&4)>>2)

#define IS_SHIFT_PRESSED(ev) ((ev)->ev_key_mod&1u)
#define IS_CONTROL_PRESSED(ev) (((ev)->ev_key_mod&2u)>>1)

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
 *
 * @author Hj. Malthaner, Niels Roest
 */
struct event_t {
	unsigned int ev_class;
	unsigned int ev_code;
	int mx, my;

	/**
	 * position of last mouse click
	 */
	int cx, cy;

	/**
	 * current mouse button state
	 */
	int button_state;

	/**
	 * mod key (SHIFT; ALT; CTRL; etc) pressed while event as triggered
	 * @author hsiegeln
	 */
	unsigned int ev_key_mod;

	event_t(unsigned int event_class = EVENT_NONE) : ev_class(event_class),
		ev_code(0),
		mx(0), my(0), cx(0), cy(0),
		button_state(0), ev_key_mod(0)
		{ }
};

#ifdef __cplusplus
/**
 * Translate event origin. Useful when transferring events to sub-components.
 * @author Hj. Malthaner
 */
static inline void translate_event(event_t* const ev, int x, int y)
{
	ev->mx += x;
	ev->cx += x;
	ev->my += y;
	ev->cy += y;
}
#endif

/**
 * Return one event. Does *not* wait.
 * @author Hj. Malthaner
 */
void display_poll_event(event_t*);

/**
 * Wait for one event, and return it.
 * @author Hj. Malthaner
 */
void display_get_event(event_t*);
void change_drag_start(int x, int y);

int event_get_last_control_shift();
unsigned int last_meta_event_get_class();

/**
 * Adds new events to be processed.
 */
void queue_event(event_t *event);

#endif
