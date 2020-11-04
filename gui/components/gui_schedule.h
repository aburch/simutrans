/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_SCHEDULE_H
#define GUI_COMPONENTS_GUI_SCHEDULE_H


#include "gui_label.h"
#include "gui_numberinput.h"
#include "gui_aligned_container.h"
#include "gui_button.h"
#include "gui_action_creator.h"
#include "gui_combobox.h"
#include "gui_scrollpane.h"
#include "action_listener.h"

#include "../../convoihandle_t.h"
#include "../../linehandle_t.h"
#include "../../tpl/vector_tpl.h"


class schedule_t;
class player_t;
class cbuffer_t;
class loadsave_t;
class schedule_gui_stats_t;


/**
 * GUI for Schedule dialog
 */
class gui_schedule_t :
	public gui_action_creator_t,
	public gui_aligned_container_t,
	public action_listener_t
{
private:
	enum mode_t {
		adding = 0,
		inserting,
		removing,
		MAX_MODE
	};

	mode_t mode;

	static const char *bt_mode_text[ MAX_MODE ];
	static const char *bt_mode_tooltip[ MAX_MODE ];

	gui_label_buf_t lb_waitlevel;

	// always needed
	button_t bt_apply, bt_revert, bt_return, bt_mode;

	gui_label_t lb_wait, lb_load;
	gui_numberinput_t numimp_load;
	gui_combobox_t wait_load;

	schedule_gui_stats_t* stats;
	gui_scrollpane_t scrolly;

	bool make_return; // either reverting line or add mirror
	bool no_editing; // if convoi schedule is part of a line, if has to be removed from it

	// set the correct tool now ...
	void update_tool(bool set);

	// changes the waiting/loading levels if allowed
	void update_selection();

	schedule_t *get_old_schedule() const;

protected:
	// only one of these is possible
	schedule_t *schedule_mode;
	linehandle_t line_mode;
	convoihandle_t convoi_mode;

	// the actual data
	schedule_t * schedule, *old_schedule;
	player_t *player;
	uint8 current_schedule_rotation;	// to detect the rotation of the map independently

public:
	gui_schedule_t(schedule_t* schedule = NULL, player_t* player = NULL, convoihandle_t cnv = convoihandle_t(), linehandle_t lin = linehandle_t() );

	void init(schedule_t* schedule = NULL, player_t* player = NULL, convoihandle_t cnv = convoihandle_t(), linehandle_t lin = linehandle_t() );

	virtual ~gui_schedule_t();

	void draw(scr_coord pos) OVERRIDE;

	scr_size get_max_size() const OVERRIDE { return scr_size::inf; }

	void set_size(scr_size size) OVERRIDE;

	schedule_t *get_schedule() const { return schedule; }

	void highlight_schedule( bool hl );

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr( loadsave_t *file );

	bool is_marginless() const OVERRIDE { return true; }
};

#endif
