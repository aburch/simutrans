/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "components/gui_label.h"
#include "../obj/signal.h"
#include "../player/simplay.h"

#include "signal_info.h"

#include "../tool/simmenu.h"
#include "../world/simworld.h"
#include "../dataobj/ribi.h"

signal_info_t::signal_info_t(signal_t *s) :
	obj_infowin_t(s),
	sig(s)
{
	two_ways_toggle.init( button_t::square_state, "allow reverse passage");
	two_ways_toggle.add_listener( this );
	two_ways_toggle.pressed = sig->get_two_ways();
	add_component( &two_ways_toggle );

	remove.init( button_t::roundbox, "remove signal");
	remove.add_listener( this );
	add_component( &remove );

	// show author below the settings
	if(  char const* const maker = sig->get_desc()->get_copyright()  ) {
		gui_label_buf_t* lb = new_component<gui_label_buf_t>();
		lb->buf().printf(translator::translate("Constructed by %s"), maker);
		lb->update();
	}

	recalc_size();
}


bool signal_info_t::action_triggered( gui_action_creator_t *comp, value_t /* */)
{
	if (comp == &remove) {
		bool suspended_execution=false;
		koord3d pos = sig->get_pos();
		tool_t::general_tool[TOOL_REMOVE_SIGNAL]->set_default_param(NULL);
		const char *err = welt->call_work_api( tool_t::general_tool[TOOL_REMOVE_SIGNAL], welt->get_active_player(), pos, suspended_execution);
		if(!suspended_execution) {
			// play sound / error message
			welt->get_active_player()->tell_tool_result(tool_t::general_tool[TOOL_REMOVE_SIGNAL], pos, err);
		}
	}
	else if (comp == &two_ways_toggle) {
		char param[64];
		sprintf(param, "%s,1,%u", sig->get_pos().get_str(), !sig->get_two_ways());
		tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param(param);
		welt->set_tool(tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player());
	}
	return true;
}


void signal_info_t::draw( scr_coord pos, scr_size size )
{
	two_ways_toggle.pressed = sig->get_two_ways();
	two_ways_toggle.enable( ribi_t::is_single(sig->get_dir()) );
	remove.enable( !sig->get_removal_error( welt->get_active_player() ) );
	obj_infowin_t::draw( pos, size );
}
