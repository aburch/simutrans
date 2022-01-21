/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CITYLIST_FRAME_T_H
#define GUI_CITYLIST_FRAME_T_H


#include "simwin.h"
#include "gui_frame.h"
#include "citylist_stats_t.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_button_to_chart.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_combobox.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"

// for the number of cost entries
#include "../simworld.h"

/**
 * City list window
 */
class citylist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static const char *sort_text[citylist_stats_t::SORT_MODES];

	static const char hist_type[karte_t::MAX_WORLD_COST][20];
	static const uint8 hist_type_color[karte_t::MAX_WORLD_COST];
	static const uint8 hist_type_type[karte_t::MAX_WORLD_COST];

	gui_combobox_t sortedby;
	button_t sorteddir;
	button_t filter_by_owner;
	gui_combobox_t filterowner;

	static char name_filter[256];
	char last_name_filter[256];
	gui_textinput_t name_filter_input;

	gui_scrolled_list_t scrolly;

	gui_aligned_container_t container_year, container_month;
	gui_chart_t chart, mchart;
	gui_button_to_chart_array_t button_to_chart;
	gui_tab_panel_t year_month_tabs;
	gui_tab_panel_t main;

	gui_aligned_container_t list, statistics;
	gui_label_buf_t citizens;

	uint32 old_city_count, old_halt_count;

	void fill_list();
	void update_label();
/*
 * All filter settings are static, so they are not reset each
 * time the window closes.
 */
static bool sortreverse;

public:
	citylist_frame_t();

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	const char *get_help_filename() const OVERRIDE {return "citylist_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE { fill_list(); }

	void rdwr(loadsave_t* file) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_citylist_frame_t; }
};

#endif
