/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CITYLIST_FRAME_T_H
#define GUI_CITYLIST_FRAME_T_H


#include "gui_frame.h"
#include "citylist_stats_t.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_button_to_chart.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_combobox.h"

// for the number of cost entries
#include "../simworld.h"

/**
 * City list window
 */
class citylist_frame_t : public gui_frame_t, private action_listener_t
{

 private:
    static const char *sort_text[citylist_stats_t::SORT_MODES];

	static const char hist_type[karte_t::MAX_WORLD_COST][21];
	static const char hist_type_tooltip[karte_t::MAX_WORLD_COST][256];
	static const uint8 hist_type_color[karte_t::MAX_WORLD_COST];
	static const uint8 hist_type_type[karte_t::MAX_WORLD_COST];

	gui_combobox_t sortedby;
	button_t sort_asc, sort_desc;
	button_t	filter_within_network;

    //gui_scrollpane_t scrolly;
	gui_scrolled_list_t scrolly;

	gui_aligned_container_t container_year, container_month;
	gui_chart_t chart, mchart;
	gui_button_to_chart_array_t button_to_chart;
	gui_tab_panel_t year_month_tabs;
	gui_tab_panel_t main;

	gui_aligned_container_t list, statistics;
	gui_label_buf_t citizens;
	gui_label_updown_t fluctuation_world;

	void fill_list();
	void update_label();
	/*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static citylist_stats_t::sort_mode_t sortby;

 public:

    citylist_frame_t();

   /**
     * Draw new component. The values to be passed refer to the window
     * i.e. It's the screen coordinates of the window where the
     * component is displayed.
     */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool has_min_sizer() const OVERRIDE {return true;}

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	*/
	const char * get_help_filename() const OVERRIDE {return "citylist_filter.txt"; }

    static void set_sortierung(const citylist_stats_t::sort_mode_t& sm) { sortby = sm; }

    bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
