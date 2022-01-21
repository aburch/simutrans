/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CONVOI_FRAME_H
#define GUI_CONVOI_FRAME_H


#include "gui_frame.h"
#include "simwin.h"

#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_textinput.h"
#include "components/gui_waytype_tab_panel.h"
#include "../convoihandle_t.h"

class player_t;
class goods_desc_t;
class gui_scrolled_convoy_list_t;

/**
 * Displays a scrollable list of all convois of a player
 */
class convoi_frame_t :
	public gui_frame_t,
	private action_listener_t
{
public:
	enum sort_mode_t {
		nach_name   = 0,
		nach_gewinn = 1,
		nach_typ    = 2,
		nach_id     = 3,
		SORT_MODES  = 4
	};

private:
	player_t *owner;

	static const char *sort_text[SORT_MODES];

	/// number of convoys the last time we checked.
	uint32 last_world_convois;

	// these are part of the top UI
	gui_combobox_t sortedby;
	button_t sorteddir;
	button_t filter_details;

	static char name_filter[256], last_name_filter[256];
	gui_textinput_t name_filter_input;

	// scroll container of list of convois
	gui_scrolled_convoy_list_t *scrolly;

	gui_waytype_tab_panel_t tabs;

	// actual filter setting
	static const slist_tpl<const goods_desc_t *>*waren_filter;
	static uint32 filter_flags;
	static waytype_t current_wt;

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

public:

	static bool compare_convois(convoihandle_t, convoihandle_t);

	/**
	 * Check all filters for one convoi.
	 * returns true, if it is not filtered away.
	 */
	bool passes_filter(convoihandle_t cnv);

	/**
	 * Resorts convois
	 */
	void sort_list( uint32 filter, const slist_tpl<const goods_desc_t *> *wares );

	convoi_frame_t();

	~convoi_frame_t();

	/**
	 * Events werden hiermit an die GUI-Komponenten gemeldet
	 */
	bool infowin_event(const event_t *ev) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	const char * get_help_filename() const OVERRIDE {return "convoi.txt"; }

	static sort_mode_t get_sortierung() { return sortby; }
	static void set_sortierung(sort_mode_t sm) { sortby = sm; }

	static bool get_reverse() { return sortreverse; }
	static void set_reverse(bool reverse) { sortreverse = reverse; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_convoi_list; }
};

#endif
