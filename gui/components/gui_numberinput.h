/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * @author Dwachs
 */

#ifndef gui_components_gui_numberinput_h
#define gui_components_gui_numberinput_h

#include "../../ifc/gui_action_creator.h"
#include "gui_textinput.h"
#include "gui_button.h"
#include "../../simtypes.h"
#include "../../dataobj/koord.h"
#include "action_listener.h"

/**
 * An input field for integer numbers (with arrow buttons for dec/inc)
 * @author Dwachs
 */
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

	bool wrapping;

	// since only the last will prevail
	static char tooltip[256];

public:
	gui_numberinput_t();
	virtual ~gui_numberinput_t() {}

	virtual void set_groesse(koord groesse);

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
	 * AUTOLINEAR: linear increment, scroll whell 1% range
	 * POWER2: 16, 32, 64, ...
	 * PROGRESS: 0, 1, 5, 10, 25, 50, 75, 90, 95, 99, 100% of range
	 * any other mode value: actual step size
	 */

	void set_increment_mode( sint32 m ) { step_mode = m; }

	// true, if the compnent wraps around
	bool wrap_mode( bool new_mode ) {
		bool m=wrapping;
		wrapping=new_mode;
		return m;
	}

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Dwachs
	 */
	bool infowin_event(const event_t *);

	/**
	 * Zeichnet die Komponente
	 * @author Dwachs
	 */
	void zeichnen(koord offset);

	/**
	 * This method is called if an action is triggered
	 */
	virtual bool action_triggered(gui_action_creator_t *komp, value_t p);

	/**
	 * returns element that has the focus
	 * that is: go down the hierarchy as much as possible
	 */
	gui_komponente_t *get_focus() { return (gui_komponente_t *)this; }
};

#endif
