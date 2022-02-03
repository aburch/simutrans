/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LINE_ITEM_H
#define GUI_LINE_ITEM_H


#include "components/gui_scrolled_list.h"
#include "../linehandle.h"

/**
 * Container for list entries - consisting of text and color
 */
class line_scrollitem_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
public:
	// helper to sort
	enum sort_modes_t {
		SORT_BY_NAME = 0,
		SORT_BY_ID,
		SORT_BY_PROFIT,
		SORT_BY_REVENUE,
		SORT_BY_TRANSPORTED,
		SORT_BY_CONVOIS,
		SORT_BY_DISTANCE,
		MAX_SORT_MODES
	};
	// selection mode
	enum select_modes_t {
		SELECT_ITEM, ///< show selected: if item is selected in list to apply line for convoy
		SELECT_WIN,  ///< show selected: if line window is open
	};
	static sort_modes_t sort_mode;
	static bool sort_reverse;
	// to update selected status
	void draw( scr_coord offset ) OVERRIDE;
	// normal items
	line_scrollitem_t( linehandle_t l, select_modes_t sm = SELECT_ITEM) : gui_scrolled_list_t::const_text_scrollitem_t( NULL, color_idx_to_rgb(COL_ORANGE) ), select_mode(sm) { line = l; }
	PIXVAL get_color() const OVERRIDE;
	linehandle_t get_line() const { return line; }
	char const* get_text() const OVERRIDE;
	void set_text(char const*) OVERRIDE;
	bool is_valid() const OVERRIDE { return line.is_bound(); } //  can be used to indicate invalid entries
	bool is_editable() const OVERRIDE { return true; }
	static bool compare(const gui_component_t *a, const gui_component_t *b );

private:
	linehandle_t line;
	select_modes_t select_mode;
};

#endif
