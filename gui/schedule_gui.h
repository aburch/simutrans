/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCHEDULE_GUI_H
#define GUI_SCHEDULE_GUI_H


#include "simwin.h"
#include "gui_frame.h"

#include "components/action_listener.h"
#include "components/gui_schedule.h"
#include "components/gui_combobox.h"
#include "components/gui_button.h"

class player_t;


/**
 * GUI for Schedule dialog
 */
class schedule_gui_t : public gui_frame_t, public action_listener_t
{
	gui_combobox_t line_selector;

	convoihandle_t cnv;

	// to add new lines automatically
	uint32 old_line_count;
	linehandle_t line, old_line;

	player_t *player;

	gui_schedule_t scd;

public:
	schedule_gui_t(schedule_t* schedule = NULL, player_t* player = NULL, convoihandle_t cnv = convoihandle_t());

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_schedule_rdwr_dummy; }

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered( gui_action_creator_t *comp, value_t p ) OVERRIDE;

	void init_line_selector();
};

#endif
