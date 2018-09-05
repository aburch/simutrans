/*
 * Dialogue to set a value, when CTRL+clicking an icon on toolbar
 * @author THLeaderH
 */

#ifndef simple_number_input_h
#define simple_number_input_h

#include "gui_frame.h"
#include "components/action_listener.h"

class gui_numberinput_t;
class gui_label_t;
class tool_build_wayobj_t;
class tool_build_way_t;
class player_t;

class simple_number_input_frame_t : public gui_frame_t, private action_listener_t
{
private:
	uint8 val;
	gui_numberinput_t input;
	gui_label_t label;

public:
	simple_number_input_frame_t(const char* frame_title, const char* label_text, uint8 min_val, uint8 max_val, uint8 default_val);
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	virtual void register_val(uint8 v) = 0;
};

class wayobj_spacing_frame_t : public simple_number_input_frame_t
{
private:
	tool_build_wayobj_t* tool;
public:
	wayobj_spacing_frame_t( player_t *, tool_build_wayobj_t * );
	void register_val(uint8 v) OVERRIDE;
	const char * get_help_filename() const { return "wayobj_spacing.txt"; }
};

class height_offset_frame_t : public simple_number_input_frame_t
{
private:
	tool_build_way_t* tool;
public:
	height_offset_frame_t( player_t *, tool_build_way_t * );
	void register_val(uint8 v) OVERRIDE;
	const char * get_help_filename() const { return "height_offset.txt"; }
};

#endif
