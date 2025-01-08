/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCHEDULE_GUI_H
#define GUI_SCHEDULE_GUI_H


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
class zeiger_t;
class gui_schedule_entry_t;


/**
 * GUI for Schedule dialog
 */
class schedule_gui_t : public gui_frame_t, public action_listener_t
{
	enum mode_t {
		adding,
		inserting,
		removing,
		undefined_mode
	};

	mode_t mode;

	// only active with lines
	button_t bt_promote_to_line;
	gui_combobox_t line_selector, departure_slot_group_selector;
	gui_label_buf_t lb_waitlevel;

	// always needed
	button_t bt_add, bt_insert, bt_remove; // stop management
	button_t bt_revert, bt_return;
	button_t bt_wait_load;

	gui_label_t lb_wait, lb_load, lb_departure_slot_group;
	gui_numberinput_t numimp_load, numimp_wait_load;
	
	// for advanced settings
	// coupling, load/unload only, temp schedule, departure time, max_speed
	button_t bt_extract_settings;
	button_t bt_find_parent, bt_wait_for_child; // convoy coupling
	button_t bt_no_load, bt_no_unload, bt_tmp_schedule, bt_wait_for_time, 
		bt_same_dep_time, bt_full_load_acceleration, bt_full_load_time,bt_unload_all,bt_transfer_interval,
		bt_load_before_departure, bt_reverse_convoy, bt_reverse_coupling;
	gui_numberinput_t numimp_spacing, numimp_spacing_shift, 
		numimp_delay_tolerance, numimp_max_speed, numimp_tbgr_waiting_time;
	gui_label_t lb_spacing, lb_spacing_shift, lb_title1, lb_title2, lb_max_speed, lb_tbgr_waiting_time;
	char lb_spacing_str[20];
	char lb_spacing_shift_str[15];

	schedule_gui_stats_t* stats;
	gui_scrollpane_t scrolly;

	// to add new lines automatically
	uint32 old_line_count;
	uint32 last_schedule_count;

	char schedule_filter[256];
	char old_schedule_filter[256];
	gui_textinput_t name_filter_input;

	// set the correct tool now ...
	void update_tool(bool set);

	// changes the waiting/loading levels if allowed
	void update_selection();
	
	void extract_advanced_settings(bool yesno);
	
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
	void init_departure_slot_group_selector();

	bool infowin_event(event_t const*) OVERRIDE;

	const char *get_help_filename() const OVERRIDE {return "schedule.txt";}

	/**
	 * Draw the Frame
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
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


/**
 * List of displayed schedule entries.
 */
class schedule_gui_stats_t : public gui_aligned_container_t, action_listener_t, public gui_action_creator_t
{
	static cbuffer_t buf;

	vector_tpl<gui_schedule_entry_t*> entries;
	schedule_t *last_schedule; ///< last displayed schedule
	zeiger_t *current_stop_mark; ///< mark current stop on map
	const char* empty_message;
public:
	schedule_t *schedule;      ///< schedule under editing
	player_t*  player;

	schedule_gui_stats_t();
	~schedule_gui_stats_t();

	// shows/deletes highlighting of tiles
	void highlight_schedule(bool marking);
	void update_schedule();
	void draw(scr_coord offset) OVERRIDE;
	bool action_triggered(gui_action_creator_t *, value_t v) OVERRIDE;
};

#endif
