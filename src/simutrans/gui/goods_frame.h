/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GOODS_FRAME_T_H
#define GUI_GOODS_FRAME_T_H


#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_numberinput.h"
#include "components/gui_textarea.h"
#include "components/gui_combobox.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "goods_stats.h"
#include "../simline.h"
#include "../utils/cbuffer.h"

class goods_desc_t;

/**
 * Shows statistics. Only goods so far.
 */
class goods_frame_t : public gui_frame_t, private action_listener_t
{
private:
	enum sort_mode_t {
		unsortiert  = 0,
		nach_name   = 1,
		nach_gewinn = 2,
		nach_bonus  = 3,
		nach_catg   = 4,
		SORT_MODES  = 5
	};
	static const char *sort_text[SORT_MODES];

	// Variables used for remembering last state of window when closed.

	// The last selected speed.
	static sint32 selected_speed;

	// If the last selected speed was the average speed so as to allow selected speed to follow average speed over time.
	static bool average_selection;

	/**
	 * This variable defines by which column the table is sorted
	 * Values: 0 = Unsorted (passengers and mail first)
	 *         1 = Alphabetical
	 *         2 = Revenue
	 */
	static sort_mode_t sortby;

	/**
	 * This variable defines the sort order (ascending or descending)
	 * Values: 1 = ascending, 2 = descending)
	 */
	static bool sortreverse;

	/**
	 * This variable controls whether all goods, way types and speeds are displayed, or
	 * just the ones relevant to the current game
	 * Values: false = all goods, way types and maximum speed shown, true = goods with factories, way types with vehicles and current best speed shown.
	 */
	static bool filter_goods;

	static simline_t::linetype last_scheduletype;

	cbuffer_t speed_message;
	vector_tpl<const goods_desc_t*> good_list;

	gui_combobox_t sortedby;
	button_t sorteddir;

	gui_numberinput_t speed;
	gui_combobox_t    scheduletype;

	gui_textarea_t speed_text;
	gui_aligned_container_t *sort_row;

	button_t filter_goods_toggle;

	goods_stats_t goods_stats;
	gui_scrollpane_t scrolly;

	/* Array to map linetype combobox selection to linetypes.
	 * Allows the combo box to be filtered or sorted.
	 */
	simline_t::linetype linetype_selection_map[simline_t::MAX_LINE_TYPE];

	/* Builds the linetype combobox list.
	 * When show_used is specified then only types which have at least 1 powered vehicle will be shown.
	 */
	void build_linetype_list(bool const show_used);

	// creates the list and pass it to the child function good_stats, which does the display stuff ...
	static bool compare_goods(goods_desc_t const* const w1, goods_desc_t const* const w2);

	/* Changes the currently set linetype.
	 * Updates the selectable speed limit as well as automatically adjusts the value to keep the same bonus.
	 * A non 0 speed_value will set the speed value while a 0 value will automatically adjust.
	 */
	void set_linetype(simline_t::linetype const linetype, sint32 const speed_value = 0);

	void sort_list();

public:
	goods_frame_t();

	// yes we can reload
	uint32 get_rdwr_id() OVERRIDE;
	void rdwr( loadsave_t *file ) OVERRIDE;

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE {return "goods_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
