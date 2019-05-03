/*
 * part of the Simutrans project
 * @author hsiegeln
 * 01/12/2003
 */

#include "schedule_gui.h"

#include "../linehandle_t.h"

class player_t;
class loadsave_t;

class line_management_gui_t : public schedule_gui_t
{
public:
	line_management_gui_t(linehandle_t line = linehandle_t(), player_t* player = NULL);
	virtual ~line_management_gui_t();

	bool infowin_event(event_t const*) OVERRIDE;

	// stuff for UI saving
	void rdwr( loadsave_t *file ) OVERRIDE;
	uint32 get_rdwr_id() OVERRIDE { return magic_line_schedule_rdwr_dummy; }


private:
	linehandle_t line;
};
