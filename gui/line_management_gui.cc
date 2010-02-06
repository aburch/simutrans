/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "line_management_gui.h"
#include "../dataobj/translator.h"
#include "../simline.h"
#include "../simwin.h"
#include "../simwerkz.h"
#include "../simlinemgmt.h"


line_management_gui_t::line_management_gui_t(linehandle_t line, spieler_t* sp) :
	fahrplan_gui_t(line->get_schedule()->copy(), sp, convoihandle_t() )
{
	this->line = line;
	line->prepare_for_update();
	show_line_selector(false);
}


const char *line_management_gui_t::get_name() const
{
	return translator::translate("Line Management");
}

void line_management_gui_t::infowin_event(const event_t *ev)
{
	if(!line.is_bound()) {
		destroy_win( this );
	}
	else {
		fahrplan_gui_t::infowin_event(ev);
		if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
			// Added by : Knightly
			// Check if the schedule is modified
			if ( line->is_schedule_updated() )
			{
				// update all convoys of this line!
				// update line schedule via tool!
				werkzeug_t *w = create_tool( WKZ_LINE_TOOL | SIMPLE_TOOL );
				cbuffer_t buf(5500);
				buf.printf( "g,%i,", line.get_id() );
				fpl->sprintf_schedule( buf );
				w->set_default_param(buf);
				line->get_besitzer()->get_welt()->set_werkzeug( w, line->get_besitzer() );
				// since init always returns false, it is save to delete immediately
				delete w;
			}
		}
	}
}
