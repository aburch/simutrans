/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCHEDULE_LIST_H
#define GUI_SCHEDULE_LIST_H


#include "gui_frame.h"
#include "components/gui_container.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_convoiinfo.h"
#include "halt_list_stats.h"
#include "../simline.h"

class player_t;

/**
 * Window displaying information about all schedules and lines.
 */
class schedule_list_gui_t : public gui_frame_t, public action_listener_t
{
private:
	player_t *player, *old_player;

	button_t bt_new_line, bt_edit_line, bt_delete_line, bt_withdraw_line;
	gui_scrolled_list_t scl, scrolly_convois, scrolly_haltestellen;
	gui_speedbar_t filled_bar;
	gui_textinput_t inp_name, inp_filter;
	gui_label_t lbl_filter;
	gui_chart_t chart;
	button_t filterButtons[MAX_LINE_COST];
	gui_tab_panel_t tabs;

	sint32 selection, capacity, load, loadfactor;

	uint32 old_line_count;
	schedule_t *last_schedule;
	uint32 last_vehicle_count;

	// only show schedules containing ...
	char schedule_filter[512], old_schedule_filter[512];

	// so even japanese can have long enough names ...
	char line_name[512], old_line_name[512];

	// resets textinput to current line name
	// necessary after line was renamed
	void reset_line_name();

	// rename selected line
	// checks if possible / necessary
	void rename_line();

	void display(scr_coord pos);

	void update_lineinfo(linehandle_t new_line);

	linehandle_t line;

	vector_tpl<linehandle_t> lines;

	void build_line_list(int filter);

public:
	/// last selected line per tab
	static linehandle_t selected_line[MAX_PLAYER_COUNT][simline_t::MAX_LINE_TYPE];


	schedule_list_gui_t(player_t* player_);
	~schedule_list_gui_t();

	/**
	 * in top-level windows the name is displayed in titlebar
	 * @return the non-translated component name
	 */
	const char* get_name() const { return "Line Management"; }

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char* get_help_filename() const OVERRIDE { return "linemanagement.txt"; }

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 */
	bool has_min_sizer() const OVERRIDE {return true;}

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
	 */
	void set_windowsize(scr_size size) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Select line and show its info
	 */
	void show_lineinfo(linehandle_t line);

	/**
	 * called after renaming of line
	 */
	void update_data(linehandle_t changed_line);

	void map_rotate90( sint16 ) OVERRIDE { update_lineinfo( line ); }

	// following: rdwr stuff
	void rdwr( loadsave_t *file ) OVERRIDE;
	uint32 get_rdwr_id() OVERRIDE;
};

#endif
