/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Traffic light phase buttons
 */

#include "trafficlight_info.h"
#include "../obj/roadsign.h" // The rest of the dialog

#include "../simmenu.h"
#include "../simworld.h"

trafficlight_info_t::trafficlight_info_t(roadsign_t* s) :
	obj_infowin_t(s),
	roadsign(s)
{
	add_table(3,5);
	
	// number inputs
	ns.set_limits( 1, 255 );
	ns.set_value( s->get_ticks_ns() );
	ns.wrap_mode( false );
	ns.add_listener( this );
	add_component( &ns );
	
	ow.set_limits( 1, 255 );
	ow.set_value( s->get_ticks_ow() );
	ow.wrap_mode( false );
	ow.add_listener( this );
	add_component( &ow );
	
	offset.set_limits( 1, 255 );
	offset.set_value( s->get_ticks_offset() );
	offset.wrap_mode( false );
	offset.add_listener( this );
	add_component( &offset );

	// direction_buttons
	const char* direction_texts[4] = {"nord","ost","sued","west"};
	for(uint8 i=0; i<4; i++) {
		// left side
		direction_buttons[i].init( button_t::square_state, direction_texts[i]);
		add_component( &direction_buttons[i] );

		// right side
		direction_buttons[i+4].init( button_t::square_state, "");
		add_component( &direction_buttons[i+4] );
		
		new_component<gui_fill_t>(false,false);
	}
	for(uint8 i=0; i<8; i++) {
		direction_buttons[i].add_listener( this );
		direction_buttons[i].pressed = ((roadsign->get_open_direction())&(1<<i))!=0;
	}
	
	end_table();
	recalc_size();
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool trafficlight_info_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	char param[256];
	if(komp == &ns) {
		sprintf( param, "%s,1,%i", roadsign->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
	else if(komp == &ow) {
		sprintf( param, "%s,0,%i", roadsign->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
 	else if(komp == &offset) {
		sprintf( param, "%s,2,%i", roadsign->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
	else {
		// maybe this event is caused by the direction buttons.

		uint8 dir = roadsign->get_open_direction();
		for(uint8 i=0; i<8; i++) {
			if(komp == &direction_buttons[i]) {
				dir ^= 1 << i;
			}
		}
		// set open_direction
		sprintf( param, "%s,3,%i", roadsign->get_pos().get_str(), dir );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
		for(uint8 i=0; i<8; i++) {
			direction_buttons[i].pressed = ((roadsign->get_open_direction())&(1<<i))!=0;
		}
	}
	return true;
}


// notify for an external update
void trafficlight_info_t::update_data()
{
	ns.set_value( roadsign->get_ticks_ns() );
	ow.set_value( roadsign->get_ticks_ow() );
	offset.set_value( roadsign->get_ticks_offset() );
	for(uint8 i=0; i<8; i++) {
		direction_buttons[i].pressed = ((roadsign->get_open_direction())&(1<<i))!=0;
	}
}
