/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HALT_LIST_FRAME_H
#define GUI_HALT_LIST_FRAME_H


#include "gui_frame.h"
#include "halt_list_stats.h"
#include "components/gui_button.h"
#include "components/action_listener.h"
#include "../tpl/vector_tpl.h"

class player_t;
class goods_desc_t;
class gui_scrolled_halt_list_t;

/**
 * Displays a scrollable list of all stations of a player
 */
class halt_list_frame_t : public gui_frame_t , private action_listener_t
{
public:
	enum sort_mode_t { nach_name, nach_wartend, nach_typ, SORT_MODES };
	enum filter_flag_t { any_filter=1, name_filter=2, typ_filter=4,
		spezial_filter=8, ware_ab_filter=16, ware_an_filter=32,
		sub_filter=64,  // Start sub-filter from here!
		frachthof_filter=64,
		bahnhof_filter=128,
		bushalt_filter=256,
		dock_filter=512,
		airport_filter=1024,
		ueberfuellt_filter=2048,
		ohneverb_filter=4096,
		monorailstop_filter=8192,
		maglevstop_filter=16384,
		narrowgaugestop_filter=32768,
		tramstop_filter=65536
	};

private:
	player_t *m_player;

	static const char *sort_text[SORT_MODES];

	uint32 last_world_stops;

	/*
	 * All gui elements of this dialog:
	 */
	button_t	sortedby;
	button_t	sorteddir;
	button_t	filter_on;
	button_t	filter_details;
	gui_scrolled_halt_list_t *scrolly;

	/*
	 * Child window, if open
	 */
	gui_frame_t *filter_frame;

	/*
	 * All filter settings are static, so they are not reset each
	 * time the window closes.
	 */
	static sort_mode_t sortby;
	static bool sortreverse;

	static int filter_flags;

	static char name_filter_value[64];

	static slist_tpl<const goods_desc_t *> waren_filter_ab;
	static slist_tpl<const goods_desc_t *> waren_filter_an;

	/// refill the list of halt info elements
	void fill_list();

public:

	static bool compare_halts(halthandle_t, halthandle_t);

	halt_list_frame_t(player_t *player);

	~halt_list_frame_t();

	/**
	 * The filter frame tells us when it is closed.
	 */
	void filter_frame_closed() { filter_frame = NULL; }

	// must be handled, because we could not use scrollpanel
	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	/// sort & filter halts in list
	void sort_list();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char *get_help_filename() const OVERRIDE {return "haltlist.txt"; }

	static sort_mode_t get_sortierung() { return sortby; }
	static void set_sortierung(sort_mode_t sm) { sortby = sm; }

	static bool get_reverse() { return sortreverse; }
	static void set_reverse(bool reverse) { sortreverse = reverse; }

	static bool get_filter(filter_flag_t filter) { return (filter_flags & filter) != 0; }
	static void set_filter(filter_flag_t filter, bool on) { filter_flags = on ? (filter_flags | filter) : (filter_flags & ~filter); }

	static char *access_name_filter() { return name_filter_value; }

	static bool get_ware_filter_ab(const goods_desc_t *ware) { return waren_filter_ab.is_contained(ware); }
	static void set_ware_filter_ab(const goods_desc_t *ware, int mode);
	static void set_alle_ware_filter_ab(int mode);

	static bool get_ware_filter_an(const goods_desc_t *ware) { return waren_filter_an.is_contained(ware); }
	static void set_ware_filter_an(const goods_desc_t *ware, int mode);
	static void set_alle_ware_filter_an(int mode);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool has_min_sizer() const OVERRIDE {return true;}

	void map_rotate90( sint16 ) OVERRIDE { fill_list(); }
};

#endif
