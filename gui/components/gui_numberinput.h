/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * @author Dwachs
 */

/*
 * An input field for integer numbers (with arrow buttons for dec/inc)
 * @author Dwachs
 */

#ifndef gui_components_gui_numberinput_h
#define gui_components_gui_numberinput_h

#include "../../simtypes.h"
#include "../../display/scr_coord.h"
#include "action_listener.h"
#include "gui_action_creator.h"
#include "gui_textinput.h"
#include "gui_button.h"
#include "../gui_theme.h"


class gui_numberinput_t :
	public gui_action_creator_t,
	public gui_komponente_t,
	public action_listener_t
{
private:
	bool check_value(sint32 _value);

	// more sophisticated increase routines
	static sint8 percent[7];
	sint32 get_prev_value();
	sint32 get_next_value();

	// transformation char* -> int
	sint32 get_text_value();

	// the input field
	gui_textinput_t textinp;

	// arrow buttons for increasing / decr.
	button_t bt_left, bt_right;

	sint32 value;

	sint32 min_value, max_value;

	char textbuffer[20];

	sint32 step_mode;

	bool wrapping:1;
	bool b_enabled:1;

	// since only the last will prevail
	static char tooltip[256];

public:
	gui_numberinput_t();

	void set_size(scr_size size) OVERRIDE;
	void set_width_by_len(size_t width, const char* symbols = NULL) {
		set_width( display_get_char_max_width( (symbols) ? symbols : "+-/0123456789" ) * width + D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH + 2 ); }

	// all init in one ...
	void init( sint32 value, sint32 min, sint32 max, sint32 mode, bool wrap );

	/**
	 * sets and get the current value.
	 * return current value (or min or max in currently set to outside value)
	 */
	sint32 get_value();
	void set_value(sint32);

	/**
	 * digits: length of textbuffer
	 */
	void set_limits(sint32 _min, sint32 _max);

	enum { AUTOLINEAR=0, POWER2=-1, PROGRESS=-2 };
	/**
	 * AUTOLINEAR: linear increment, scroll wheel 1% range
	 * POWER2: 16, 32, 64, ...
	 * PROGRESS: 0, 1, 5, 10, 25, 50, 75, 90, 95, 99, 100% of range
	 * any other mode value: actual step size
	 */

	void set_increment_mode( sint32 m ) { step_mode = m; }

	// true, if the component wraps around
	bool wrap_mode( bool new_mode ) {
		bool m=wrapping;
		wrapping=new_mode;
		return m;
	}

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the component
	 * @author Dwachs
	 */
	void draw(scr_coord offset);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void enable() { b_enabled = true; set_focusable(true); bt_left.enable(); bt_right.enable(); }
	void disable() { b_enabled = false; set_focusable(false); bt_left.disable(); bt_right.disable(); }
	bool enabled() const { return b_enabled; }
	virtual bool is_focusable() { return b_enabled && gui_komponente_t::is_focusable(); }
};

#endif
