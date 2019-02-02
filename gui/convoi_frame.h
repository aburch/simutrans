/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CONVOI_FRAME_H
#define GUI_CONVOI_FRAME_H


#include "gui_frame.h"
#include "components/action_listener.h"  // 28-Dec-2001  Markus Weber    Added
#include "components/gui_button.h"
#include "components/gui_convoiinfo.h"
#include "../convoihandle_t.h"

class player_t;
class goods_desc_t;
class gui_scrolled_convoy_list_t;

/*
 * Displays a scrollable list of all convois of a player
 *
 * @author Hj. Malthaner, Sort/Filtering by V. Meyer
 * @date 15-Jun-01
 */
class convoi_frame_t :
	public gui_frame_t,
	private action_listener_t  //28-Dec-01     Markus Weber    Added , private action_listener_t
{
public:
	enum sort_mode_t { by_name = 0, by_profit, by_type, by_id, by_power, SORT_MODES };

private:
	player_t *owner;

	static const char *sort_text[SORT_MODES];

	/// number of convoys the last time we checked.
	uint32 last_world_convois;

	// these are part of the top UI
	button_t	sortedby;
	button_t	sorteddir;
	button_t	display_mode;
	button_t	filter_on;
	button_t	filter_details;

	// scroll container of list of convois
	gui_scrolled_convoy_list_t *scrolly;

	// actual filter setting
	bool filter_is_on;
	const slist_tpl<const goods_desc_t *>*waren_filter;
	char *name_filter;
	uint32 filter_flags;

	bool get_filter(uint32 filter) { return (filter_flags & filter) != 0; }
	void set_filter(uint32 filter, bool on) { filter_flags = on ? (filter_flags | filter) : (filter_flags & ~filter); }

	/// sort & filter convoys in list
	void sort_list();

	/// refill the list of convoy info elements
	void fill_list();

	/*
	 * All filter settings are static, so they are not reset each
	 * time the window closes.
	 */
	static sort_mode_t sortby;
	static bool sortreverse;

	inline uint8 get_cinfo_height(uint8 cl_display_mode)
	{
		switch (cl_display_mode) {
		case gui_convoiinfo_t::cnvlist_formation:
			return 55;
		case gui_convoiinfo_t::cnvlist_payload:
		case gui_convoiinfo_t::cnvlist_normal:
		default:
			return 40;
		}
	}

public:

	static bool compare_convois(convoihandle_t, convoihandle_t);

	/**
	 * Check all filters for one convoi.
	 * returns true, if it is not filtered away.
	 * @author V. Meyer
	 */
	bool passes_filter(convoihandle_t cnv);
	/**
	 * Resorts convois
	 */
	void sort_list( char *name, uint32 filter, const slist_tpl<const goods_desc_t *> *wares );

	convoi_frame_t(player_t *player);

	virtual ~convoi_frame_t();

	/**
	 * Events werden hiermit an die GUI-components
	 * gemeldet
	 * @author V. Meyer
	 */
	bool infowin_event(const event_t *ev) OVERRIDE;

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const OVERRIDE {return "convoi.txt"; }

	static sort_mode_t get_sortierung() { return sortby; }
	static void set_sortierung(sort_mode_t sm) { sortby = sm; }

	static bool get_reverse() { return sortreverse; }
	static void set_reverse(bool reverse) { sortreverse = reverse; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
