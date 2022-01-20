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
#include "gui_scrolled_list.h"
#include "gui_timeinput.h"
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
	// always needed
	button_t bt_revert, bt_return;

	gui_label_t lb_load_str, lb_departure_str;
	gui_numberinput_t numimp_load;
	gui_combobox_t cb_wait, insert_mode;

	schedule_gui_stats_t* stats;
	gui_scrollpane_t scrolly;

	gui_aligned_container_t *loading_details;

	gui_timeinput_t departure;

	bool make_return; // either reverting line or add mirror
	bool no_editing; // if convoi schedule is part of a line, if has to be removed from it

	// set the correct tool now ...
	void update_tool(bool set);

	// changes the waiting/loading levels if allowed
	void update_selection();

	schedule_t *get_old_schedule() const;

protected:
	// schedule of this line (or of convoi belonging to this line) is being edited
	linehandle_t line_mode;
	// schedule of this convoi is being edited
	convoihandle_t convoi_mode;

	// the schedule that is edited, copy of old_schedule
	schedule_t *schedule;
	// the original schedule (used to detect calls to init with same original schedule), might be non-null but invalid, do not dereference
	schedule_t *old_schedule;

	player_t *player;
	uint8 current_schedule_rotation;	// to detect the rotation of the map independently

public:
	gui_schedule_t();

	void init(schedule_t* schedule = NULL, player_t* player = NULL, convoihandle_t cnv = convoihandle_t(), linehandle_t lin = linehandle_t() );

	virtual ~gui_schedule_t();

	void draw(scr_coord pos) OVERRIDE;

	// true if the schedule has changed
	bool has_pending_changes() { return bt_revert.enabled(); }

	scr_size get_max_size() const OVERRIDE { return scr_size::inf; }

	void set_size(scr_size size) OVERRIDE;

	schedule_t *get_schedule() const { return schedule; }

	void highlight_schedule( bool hl );

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr( loadsave_t *file );

	bool is_marginless() const OVERRIDE { return true; }
};

#endif
