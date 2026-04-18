/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "onewaysign_info.h"
#include "../obj/roadsign.h"
#include "../player/simplay.h"


#include "../simmenu.h"
#include "../simworld.h"

onewaysign_info_t::onewaysign_info_t(roadsign_t* s, koord3d) :
	obj_infowin_t(s),
	sign(s)
{
	set_table_layout(1,0);
	if(  sign->get_desc()->is_single_way()  ) {
		direction[0].init( button_t::square_state, translator::translate("Left") );
		direction[1].init( button_t::square_state, translator::translate("Right") );
		direction[0].add_listener( this );
		direction[1].add_listener( this );
		direction[0].pressed = (sign->get_lane_affinity() & 1) != 0;
		direction[1].pressed = (sign->get_lane_affinity() & 2) != 0;
		
		// place components
		add_table(2,1);
		{
			add_component( &direction[0] );
			add_component( &direction[1] );
		}
		end_table();
	}

	if(  sign->get_desc()->is_choose_sign()  ) {
		bt_length_based.init( button_t::square_state, translator::translate("Length based choose") );
		bt_length_based.add_listener(this);
		bt_length_based.pressed = sign->is_length_based();
		add_component(&bt_length_based);
	}

	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
 */
bool onewaysign_info_t::action_triggered( gui_action_creator_t *komp, value_t /* */)
{
	if(  komp == &bt_length_based  ) {
		char param[256];
		bool v = sign->is_length_based();
		sprintf( param, "%s,%i,l", sign->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	uint8 fix = sign->get_lane_affinity();
	for(  int i=0;  i<2;  i++  ) {
		if(komp == &direction[i]) {
			fix ^= (i+1);
		}
	}
	char param[256];
	sprintf( param, "%s,%i,r", sign->get_pos().get_str(), fix );
	tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
	welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
	return true;
}


// notify for an external update
void onewaysign_info_t::update_data()
{
	if(  sign->get_desc()->is_single_way()  ) {
		for(  int i=0;  i<2;  i++  ) {
			direction[i].pressed = (sign->get_lane_affinity() & (i+1)) != 0;
		}
	}
	if(  sign->get_desc()->is_choose_sign()  ) {
		bt_length_based.pressed = sign->is_length_based();
	}
}
