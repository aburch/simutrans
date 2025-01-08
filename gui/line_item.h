/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LINE_ITEM_H
#define GUI_LINE_ITEM_H


#include "components/gui_scrolled_list.h"
#include "../linehandle_t.h"
#include "../player/simplay.h"
#include "../simline.h"

/**
 * Container for list entries - consisting of text and color
 */
class line_scrollitem_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
private:
	linehandle_t line;
public:
	// helper to sort
	enum sort_modes_t {
		SORT_BY_NAME = 0,
		SORT_BY_ID,
		SORT_BY_PROFIT,
		SORT_BY_TRANSPORTED,
		SORT_BY_CONVOIS,
		SORT_BY_DISTANCE,
		MAX_SORT_MODES
	};
	static sort_modes_t sort_mode;
	static bool sort_reverse;
	// normal items
	line_scrollitem_t( linehandle_t l ) : gui_scrolled_list_t::const_text_scrollitem_t( NULL, color_idx_to_rgb(COL_ORANGE) ) { line = l; }
	PIXVAL get_color() const OVERRIDE;
	linehandle_t get_line() const { return line; }
	char const* get_text() const OVERRIDE;
	void set_text(char const*) OVERRIDE;
	bool is_valid() const OVERRIDE { return line.is_bound(); } //  can be used to indicate invalid entries
	bool is_editable() const OVERRIDE { return true; }
	static bool compare(const gui_component_t *a, const gui_component_t *b );
};

class company_color_line_scroll_item_t: public line_scrollitem_t 
{
public:
	company_color_line_scroll_item_t( linehandle_t l ) : line_scrollitem_t(l) {};
	PIXVAL get_color() const OVERRIDE {
		const uint8 color_idx = get_line()->get_owner()->get_player_color1();
		return color_idx_to_rgb(color_idx);
	}
};

#endif
