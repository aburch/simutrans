/*
 * Dialogue to set the signal spacing, when CTRL+clicking a signal on toolbar
 * Used by tool_roadsign_t
 */

#ifndef wayobj_spacing_h
#define wayobj_spacing_h

#include "gui_frame.h"
#include "components/action_listener.h"

class gui_numberinput_t;
class gui_label_t;
class tool_build_wayobj_t;
class player_t;

class wayobj_spacing_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static uint8 spacing;
	player_t *player;
	tool_build_wayobj_t* tool;
	gui_numberinput_t spacing_inp;
	gui_label_t label;

public:
	wayobj_spacing_frame_t( player_t *, tool_build_wayobj_t * );
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	const char * get_help_filename() const { return "wayobj_spacing.txt"; }
};

#endif
