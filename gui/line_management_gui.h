/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "schedule_gui.h"
#include "components/gui_textinput.h"
#include "components/action_listener.h"
#include "gui_frame.h"

#include "../linehandle_t.h"

class player_t;
class loadsave_t;

class line_management_gui_t : public schedule_gui_t
{
public:
	line_management_gui_t(linehandle_t line, player_t* player);
	virtual ~line_management_gui_t();

	bool infowin_event(event_t const*) OVERRIDE;

	// stuff for UI saving
	line_management_gui_t();
	virtual void rdwr( loadsave_t *file );
	virtual uint32 get_rdwr_id() { return magic_line_schedule_rdwr_dummy; }


private:
	linehandle_t line;
};
