/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"

#include "signal_spacing.h"
#include "../tool/simtool.h"


uint8 signal_spacing_frame_t::signal_spacing = 2;
bool signal_spacing_frame_t::remove = true;
bool signal_spacing_frame_t::replace = true;

signal_spacing_frame_t::signal_spacing_frame_t(player_t *player_, tool_build_roadsign_t* tool_) :
	gui_frame_t( translator::translate("set signal spacing") ),
	signal_label("signal spacing")
{
	player = player_;
	tool = tool_;
	tool->get_values(player, signal_spacing, remove, replace);

	set_table_layout(3,0);

	add_component( &signal_label );
	new_component<gui_fill_t>();

	signal_spacing_inp.add_listener(this);
	signal_spacing_inp.init(signal_spacing, 1, 50, 1, true, 3);
	add_component( &signal_spacing_inp );

	remove_button.init( button_t::square_state, "remove interm. signals");
	remove_button.add_listener(this);
	remove_button.pressed = remove;
	add_component( &remove_button, 3);

	replace_button.init( button_t::square_state, "replace other signals");
	replace_button.add_listener(this);
	replace_button.pressed = replace;
	add_component( &replace_button, 3);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}

bool signal_spacing_frame_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if( comp == &signal_spacing_inp ) {
		signal_spacing = signal_spacing_inp.get_value();
	}
	else if( comp == &remove_button ) {
		remove = !remove;
		remove_button.pressed = remove;
	}
	else if( comp == &replace_button ) {
		replace = !replace;
		replace_button.pressed = replace;
	}
	tool->set_values(player, signal_spacing, remove, replace);
	return true;
}
