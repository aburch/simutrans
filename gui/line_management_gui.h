/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LINE_MANAGEMENT_GUI_H
#define GUI_LINE_MANAGEMENT_GUI_H


#include "simwin.h"
#include "gui_frame.h"

#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_button_to_chart.h"
#include "components/gui_chart.h"
#include "components/gui_label.h"
#include "components/gui_schedule.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_speedbar.h"
#include "components/gui_tab_panel.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"

#include "../linehandle_t.h"

class player_t;
class loadsave_t;

class line_management_gui_t : public gui_frame_t, public action_listener_t
{
	linehandle_t line;

	gui_textinput_t inp_name;

	gui_label_t lb_convoi_count;
	cbuffer_t lb_convoi_count_text;
	gui_speedbar_t capacity_bar;
	cbuffer_t lb_profit_value_text;
	gui_label_t lb_profit_value;
	button_t bt_delete_line;

	gui_tab_panel_t switch_mode;
	gui_schedule_t scd;

	gui_chart_t chart;
	gui_button_to_chart_array_t button_to_chart;

	button_t bt_withdraw_line, bt_find_convois;
	gui_scrolled_list_t scrolly_convois, scrolly_halts;

	gui_aligned_container_t container_schedule, container_stats, container_convois, container_halts;

	cbuffer_t loading_text;
	gui_textarea_t loading_info;

	player_t *player;

	// so even japanese can have long enough names ...
	char line_name[512], old_line_name[512];

	// rename selected line
	// checks if possible / necessary
	void rename_line();

	uint32 old_convoi_count, old_halt_count;

	sint32 capacity, load;

	void init();

	void apply_schedule();

public:
	line_management_gui_t(linehandle_t line = linehandle_t(), player_t* player_ = NULL, int active_tab=0 );

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	// stuff for UI saving
	void rdwr( loadsave_t *file ) OVERRIDE;

	const char *get_help_filename() const OVERRIDE { return "linedetails.txt"; }

	bool infowin_event( const event_t *ev ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_line_schedule_rdwr_dummy; }
};

#endif
