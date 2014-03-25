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
#include "components/gui_button.h"

#include "../dataobj/translator.h"


jump_frame_t::jump_frame_t() :
	gui_frame_t( translator::translate("Jump to") )
{
	scr_coord cursor(D_MARGIN_LEFT,D_MARGIN_TOP);

	// Input box for new name
	sprintf(buf, "%i,%i", welt->get_viewport()->get_world_position().x, welt->get_viewport()->get_world_position().y );
	input.set_text(buf, 62);
	input.add_listener(this);
	input.set_pos(cursor);
	input.set_size(scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	add_komponente(&input);
	cursor.y += D_BUTTON_HEIGHT + D_V_SPACE;

	divider1.set_pos(cursor);
	divider1.set_size(scr_size(D_BUTTON_WIDTH,D_DIVIDER_HEIGHT));
	add_komponente(&divider1);
	cursor.y += D_DIVIDER_HEIGHT + D_V_SPACE;

	jumpbutton.init( button_t::roundbox, "Jump to", cursor );
	jumpbutton.add_listener(this);
	add_komponente(&jumpbutton);

	set_focus(&input);
	set_windowsize(scr_size(cursor.x + D_BUTTON_WIDTH + D_MARGIN_RIGHT, cursor.y + D_BUTTON_HEIGHT + D_MARGIN_BOTTOM + D_TITLEBAR_HEIGHT));
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
