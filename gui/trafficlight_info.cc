/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "trafficlight_info.h"
#include "../obj/roadsign.h" // The rest of the dialog

#include "../simmenu.h"
#include "../simworld.h"

trafficlight_info_t::trafficlight_info_t(roadsign_t* s) :
	obj_infowin_t(s),
	roadsign(s)
{
	add_table(3,0);
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

	offset.set_limits( 0, 255 );
	offset.set_value( s->get_ticks_offset() );
	offset.wrap_mode( false );
	offset.add_listener( this );
	add_component( &offset );

	end_table();
	recalc_size();
}


/**
 * This method is called if an action is triggered
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 */
bool trafficlight_info_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	char param[256];
	if(comp == &ns) {
		sprintf( param, "%s,1,%i", roadsign->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
	else if(comp == &ow) {
		sprintf( param, "%s,0,%i", roadsign->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
	else if(comp == &offset) {
		sprintf( param, "%s,2,%i", roadsign->get_pos().get_str(), (int)v.i );
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	}
	return true;
}


// notify for an external update
void trafficlight_info_t::update_data()
{
	ns.set_value( roadsign->get_ticks_ns() );
	ow.set_value( roadsign->get_ticks_ow() );
	offset.set_value( roadsign->get_ticks_offset() );
}
