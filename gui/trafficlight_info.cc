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

trafficlight_info_t::trafficlight_info_t(roadsign_t* s) :
	obj_infowin_t(s),
	ampel(s)
{
	ns.set_pos( scr_coord(10,get_windowsize().h-40) );
	ns.set_size( scr_size(52, 12) );
	ns.set_limits( 1, 255 );
	ns.set_value( s->get_ticks_ns() );
	ns.wrap_mode( false );
	ns.add_listener( this );
	add_component( &ns );

	ow.set_pos( scr_coord(66,get_windowsize().h-40) );
	ow.set_size( scr_size(52, 12) );
	ow.set_limits( 1, 255 );
	ow.set_value( s->get_ticks_ow() );
	ow.wrap_mode( false );
	ow.add_listener( this );
	add_component( &ow );

	offset.set_pos( scr_coord(122,get_windowsize().h-40) );
	offset.set_size( scr_size(52, 12) );
	offset.set_limits( 0, 255 );
	offset.set_value( s->get_ticks_offset() );
	offset.wrap_mode( false );
	offset.add_listener( this );
	add_component( &offset );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool trafficlight_info_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	char param[256];
	if(comp == &ns) {
		sprintf( param, "%s,1,%i", ampel->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
	else if(comp == &ow) {
		sprintf( param, "%s,0,%i", ampel->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
 	else if(comp == &offset) {
		sprintf( param, "%s,2,%i", ampel->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
	return true;
}


// notify for an external update
void trafficlight_info_t::update_data()
{
	ns.set_value( ampel->get_ticks_ns() );
	ow.set_value( ampel->get_ticks_ow() );
	offset.set_value( ampel->get_ticks_offset() );
}
