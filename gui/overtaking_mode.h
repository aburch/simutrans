/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_OVERTAKING_MODE_H
#define GUI_OVERTAKING_MODE_H


#include "gui_frame.h"
#include "components/action_listener.h"

class button_t;
class gui_label_t;
class tool_build_way_t;
class tool_build_bridge_t;
class tool_build_tunnel_t;
class player_t;

#define MAX_OVERTAKING_MODE 5

/*
 * Dialogue to set the signal spacing, when CTRL+clicking a signal on toolbar
 * Used by tool_roadsign_t
 */
class overtaking_mode_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static overtaking_mode_t overtaking_mode;
	static char mode_name[MAX_OVERTAKING_MODE][96];
	player_t *player;
	tool_build_way_t* tool_w;
	tool_build_bridge_t* tool_b;
	tool_build_tunnel_t* tool_tu;
	uint8 tool_class; // 0:way, 1:bridge, 2:tunnel
	button_t mode_button[MAX_OVERTAKING_MODE];
	void init(player_t *player, overtaking_mode_t overtaking_mode );

public:
	overtaking_mode_frame_t( player_t *, tool_build_way_t * );
	overtaking_mode_frame_t( player_t *, tool_build_bridge_t * );
	overtaking_mode_frame_t( player_t *, tool_build_tunnel_t * );
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	const char * get_help_filename() const OVERRIDE { return "overtaking_mode_frame.txt"; }
};

#endif
