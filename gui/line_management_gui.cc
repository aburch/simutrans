/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "line_management_gui.h"
#include "../simplay.h"
#include "../simline.h"
#include "../simlinemgmt.h"


line_management_gui_t::line_management_gui_t(karte_t *welt, linehandle_t line, spieler_t *sp) :
	fahrplan_gui_t(welt, line->get_fahrplan(), sp)
{
	this->line = line;
	this->sp = sp;
	line->prepare_for_update();
	show_line_selector(false);
}

const char *
line_management_gui_t::gib_name() const
{
    return translator::translate("Line Management");
}

void
line_management_gui_t::infowin_event(const event_t *ev)
{
	if(!line.is_bound()) {
		destroy_win( this );
	}
	else {
		if(ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE) {
			// update all convoys of this line!
			sp->simlinemgmt.update_line(line);
		}
		fahrplan_gui_t::infowin_event(ev);
	}
}
