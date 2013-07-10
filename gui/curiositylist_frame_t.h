/**
 * Curiosity list window
 * @author Hj. Malthaner
 */

#ifndef curiositylist_frame_t_h
#define curiositylist_frame_t_h

#include "../gui/gui_frame.h"
#include "../gui/curiositylist_stats_t.h"
#include "components/action_listener.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"

class karte_t;

class curiositylist_frame_t : public gui_frame_t, private action_listener_t
{
 private:
	static const char *sort_text[curiositylist::SORT_MODES];

	gui_label_t sort_label;
	button_t	sortedby;
	button_t	sorteddir;
	curiositylist_stats_t stats;
	gui_scrollpane_t scrolly;

	/*
	 * All filter settings are static, so they are not reset each
	 * time the window closes.
	 */
	static curiositylist::sort_mode_t sortby;
	static bool sortreverse;

 public:
	curiositylist_frame_t(karte_t * welt);

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
	const char * get_hilfe_datei() const {return "curiositylist_filter.txt"; }

	 /**
	 * This function refreshes the station-list
	 * @author Markus Weber
	 */
	void display_list();

	static curiositylist::sort_mode_t get_sortierung() { return sortby; }
	static void set_sortierung(const curiositylist::sort_mode_t sm) { sortby = sm; }

	static bool get_reverse() { return sortreverse; }
	static void set_reverse(const bool& reverse) { sortreverse = reverse; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
