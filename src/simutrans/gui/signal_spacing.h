/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SIGNAL_SPACING_H
#define GUI_SIGNAL_SPACING_H


#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"
#include "components/gui_label.h"


class gui_numberinput_t;
class button_t;
class gui_label_t;
class tool_build_roadsign_t;
class player_t;


/*
 * Dialogue to set the signal spacing, when CTRL+clicking a signal on toolbar
 * Used by tool_build_roadsign_t
 */
class signal_spacing_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static uint8 signal_spacing;
	static bool remove, replace;
	player_t *player;
	tool_build_roadsign_t* tool;
	gui_numberinput_t signal_spacing_inp;
	gui_label_t signal_label;
	button_t remove_button, replace_button;

public:
	signal_spacing_frame_t( player_t *, tool_build_roadsign_t * );
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	const char * get_help_filename() const OVERRIDE { return "signal_spacing.txt"; }
};

#endif
