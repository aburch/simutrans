#include "signal_info.h"
#include "../obj/signal.h"
#include "../player/simplay.h"


#include "../simmenu.h"
#include "../simworld.h"

signal_info_t::signal_info_t(signal_t* s) :
obj_infowin_t(s),
signal(s)
{
	set_table_layout(1,0);
	
	bt_require_parent.init( button_t::square_state, translator::translate("Require parent convoy to enter."));
	bt_require_parent.add_listener(this);
	bt_require_parent.pressed = signal->is_guide_signal();
	if(  signal->get_desc()->is_choose_sign()  ) {
		add_component(&bt_require_parent);
	}
	
	bt_remove_signal.init( button_t::roundbox, translator::translate("remove signal"));
	bt_remove_signal.enable( !signal->is_deletable( welt->get_active_player() ) );
	bt_remove_signal.add_listener(this);
	add_component(&bt_remove_signal);
	
	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}

bool signal_info_t::action_triggered( gui_action_creator_t* comp, value_t /* */)
{
	if(  comp==&bt_remove_signal  ) {
		bool suspended_execution=false;
		koord3d pos = signal->get_pos();
		tool_t::general_tool[TOOL_REMOVE_SIGNAL]->set_default_param(NULL);
		const char *err = world()->call_work( tool_t::general_tool[TOOL_REMOVE_SIGNAL], welt->get_active_player(), pos, suspended_execution );
		if(!suspended_execution) {
			// play sound / error message
			welt->get_active_player()->tell_tool_result(tool_t::general_tool[TOOL_REMOVE_SIGNAL], pos, err);
		}
		return true;
	}
	if(  comp==&bt_require_parent  ) {
		char param[256];
		bool v = signal->is_guide_signal();
		sprintf( param, "%s,%i,s", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	return false;
}


// notify for an external update
void signal_info_t::update_data()
{
	bt_require_parent.pressed = signal->is_guide_signal();
	bt_remove_signal.enable( !signal->is_deletable( welt->get_active_player() ) );
}
