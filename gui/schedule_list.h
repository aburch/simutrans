/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
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
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "components/gui_convoiinfo.h"
#include "halt_list_stats.h"
#include "../simline.h"

class player_t;
class schedule_list_gui_t : public gui_frame_t, public action_listener_t
{
public:
	enum sort_mode_t { by_name = 0, by_schedule, by_profit, by_loading_lvl, by_max_speed, by_power, by_value, by_age, SORT_MODES };
private:
	player_t *player, *old_player;

	static const char *sort_text[SORT_MODES];

	button_t bt_new_line, bt_change_line, bt_delete_line, bt_withdraw_line, bt_line_class_manager, bt_times_history, bt_mode_convois;
	button_t sort_asc, sort_desc;
	gui_container_t cont, cont_haltestellen, cont_charts, cont_convoys;
	gui_scrollpane_t scrolly_convois, scrolly_haltestellen;
	gui_scrolled_list_t scl;
	gui_speedbar_t filled_bar;
	gui_textinput_t inp_name, inp_filter;
	gui_label_t lbl_filter;
	gui_chart_t chart;
	button_t filterButtons[MAX_LINE_COST];
	gui_tab_panel_t tabs; // line selector
	gui_tab_panel_t info_tabs;

	// vector of convoy info objects that are being displayed
	vector_tpl<gui_convoiinfo_t *> convoy_infos;
	vector_tpl<convoihandle_t> line_convoys;

	// vector of stop info objects that are being displayed
	vector_tpl<halt_list_stats_t *> stop_infos;

	sint32 selection, capacity, load, loadfactor;

	uint32 old_line_count;
	schedule_t *last_schedule;
	uint32 last_vehicle_count;

	button_t filter_btn_all_pas, filter_btn_all_mails, filter_btn_all_freights;
	uint8 line_type_flags = simline_t::all_ftype;

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

	static uint16 livery_scheme_index;
	vector_tpl<uint16> livery_scheme_indices;

	cbuffer_t tab_name;

	// sort stuff
	static sort_mode_t sortby;
	static bool sortreverse;

	static bool compare_convois(convoihandle_t, convoihandle_t);
	void sort_list();

	gui_combobox_t livery_selector, sortedby;

	uint8 get_filter_type_bits() { return line_type_flags; }

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

	static void set_sortierung(const sort_mode_t sm) { sortby = sm; }

	static bool get_reverse() { return sortreverse; }
	static void set_reverse(bool reverse) { sortreverse = reverse; }

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
	virtual void set_windowsize(scr_size size) OVERRIDE;

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

	// following: rdwr stuff
	void rdwr( loadsave_t *file ) OVERRIDE;
	uint32 get_rdwr_id() OVERRIDE;
};

#endif
