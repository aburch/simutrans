/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_NUMBERINPUT_H
#define GUI_COMPONENTS_GUI_NUMBERINPUT_H


#include "../../simtypes.h"
#include "../../display/scr_coord.h"
#include "action_listener.h"
#include "gui_action_creator.h"
#include "gui_textinput.h"
#include "gui_button.h"
#include "../gui_theme.h"


/**
 * An input field for integer numbers (with arrow buttons for dec/inc)
 */
class gui_numberinput_t :
	public gui_action_creator_t,
	public gui_component_t,
	public action_listener_t
{
private:
	bool check_value(sint32 _value);

	scr_coord_val max_numbertext_width;

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

	// number of digits,
	// used to determine min size
	uint16 digits;

	char textbuffer[20];

	sint32 step_mode;

	bool wrapping : 1;
	bool b_enabled : 1;
	bool no_tooltip : 1;

	// since only the last will prevail
	static char tooltip[256];

public:
	gui_numberinput_t();

	void set_size(scr_size size) OVERRIDE;

	// all init in one ...
	void init( sint32 value, sint32 min, sint32 max, sint32 mode = 1, bool wrap = true, uint16 digits = 5, bool tooltip=true);

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

	enum {
		AUTOLINEAR = 0,
		POWER2     = -1,
		PROGRESS   = -2
	};

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

	void draw(scr_coord offset) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void enable() { b_enabled = true; set_focusable(true); bt_left.enable(); bt_right.enable(); }
	void disable() { b_enabled = false; set_focusable(false); bt_left.disable(); bt_right.disable(); }
	bool enabled() const { return b_enabled; }
	bool is_focusable() OVERRIDE { return b_enabled && gui_component_t::is_focusable(); }
	void enable( bool yesno ) {
		if( yesno && !gui_component_t::is_focusable() ) {
			enable();
		}
		else if( !yesno  &&  gui_component_t::is_focusable() ) {
			disable();
		}
	}

	void allow_tooltip(bool b) { no_tooltip = !b; }

	scr_size get_max_size() const OVERRIDE;

	scr_size get_min_size() const OVERRIDE;
};

#endif
