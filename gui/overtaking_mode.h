/*
 * Dialogue to set the signal spacing, when CTRL+clicking a signal on toolbar
 * Used by tool_roadsign_t
 */

#ifndef overtaking_mode_h
#define overtaking_mode_h

#include "gui_frame.h"
#include "components/action_listener.h"

class button_t;
class gui_label_t;
class gui_divider_t;
class gui_numberinput_t;
class tool_build_way_t;
class tool_build_bridge_t;
class tool_build_tunnel_t;
class player_t;

class overtaking_mode_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static overtaking_mode_t overtaking_mode;
	static char mode_name[6][20];
	player_t *player;
	tool_build_way_t* tool_w;
	tool_build_bridge_t* tool_b;
	tool_build_tunnel_t* tool_tu;
	uint8 tool_class; // 0:way, 1:bridge, 2:tunnel
	button_t mode_button[6];
	gui_divider_t divider[2];
	button_t avoid_cityroad_button;
	button_t citycar_no_entry_button;
	gui_numberinput_t height_offset;
	void init(player_t *player, overtaking_mode_t overtaking_mode, uint8 street_flag, bool show_avoid_cityroad);

public:
	overtaking_mode_frame_t( player_t *, tool_build_way_t *, bool );
	overtaking_mode_frame_t( player_t *, tool_build_bridge_t * );
	overtaking_mode_frame_t( player_t *, tool_build_tunnel_t * );
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	const char * get_help_filename() const { return "overtaking_mode_frame.txt"; }
};

#endif
