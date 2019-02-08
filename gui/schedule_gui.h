/*
 * Dialog window for defining a schedule
 *
 * Hj. Malthaner
 *
 * Juli 2000
 */

#ifndef gui_schedule_gui_h
#define gui_schedule_gui_h

#include "gui_frame.h"

#include "components/gui_label.h"
#include "components/gui_numberinput.h"
#include "components/gui_combobox.h"
#include "components/gui_button.h"
#include "components/action_listener.h"

#include "components/gui_scrollpane.h"

#include "../convoihandle_t.h"
#include "../linehandle_t.h"
#include "simwin.h"
#include "../tpl/vector_tpl.h"


class schedule_t;
class player_t;
class cbuffer_t;
class loadsave_t;
class schedule_gui_stats_t;


/**
 * GUI for Schedule dialog
 *
 * @author Hj. Malthaner
 */
class schedule_gui_t :	public gui_frame_t,
						public action_listener_t
{
	enum mode_t {adding, inserting, removing, undefined_mode};

	mode_t mode;

	// only active with lines
	button_t bt_promote_to_line;
	gui_combobox_t line_selector;
	gui_label_buf_t lb_waitlevel;

	// always needed
	button_t bt_add, bt_insert, bt_remove; // stop management
	button_t bt_return;

	gui_label_t lb_wait, lb_load;
	gui_numberinput_t numimp_load;
	gui_combobox_t wait_load;

	schedule_gui_stats_t* stats;
	gui_scrollpane_t scrolly;

	// to add new lines automatically
	uint32 old_line_count;
	uint32 last_schedule_count;

	// set the correct tool now ...
	void update_tool(bool set);

	// changes the waiting/loading levels if allowed
	void update_selection();
protected:
	schedule_t *schedule;
	schedule_t* old_schedule;
	player_t *player;
	convoihandle_t cnv;

	linehandle_t new_line, old_line;

	void init(schedule_t* schedule, player_t* player, convoihandle_t cnv);

public:
	schedule_gui_t(schedule_t* schedule = NULL, player_t* player = NULL, convoihandle_t cnv = convoihandle_t());

	virtual ~schedule_gui_t();

	// for updating info ...
	void init_line_selector();

	bool infowin_event(event_t const*) OVERRIDE;

	const char *get_help_filename() const OVERRIDE {return "schedule.txt";}

	/**
	 * Draw the Frame
	 * @author Hansjörg Malthaner
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
	 * @author Hj. Malthaner
	 */
	void set_windowsize(scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Map rotated, rotate schedules too
	 */
	void map_rotate90( sint16 ) OVERRIDE;

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_schedule_rdwr_dummy; }
};

#endif
