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
 class karte_t;
 class loadsave_t;

 class line_management_gui_t : public fahrplan_gui_t
 {
public:
	line_management_gui_t(linehandle_t line, spieler_t* sp);
	virtual ~line_management_gui_t() { delete old_fpl; old_fpl = NULL; }	// since we pass a *copy* of the line's schedule to the base class
	const char * get_name() const;
	bool infowin_event(const event_t *ev);

	// stuff for UI saving
	line_management_gui_t(karte_t *welt);
	virtual void rdwr( loadsave_t *file );
	virtual uint32 get_rdwr_id() { return magic_line_schedule_rdwr_dummy; }


private:
	linehandle_t line;
};
