/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * jumps to a position
 */

#include <string.h>
#include <time.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../display/viewport.h"
#include "jump_frame.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"

#include "../dataobj/translator.h"


jump_frame_t::jump_frame_t() :
	gui_frame_t( translator::translate("Jump to") )
{
	set_table_layout(1,0);

	// Input box for new name
	sprintf(buf, "%i,%i", welt->get_viewport()->get_world_position().x, welt->get_viewport()->get_world_position().y );
	input.set_text(buf, 62);
	input.add_listener(this);
	add_component(&input);

	new_component<gui_divider_t>();

	jumpbutton.init( button_t::roundbox, "Jump to");
	jumpbutton.add_listener(this);
	add_component(&jumpbutton);

	new_component<gui_divider_t>();

	sprintf(auto_jump_countdown_char, "%s", "-----");
	add_table(3,1);
	{
		auto_jump_button.init( button_t::square_state, "AutoJump ");
		auto_jump_button.add_listener( this );
		auto_jump_button.pressed = auto_jump;
		add_component( &auto_jump_button );
		new_component<gui_label_t>(auto_jump_countdown_char );
		auto_jump_interval_numberinput.add_listener(this);
		auto_jump_interval_numberinput.init(auto_jump_interval, 5, 65535, 1, true, 5);
		add_component( &auto_jump_interval_numberinput );
	}

	set_focus(&input);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool jump_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &input || comp == &jumpbutton) {
		// OK- Button or Enter-Key pressed
		//---------------------------------------
		koord my_pos;
		sscanf(buf, "%hd,%hd", &my_pos.x, &my_pos.y);
		if(welt->is_within_limits(my_pos)) {
			welt->get_viewport()->change_world_position(koord3d(my_pos,welt->min_hgt(my_pos)));
		}
	}
	else if(comp == &auto_jump_button) {
		auto_jump = !auto_jump;
		auto_jump_button.pressed = auto_jump;
		auto_jump_base_time = time(NULL);
	}
	else if(comp == &auto_jump_interval_numberinput) {
		uint16 buf = auto_jump_interval_numberinput.get_value();
		auto_jump_interval = buf;
		auto_jump_base_time = time(NULL);
	}
	return true;
}
