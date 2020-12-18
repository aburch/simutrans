/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"

#include "signal_spacing.h"
#include "../simworld.h"
#include "../simtool.h"


uint16 signal_spacing_frame_t::signal_spacing = 16;
bool signal_spacing_frame_t::remove = true;
bool signal_spacing_frame_t::replace = true;
bool signal_spacing_frame_t::backward = false;
koord3d signal_spacing_frame_t::signalbox = koord3d::invalid;

signal_spacing_frame_t::signal_spacing_frame_t(player_t *player_, tool_build_roadsign_t* tool_) :
	gui_frame_t( translator::translate("set signal spacing") )
{
	player = player_;
	tool = tool_;
	tool->get_values(player, signal_spacing, remove, replace, backward, signalbox);

	set_table_layout(3,0);

	new_component<gui_label_t>("signal spacing");
	new_component<gui_fill_t>();

	add_table(2,1);
	signal_spacing_inp.add_listener(this);
	signal_spacing_meter = signal_spacing * welt->get_settings().get_meters_per_tile();
	signal_spacing_inp.init(signal_spacing_meter, 1*welt->get_settings().get_meters_per_tile(), 128*welt->get_settings().get_meters_per_tile(), welt->get_settings().get_meters_per_tile(), true, 4);
	add_component( &signal_spacing_inp );
	new_component<gui_label_t>("m");
	end_table();

	remove_button.init( button_t::square_state, "remove interm. signals");
	remove_button.add_listener(this);
	remove_button.pressed = remove;
	add_component( &remove_button, 3);

	replace_button.init( button_t::square_state, "replace other signals");
	replace_button.add_listener(this);
	replace_button.pressed = replace;
	add_component( &replace_button, 3);

	backward_button.init( button_t::square_state, "place signals backward");
	backward_button.add_listener(this);
	backward_button.pressed = backward;
	add_component( &backward_button, 3);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}

bool signal_spacing_frame_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	if( comp == &signal_spacing_inp ) {
		signal_spacing = signal_spacing_inp.get_value() / (welt->get_settings().get_meters_per_tile());
	}
	else if( comp == &remove_button ) {
		remove = !remove;
		remove_button.pressed = remove;
	}
	else if( comp == &replace_button ) {
		replace = !replace;
		replace_button.pressed = replace;
	}
	else if( comp == &backward_button ) {
		backward = !backward;
		backward_button.pressed = backward;
		replace = !backward;
		replace_button.pressed = !backward;
		remove  = !backward;
		remove_button.pressed = !backward;

	}
	tool->set_values(player, signal_spacing, remove, replace, backward, signalbox);
	return true;
}
