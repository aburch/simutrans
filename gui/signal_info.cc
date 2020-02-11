#include "signal_info.h"
#include "../obj/signal.h"
#include "../player/simplay.h"


#include "../simmenu.h"
#include "../simworld.h"

signal_info_t::signal_info_t(signal_t* s) :
	obj_infowin_t(s),
	signal(s)
{
	bt.init( button_t::square_state, translator::translate("Require parent convoy to enter."));
	bt.add_listener(this);
	bt.pressed = signal->is_guide_signal();
	set_table_layout(1,0);
	add_component(&bt);
	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}

bool signal_info_t::action_triggered( gui_action_creator_t* , value_t /* */)
{
	char param[256];
	bool v = signal->is_guide_signal();
	sprintf( param, "%s,%i,s", signal->get_pos().get_str(), !v );
	tool_t::simple_tool[TOOL_CHANGE_ROADSIGN]->set_default_param( param );
	welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_ROADSIGN], welt->get_active_player() );
	return true;
}


// notify for an external update
void signal_info_t::update_data()
{
	bt.pressed = signal->is_guide_signal();
}
