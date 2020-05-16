/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FACTORYLIST_FRAME_T_H
#define GUI_FACTORYLIST_FRAME_T_H


#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "factorylist_stats_t.h"
#include "components/gui_combobox.h"


/*
 * Factory list window
 * @author Hj. Malthaner
 */
class factorylist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static const char *sort_text[factorylist::SORT_MODES];

	gui_label_t sort_label;
	gui_combobox_t	sortedby;
	button_t	sorteddir;
	button_t	filter_within_network;
	factorylist_stats_t stats;
	gui_scrollpane_t scrolly;

	/*
	 * All filter settings are static, so they are not reset each
	 * time the window closes.
	 */
	static factorylist::sort_mode_t sortby;
	static bool sortreverse;
	static bool filter_own_network;

public:
	factorylist_frame_t();

	/**
	 * resize window in response to a resize event
	 * @author Hj. Malthaner
	 */
	void resize(const scr_coord delta);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const {return "factorylist_filter.txt"; }

	static factorylist::sort_mode_t get_sortierung() { return sortby; }
	static void set_sortierung(const factorylist::sort_mode_t& sm) { sortby = sm; }

	static bool get_reverse() { return sortreverse; }
	static void set_reverse(const bool& reverse) { sortreverse = reverse; }
	static bool get_filter_own_network() { return filter_own_network; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
