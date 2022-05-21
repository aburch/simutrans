/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "trafficlight_info.h"
#include "components/gui_label.h"
#include "../obj/roadsign.h" // The rest of the dialog

#include "../tool/simmenu.h"
#include "../world/simworld.h"


void trafficlight_info_t::update_data()
{
	ns.set_value( roadsign->get_ticks_ns() );
	ow.set_value( roadsign->get_ticks_ow() );
	offset.set_value( roadsign->get_ticks_offset() );
	yellow_ns.set_value( roadsign->get_ticks_yellow_ns() );
	yellow_ow.set_value( roadsign->get_ticks_yellow_ow() );
}


trafficlight_info_t::trafficlight_info_t(roadsign_t* s) :
	obj_infowin_t(s),
	roadsign(s)
{
	add_table(3,1);
	{
	  ns.set_limits( 1, 255 );
	  ns.wrap_mode( false );
	  ns.add_listener( this );
	  add_component( &ns );

	  ow.set_limits( 1, 255 );
	  ow.wrap_mode( false );
	  ow.add_listener( this );
	  add_component( &ow );

	  offset.set_limits( 0, 255 );
	  offset.wrap_mode( false );
	  offset.add_listener( this );
	  add_component( &offset );
	}
	end_table();

	add_table(2,1);
	{
	  yellow_ns.set_limits( 1, 255 );
	  yellow_ns.wrap_mode( false );
	  yellow_ns.add_listener( this );
	  add_component( &yellow_ns );

	  yellow_ow.set_limits( 1, 255 );
	  yellow_ow.wrap_mode( false );
	  yellow_ow.add_listener( this );
	  add_component( &yellow_ow );
	}
	end_table();

	update_data();

	// show author below the settings
	if (char const* const maker = roadsign->get_desc()->get_copyright()) {
		gui_label_buf_t* lb = new_component<gui_label_buf_t>();
		lb->buf().printf(translator::translate("Constructed by %s"), maker);
		lb->update();
	}

	recalc_size();
}


bool trafficlight_info_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	char param[256];
	int toolnr = 0;
	if(comp == &ns) {
		toolnr = 1;
	}
	else if(comp == &ow) {
		toolnr = 0;
	}
	else if(comp == &offset) {
		toolnr = 2;
	}
 	else if(comp == &yellow_ns) {
		toolnr = 4;
	}
 	else if(comp == &yellow_ow) {
		toolnr = 3;
	}
	else {
		dbg->fatal( "trafficlight_info_t","Wrong action triggered" );
	}
	sprintf( param, "%s,%d,%i", roadsign->get_pos().get_str(), toolnr, (int)v.i );
	tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
	welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
	return true;
}


