#include "signal_info.h"
#include "../obj/signal.h"
#include "../player/simplay.h"


#include "../simmenu.h"
#include "../simworld.h"
#include "../dataobj/ribi.h"

signal_info_t::signal_info_t(signal_t* s) :
obj_infowin_t(s),
signal(s)
{
	set_table_layout(1,0);

	bt_start_signal.init( button_t::square_state, translator::translate("Starting signal"));
	bt_start_signal.add_listener(this);
	bt_start_signal.pressed = signal->is_start_signal();
	add_component(&bt_start_signal);

	bt_stop_before_check.init( button_t::square_state, translator::translate("Stop before check") );
	bt_stop_before_check.add_listener(this);
	bt_stop_before_check.pressed = signal->is_stop_before_check();
	if(  signal->get_desc()->is_simple_signal()  ||  signal->get_desc()->is_longblock_signal()  ||  signal->get_desc()->is_choose_sign()  ) {
		add_component(&bt_stop_before_check);
	}
	
	bt_require_parent.init( button_t::square_state, translator::translate("Require parent convoy to enter."));
	bt_require_parent.add_listener(this);
	bt_require_parent.pressed = signal->is_guide_signal();
	if(  signal->get_desc()->is_choose_sign()  ) {
		add_component(&bt_require_parent);
	}

	bt_advance_to_end.init( button_t::square_state, translator::translate("Advance to end") );
	bt_advance_to_end.add_listener(this);
	bt_advance_to_end.pressed = signal->is_advance_to_end();
	if(  signal->get_desc()->is_choose_sign()  &&  !welt->get_settings().get_advance_to_end() ) {
		add_component(&bt_advance_to_end);
	}
	
	bt_choose_signal.init( button_t::square_state, translator::translate("Choose signal") );
	bt_choose_signal.add_listener(this);
	bt_choose_signal.pressed = signal->is_choose_signal();
	if(  signal->get_desc()->is_choose_sign()  ) {
		add_component(&bt_choose_signal);
	}
		
	bt_skip_default_route.init( button_t::square_state, translator::translate("Do not use default route") );
	bt_skip_default_route.add_listener(this);
	bt_skip_default_route.pressed = signal->is_skip_default_route();
	if(  signal->get_desc()->is_choose_sign()  ) {
		add_component(&bt_skip_default_route);
	}

	bt_length_based.init( button_t::square_state, translator::translate("Length based choose") );
	bt_length_based.add_listener(this);
	bt_length_based.pressed = signal->is_length_based();
	if(  signal->get_desc()->is_choose_sign()  ) {
		add_component(&bt_length_based);
	}

	add_table(2,0);
	{
		lb_tiles_margin.set_text("Margin");
		lb_tiles_margin.set_tooltip(translator::translate("tiles length of the margin to stop when choosing. Set this margin to the stop side(advance to end->end side)"));
		numinp_tiles_margin.set_width(50);
		numinp_tiles_margin.set_height(5);
		numinp_tiles_margin.set_limits(0,200);
		numinp_tiles_margin.set_increment_mode(1);
		numinp_tiles_margin.disable();
		numinp_tiles_margin.add_listener(this);
		if(signal->get_desc()->is_choose_sign()  &&  !welt->get_settings().get_advance_to_end()) {
			add_component(&lb_tiles_margin);
			add_component(&numinp_tiles_margin);
		}
	}
	end_table();

	bt_two_ways.init( button_t::square_state, translator::translate("allow reverse passage") );
	bt_two_ways.set_tooltip(translator::translate("Allow reverse passage for convoys"));
	bt_two_ways.add_listener(this);
	bt_two_ways.pressed = signal->get_two_ways();
	bt_two_ways.enable( ribi_t::is_single(signal->get_dir()) );
	add_component(&bt_two_ways);

	bt_remove_signal.init( button_t::roundbox, translator::translate("remove signal"));
	bt_remove_signal.enable( !signal->is_deletable( welt->get_active_player() ) );
	bt_remove_signal.add_listener(this);
	add_component(&bt_remove_signal);
	
	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
	update_data();
}

bool signal_info_t::action_triggered( gui_action_creator_t* comp, value_t p)
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
		const bool v = signal->is_guide_signal();
		sprintf( param, "%s,%i,s", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&bt_advance_to_end  ) {
		char param[256];
		bool v = signal->is_advance_to_end();
		sprintf( param, "%s,%i,a", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&bt_choose_signal  ) {
		char param[256];
		bool v = signal->is_choose_signal();
		sprintf( param, "%s,%i,o", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&numinp_tiles_margin  ) {
		char param[256];
		sprintf( param, "%s,%i,m,", signal->get_pos().get_str(), (uint8)p.i );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&bt_stop_before_check  ) {
		char param[256];
		bool v = signal->is_stop_before_check();
		sprintf( param, "%s,%i,t", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&bt_skip_default_route  ) {
		char param[256];
		bool v = signal->is_skip_default_route();
		sprintf( param, "%s,%i,d", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&bt_start_signal  ) {
		char param[256];
		bool v = signal->is_start_signal();
		sprintf( param, "%s,%i,p", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&bt_length_based  ) {
		char param[256];
		bool v = signal->is_length_based();
		sprintf( param, "%s,%i,l", signal->get_pos().get_str(), !v );
		tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
		welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
		return true;
	}
	if(  comp==&bt_two_ways  ) {
		char param[256];
		bool v = signal->get_two_ways();
		sprintf( param, "%s,%i,w", signal->get_pos().get_str(), !v );
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
	bt_advance_to_end.pressed = signal->is_advance_to_end();
	bt_choose_signal.pressed = signal->is_choose_signal();
	bt_stop_before_check.pressed = signal->is_stop_before_check();
	bt_skip_default_route.pressed = signal->is_skip_default_route();
	bt_start_signal.pressed = signal->is_start_signal();
	bt_length_based.pressed = signal->is_length_based();
	if(  signal->is_choose_signal()  ) {
		bt_advance_to_end.enable();
		numinp_tiles_margin.enable();
		numinp_tiles_margin.set_value(signal->get_margin_length());
		bt_length_based.enable();
	} else {
		bt_advance_to_end.disable();
		numinp_tiles_margin.disable();
		bt_stop_before_check.enable(signal->get_desc()->is_longblock_signal());
		bt_length_based.disable();
	}
	if(  signal->get_desc()->is_simple_signal()  ||  signal->get_desc()->is_longblock_signal()  ||  signal->get_desc()->is_choose_sign()  ) {
		bt_stop_before_check.enable();
	}
	bt_two_ways.pressed = signal->get_two_ways();
	bt_two_ways.enable( ribi_t::is_single(signal->get_dir()) );
	bt_remove_signal.enable( !signal->is_deletable( welt->get_active_player() ) );
}
