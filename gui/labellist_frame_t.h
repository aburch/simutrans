/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LABELLIST_FRAME_T_H
#define GUI_LABELLIST_FRAME_T_H


#include "../gui/gui_frame.h"
#include "../gui/labellist_stats_t.h"
#include "components/action_listener.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/gui_combobox.h"


/**
 * label list window
 * @author Hj. Malthaner
 */
class labellist_frame_t : public gui_frame_t, private action_listener_t
{
 private:
    static const char *sort_text[labellist::SORT_MODES];

    gui_label_t sort_label;
	gui_combobox_t	sortedby;
	button_t sort_asc, sort_desc;
	button_t	filter;
    labellist_stats_t stats;

    gui_scrollpane_t scrolly;
    /*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static labellist::sort_mode_t sortby;
    static bool sortreverse;
	static bool filter_state;
 public:
    labellist_frame_t();

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
	const char * get_help_filename() const OVERRIDE {return "labellist_filter.txt"; }

     /**
     * This function refreshes the station-list
     * @author Markus Weber
     */
    void display_list();

    static void set_sortierung(const labellist::sort_mode_t sm) { sortby = sm; }

    static bool get_reverse() { return sortreverse; }
    static void set_reverse(const bool& reverse) { sortreverse = reverse; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// rotated map need new info ...
	void map_rotate90( sint16 ) OVERRIDE { display_list(); }
};

#endif
