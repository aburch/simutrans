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

#include "components/gui_textarea.h"
#include "components/gui_scrollpane.h"

#include "../convoihandle_t.h"
#include "../linehandle_t.h"
#include "../gui/simwin.h"
#include "../tpl/vector_tpl.h"


class zeiger_t;
class schedule_t;
struct schedule_entry_t;
class player_t;
class cbuffer_t;
class loadsave_t;


class schedule_gui_stats_t : public gui_world_component_t
{
private:
	static cbuffer_t buf;
	static zeiger_t *current_stop_mark;


	schedule_t* schedule;
	player_t* player;

public:
	schedule_gui_stats_t(player_t *player_);
	~schedule_gui_stats_t();

	void set_schedule( schedule_t* f ) { schedule = f; }

	void highlight_schedule( schedule_t *markschedule, bool marking );

	// Draw the component
	void draw(scr_coord offset);
};



/**
 * GUI for Schedule dialog
 *
 * @author Hj. Malthaner
 */
class schedule_gui_t :	public gui_frame_t,
						public action_listener_t
{
 public:
	/**
     * Fills buf with description of schedule's i'th entry.
	 *
	 * @author Hj. Malthaner
	 */
	static void gimme_stop_name(cbuffer_t & buf, const player_t *player_, const schedule_entry_t &entry, bool no_control_tower = false );

	/**
	 * Append description of entry to buf.
	 * short version, without loading level and position
	 */
	static void gimme_short_stop_name(cbuffer_t& buf, player_t const* player_, const schedule_t *schedule, int i, int max_chars);

private:
	enum mode_t {adding, inserting, removing, undefined_mode};

	mode_t mode;

	// only active with lines
	button_t bt_promote_to_line;
	gui_combobox_t line_selector;
	gui_label_t lb_line;

	// always needed
	button_t bt_add, bt_insert, bt_remove; // stop management
	button_t bt_bidirectional, bt_mirror, bt_wait_for_time, bt_same_spacing_shift;

	button_t bt_wait_prev, bt_wait_next;	// waiting in parts of month
	gui_label_t lb_wait, lb_waitlevel_as_clock;

	gui_label_t lb_load;
	gui_numberinput_t numimp_load;

	gui_label_t lb_spacing;
	gui_numberinput_t numimp_spacing;
	gui_label_t lb_spacing_as_clock;

	gui_label_t lb_spacing_shift;
	gui_numberinput_t numimp_spacing_shift;
	gui_label_t lb_spacing_shift_as_clock;

	char str_ladegrad[16];
	char str_parts_month[32];
	char str_parts_month_as_clock[32];

	char str_spacing_as_clock[32];
	char str_spacing_shift_as_clock[32];

	schedule_gui_stats_t stats;
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

public:
	schedule_gui_t(schedule_t* schedule, player_t* player_, convoihandle_t cnv);

	virtual ~schedule_gui_t();

	// for updating info ...
	void init_line_selector();

	bool infowin_event(event_t const*) OVERRIDE;

	const char *get_help_filename() const {return "schedule.txt";}

	/**
	 * Draw the Frame
	 * @author Hansjörg Malthaner
	 */
	void draw(scr_coord pos, scr_size size);

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
	 * @author Hj. Malthaner
	 */
	virtual void set_windowsize(scr_size size);

	/**
	 * show or hide the line selector combobox and its associated label
	 * @author hsiegeln
	 */
	void show_line_selector(bool yesno) {
		line_selector.set_visible(yesno);
		lb_line.set_visible(yesno);
	}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Map rotated, rotate schedules too
	 */
	void map_rotate90( sint16 );

	// this constructor is only used during loading
	schedule_gui_t();

	virtual void rdwr( loadsave_t *file );

	uint32 get_rdwr_id() { return magic_schedule_rdwr_dummy; }
};

#endif
