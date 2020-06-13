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
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_combobox.h"

// for the number of cost entries
#include "../simworld.h"

/**
 * City list window
 * @author Hj. Malthaner
 */
class citylist_frame_t : public gui_frame_t, private action_listener_t
{

 private:
    static const char *sort_text[citylist::SORT_MODES];

	static const char hist_type[karte_t::MAX_WORLD_COST][21];
	static const char hist_type_tooltip[karte_t::MAX_WORLD_COST][256];
	static const uint8 hist_type_color[karte_t::MAX_WORLD_COST];
	static const uint8 hist_type_type[karte_t::MAX_WORLD_COST];

	gui_label_t sort_label;

	gui_combobox_t sortedby;
	button_t sort_asc, sort_desc;
	button_t	filter_within_network;

    citylist_stats_t stats;
    gui_scrollpane_t scrolly;

    button_t	show_stats;
	gui_chart_t chart, mchart;
	button_t	filterButtons[karte_t::MAX_WORLD_COST];
	gui_tab_panel_t year_month_tabs;

    /*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static citylist::sort_mode_t sortby;
    static bool sortreverse;
	static bool filter_own_network;

 public:

    citylist_frame_t();

   /**
     * Draw new component. The values to be passed refer to the window
     * i.e. It's the screen coordinates of the window where the
     * component is displayed.
     * @author Hj. Malthaner
     */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
	void resize(const scr_coord delta) OVERRIDE;

    /**
     * Set the window associated helptext
     * @return the filename for the helptext, or NULL
     * @author V. Meyer
     */
	const char * get_help_filename() const OVERRIDE {return "citylist_filter.txt"; }

    static void set_sortierung(const citylist::sort_mode_t& sm) { sortby = sm; }

    static bool get_reverse() { return sortreverse; }
    static void set_reverse(const bool& reverse) { sortreverse = reverse; }
	static bool get_filter_own_network() { return filter_own_network; }

    bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
