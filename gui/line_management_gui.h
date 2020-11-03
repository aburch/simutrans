/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LINE_MANAGEMENT_GUI_H
#define GUI_LINE_MANAGEMENT_GUI_H


#include "simwin.h"
#include "gui_frame.h"
#include "components/gui_schedule.h"

#include "../linehandle_t.h"

class player_t;
class loadsave_t;

class line_management_gui_t : public gui_frame_t
{
	gui_schedule_t scd;
	linehandle_t line;

public:
	line_management_gui_t(linehandle_t line = linehandle_t(), player_t* player_ = NULL);

	bool infowin_event(event_t const*) OVERRIDE;

	// stuff for UI saving
	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_line_schedule_rdwr_dummy; }
};

#endif
