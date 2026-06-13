/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_DEPOT_PICKER_H
#define GUI_DEPOT_PICKER_H

#include "gui_frame.h"
#include "simwin.h"
#include "components/gui_scrollpane.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_combobox.h"
#include "components/gui_textinput.h"
#include "../convoihandle_t.h"
#include "../linehandle_t.h"
#include "../dataobj/koord3d.h"

class depot_t;
class player_t;


enum depot_sort_mode_t {
	DEPOT_SORT_ORIGINAL    = 0,   /// distance from (0,0), then by X — the original behaviour
	DEPOT_SORT_NAME        = 1,	  /// sort by name
	DEPOT_SORT_NEAREST     = 2,   /// distance from convoy position, only meaningful when called by a convoy
	DEPOT_SORT_ELECTRIFIED = 3,   /// show electrified depots first
	DEPOT_SORT_COUNT
};


/**
 * One row in the depot picker list.
 * Left-clicking selects that depot for the convoy (or teleports all line convoys).
 */
class depot_picker_item_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
	depot_t        *depot;
	convoihandle_t  cnv;
	linehandle_t    line;
	bool            teleport;   // true = 'Y' (immediate), false = 'D' (route)
	gui_image_t     waytype_symbol;
	gui_image_t     electrified_symbol;
	gui_label_buf_t label;
	button_t        gotopos;

	void update_label();
	void update_electrified();
public:
	/// Constructor for convoy context
	depot_picker_item_t(depot_t *depot, convoihandle_t cnv, bool teleport);
	/// Constructor for line context (always teleports all convoys)
	depot_picker_item_t(depot_t *depot, linehandle_t line);

	void draw(scr_coord pos) OVERRIDE;
	bool infowin_event(const event_t *) OVERRIDE;
	bool is_valid() const OVERRIDE;
	char const* get_text() const OVERRIDE { return ""; }

	/// Origin used by DEPOT_SORT_NEAREST; set to koord3d::invalid when unavailable.
	static koord3d sort_origin;
	static depot_sort_mode_t sort_mode;
	static bool compare(const gui_component_t *a, const gui_component_t *b);
};


/**
 * Modal-ish dialog: shows depots of the same waytype/owner as the convoy or line.
 * Clicking a depot sends (or teleports) the convoy (or all line convoys) there and closes.
 */
class depot_picker_t : public gui_frame_t, public action_listener_t
{
	gui_scrolled_list_t scrolly;
	gui_combobox_t      sort_combo;
	gui_textinput_t     filter_input;

	convoihandle_t cnv;
	linehandle_t   line;
	waytype_t      wt;
	player_t      *owner;
	bool           teleport;
	bool           has_nearest_sort;  ///< false when opened for a line

	char name_filter[64];

	void fill_list();
	void init_ui(const char *title);
public:
	/// Open picker for a single convoy
	depot_picker_t(convoihandle_t cnv, bool teleport);
	/// Open picker to teleport all convoys of a line to a chosen depot
	depot_picker_t(linehandle_t line, waytype_t wt, player_t *owner);
	~depot_picker_t();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	bool has_min_sizer() const OVERRIDE { return true; }
};

#endif
