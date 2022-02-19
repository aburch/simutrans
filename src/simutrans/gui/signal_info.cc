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

signal_info_t::signal_info_t(signal_t *s) :
	obj_infowin_t(s),
	sig(s)
{
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


bool signal_info_t::action_triggered( gui_action_creator_t *, value_t /* */)
{
	bool suspended_execution=false;
	koord3d pos = sig->get_pos();
	tool_t::general_tool[TOOL_REMOVE_SIGNAL]->set_default_param(NULL);
	const char *err = welt->call_work( tool_t::general_tool[TOOL_REMOVE_SIGNAL], welt->get_active_player(), pos, suspended_execution );
	if(!suspended_execution) {
		// play sound / error message
		welt->get_active_player()->tell_tool_result(tool_t::general_tool[TOOL_REMOVE_SIGNAL], pos, err);
	}
	return true;
}


void signal_info_t::draw( scr_coord pos, scr_size size )
{
	remove.enable( !sig->is_deletable( welt->get_active_player() ) );
	obj_infowin_t::draw( pos, size );
}
