/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * jumps to a position
 */

#include <string.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../display/viewport.h"
#include "jump_frame.h"
#include "components/gui_divider.h"

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

	set_focus(&input);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool jump_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &input || komp == &jumpbutton) {
		// OK- Button or Enter-Key pressed
		//---------------------------------------
		koord my_pos;
		sscanf(buf, "%hd,%hd", &my_pos.x, &my_pos.y);
		if(welt->is_within_limits(my_pos)) {
			welt->get_viewport()->change_world_position(koord3d(my_pos,welt->min_hgt(my_pos)));
		}
	}
	return true;
}
