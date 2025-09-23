#include "end_of_choose_info.h"
#include "../obj/roadsign.h"
#include "../player/simplay.h"


#include "../simmenu.h"
#include "../simworld.h"

end_of_choose_info_t::end_of_choose_info_t(roadsign_t* s) :
obj_infowin_t(s),
signal(s)
{
	set_table_layout(1,0);

	bt_end_of_choose.init( button_t::square_state, translator::translate("end of choose signal") );
	bt_end_of_choose.add_listener(this);
	bt_end_of_choose.pressed = signal->is_flag_end_of_choose();
	if(  signal->get_waytype()!=road_wt && signal->get_waytype()!=water_wt && signal->get_waytype()!=air_wt  ) {
		add_component(&bt_end_of_choose);
	}

	bt_end_of_guide.init( button_t::square_state, translator::translate("try coupling convoy not enter here") );
	bt_end_of_guide.add_listener(this);
	bt_end_of_guide.pressed = signal->is_flag_end_of_guide();
	if(  signal->get_waytype()!=road_wt && signal->get_waytype()!=water_wt && signal->get_waytype()!=air_wt  ) {
		add_component(&bt_end_of_guide);
	}
	
	// bt_remove_signal.init( button_t::roundbox, translator::translate("remove signal"));
	// bt_remove_signal.enable( !signal->is_deletable( welt->get_active_player() ) );
	// bt_remove_signal.add_listener(this);
	// add_component(&bt_remove_signal);
	
	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}

bool end_of_choose_info_t::action_triggered( gui_action_creator_t* comp, value_t /* */)
{
	// if(  comp==&bt_remove_signal  ) {
	// 	bool suspended_execution=false;
	// 	koord3d pos = signal->get_pos();
	// 	tool_t::general_tool[TOOL_REMOVE_SIGNAL]->set_default_param(NULL);
	// 	const char *err = world()->call_work( tool_t::general_tool[TOOL_REMOVE_SIGNAL], welt->get_active_player(), pos, suspended_execution );
	// 	if(!suspended_execution) {
	// 		// play sound / error message
	// 		welt->get_active_player()->tell_tool_result(tool_t::general_tool[TOOL_REMOVE_SIGNAL], pos, err);
	// 	}
	// 	return true;
	// }
	if(  comp==&bt_end_of_choose  ) {
		char param[256];
		bool v = signal->is_flag_end_of_choose();
		sprintf( param, "%s,%i,c", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	else if(  comp==&bt_end_of_guide  ) {
		char param[256];
		bool v = signal->is_flag_end_of_guide();
		sprintf( param, "%s,%i,g", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	return false;
}


// notify for an external update
void end_of_choose_info_t::update_data()
{
	bt_end_of_choose.pressed = signal->is_flag_end_of_choose();
	bt_end_of_guide.pressed = signal->is_flag_end_of_guide();
	bt_remove_signal.enable( !signal->is_deletable( welt->get_active_player() ) );
}
