/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../player/ai.h"

#include "../descriptor/skin_desc.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "ai_option.h"


bool ai_button_test(button_t *button, ai_t* ai, bool (ai_t::*get)() const, void (ai_t::*set)(bool) )
{
	button->pressed = (ai->*get)();
	(ai->*set)( !button->pressed );
	if(  button->pressed == (ai->*get)()  ) {
		if(  (ai->*get)()  ) {
			button->disable();
			return true;
		}
	}
	else {
		(ai->*set)( button->pressed );
		return true;
	}
	return false;
}


void toggle_ai_button(button_t *button, ai_t* ai, bool (ai_t::*get)() const, void (ai_t::*set)(bool) )
{
	(ai->*set)( !button->pressed );
	button->pressed = (ai->*get)();
}


ai_option_t::ai_option_t( player_t *player ) :
	gui_frame_t( translator::translate("Configure AI"), player )
{
	this->ai = dynamic_cast<ai_t *>(player);

	set_table_layout(1,0);

	new_component<gui_label_t>("construction speed");

	construction_speed.init( ai->get_construction_speed(), 25, 1000000, gui_numberinput_t::POWER2, false );
	construction_speed.add_listener( this );
	add_component( &construction_speed );

	// find out if the mode is available and can be activated

	buttons[0].init( button_t::square_state, "road vehicle");
	if (ai_button_test(buttons+0, ai, &ai_t::has_road_transport, &ai_t::set_road_transport)) {
		buttons[0].add_listener( this );
		add_component( buttons+0 );
	}

	buttons[1].init( button_t::square_state, "rail car");
	if (ai_button_test(buttons+1, ai, &ai_t::has_rail_transport, &ai_t::set_rail_transport)) {
		buttons[1].add_listener( this );
		add_component( buttons+1 );
	}

	buttons[2].init( button_t::square_state, "water vehicle");
	if (ai_button_test(buttons+2, ai, &ai_t::has_ship_transport, &ai_t::set_ship_transport)) {
		buttons[2].add_listener( this );
		add_component( buttons+2 );
	}

	buttons[3].init( button_t::square_state, "airplane");
	if (ai_button_test(buttons+3, ai, &ai_t::has_air_transport, &ai_t::set_air_transport)) {
		buttons[3].add_listener( this );
		add_component( buttons+3 );
	}

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


bool ai_option_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	if(  comp==&construction_speed  ) {
		ai->set_construction_speed( v.i );
	}
	else if(  comp==buttons+0  ) {
		toggle_ai_button(buttons+0, ai, &ai_t::has_road_transport, &ai_t::set_road_transport);
	}
	else if(  comp==buttons+1  ) {
		toggle_ai_button(buttons+1, ai, &ai_t::has_rail_transport, &ai_t::set_rail_transport);
	}
	else if(  comp==buttons+2  ) {
		toggle_ai_button(buttons+2, ai, &ai_t::has_ship_transport, &ai_t::set_ship_transport);
	}
	else if(  comp==buttons+3  ) {
		toggle_ai_button(buttons+3, ai, &ai_t::has_air_transport, &ai_t::set_air_transport);
	}
	return true;
}
