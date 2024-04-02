/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_BANNER_H
#define GUI_BANNER_H

#include "scenario_frame.h"
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
		continue_game,
		new_map,
		load_map,
		play_tutorial,
		load_scenario,
		join_map,
		options,
		install,
		quit;

	cbuffer_t continue_tooltip;

	scenario_frame_t *scenario_frame = new scenario_frame_t();

	std::string pakset_tutorial = env_t::pak_dir + "scenario/" + env_t::pakset_tutorial_dir;

public:
	banner_t();

	bool has_sticky() const OVERRIDE { return false; }

	bool has_title() const OVERRIDE { return false; }

	/**
	* Window Title
	*/
	const char *get_name() const {return ""; }

	/**
	* get color information for the window title
	* -borders and -body background
	*/
	FLAGGED_PIXVAL get_titlecolor() const OVERRIDE {return env_t::default_window_title_color; }

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	static void show_banner();

};

#endif
