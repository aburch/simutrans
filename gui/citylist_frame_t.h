/*
 * The citylist dialog
 */

#ifndef citylist_frame_t_h
#define citylist_frame_t_h

#include "gui_frame.h"
#include "citylist_stats_t.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"

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

	static const char hist_type[karte_t::MAX_WORLD_COST][20];
	static const uint8 hist_type_color[karte_t::MAX_WORLD_COST];
	static const uint8 hist_type_type[karte_t::MAX_WORLD_COST];

	static karte_t *welt;

	gui_label_t sort_label;

	button_t	sortedby;
    button_t	sorteddir;

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

 public:

    citylist_frame_t(karte_t * welt);

   /**
     * Draw new component. The values to be passed refer to the window
     * i.e. It's the screen coordinates of the window where the
     * component is displayed.
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    void resize(const koord delta);

    /**
     * Set the window associated helptext
     * @return the filename for the helptext, or NULL
     * @author V. Meyer
     */
    const char * get_hilfe_datei() const {return "citylist_filter.txt"; }

    static citylist::sort_mode_t get_sortierung() { return sortby; }
    static void set_sortierung(const citylist::sort_mode_t& sm) { sortby = sm; }

    static bool get_reverse() { return sortreverse; }
    static void set_reverse(const bool& reverse) { sortreverse = reverse; }

    bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
