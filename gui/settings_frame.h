/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SETTINGS_FRAME_H
#define GUI_SETTINGS_FRAME_H


#include "gui_frame.h"
#include "components/gui_tab_panel.h"
#include "components/gui_button.h"
#include "components/gui_scrollpane.h"

#include "settings_stats.h"
#include "components/action_listener.h"

class settings_t;


/**
 * All messages since the start of the program
 */
class settings_frame_t : public gui_frame_t, action_listener_t
{
private:
	settings_t* sets;
	gui_tab_panel_t tabs;

	settings_general_stats_t  general;
	gui_scrollpane_t          scrolly_general;
	settings_display_stats_t  display;
	gui_scrollpane_t          scrolly_display;
	settings_economy_stats_t  economy;
	gui_scrollpane_t          scrolly_economy;
	settings_routing_stats_t  routing;
	gui_scrollpane_t          scrolly_routing;
	settings_costs_stats_t    costs;
	gui_scrollpane_t          scrolly_costs;
	settings_climates_stats_t climates;
	gui_scrollpane_t          scrolly_climates;

	button_t revert_to_default, revert_to_last_save;

public:
	settings_frame_t(settings_t*);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char *get_help_filename() const OVERRIDE {return "settings.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// does not work during new world dialogue
	bool has_sticky() const OVERRIDE { return false; }

	bool infowin_event(event_t const*) OVERRIDE;
};

#endif
