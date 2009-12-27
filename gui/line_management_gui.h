/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "fahrplan_gui.h"
#include "components/gui_textinput.h"
#include "components/action_listener.h"
#include "gui_frame.h"

#include "../linehandle_t.h"

 class spieler_t;

 class line_management_gui_t : public fahrplan_gui_t
 {
public:
	line_management_gui_t(linehandle_t line, spieler_t* sp);
	const char * get_name() const;
	void infowin_event(const event_t *ev);

private:
	linehandle_t line;
};
