/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef banner_h
#define banner_h

#include "../dataobj/environment.h"
#include "components/gui_button.h"
#include "gui_frame.h"


/*
 * Class to generates the welcome screen with the scrolling
 * text to celebrate contributors.
 */

class banner_t : public gui_frame_t, action_listener_t
{
private:
	button_t
		new_map,
		load_map,
		load_scenario,
		join_map,
		quit;

public:
	banner_t();

	bool has_sticky() const OVERRIDE { return false; }

	bool has_title() const OVERRIDE { return false; }

	/**
	* Window Title
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return ""; }

	/**
	* get color information for the window title
	* -borders and -body background
	* @author Hj. Malthaner
	*/
	FLAGGED_PIXVAL get_titlecolor() const OVERRIDE {return env_t::default_window_title_color; }

	bool is_hit(int, int) OVERRIDE { return true; }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	* @author Hj. Malthaner
	*/
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
