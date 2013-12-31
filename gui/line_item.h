#ifndef line_scrollitem_h
#define line_scrollitem_h

#include "components/gui_scrolled_list.h"
#include "../linehandle_t.h"

/**
 * Container for list entries - consisting of text and color
 */
class line_scrollitem_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
private:
	linehandle_t line;
	static bool compare( gui_scrolled_list_t::scrollitem_t *a, gui_scrolled_list_t::scrollitem_t *b );
public:
	// helper to sort
	enum sort_modes_t { SORT_BY_NAME=0, SORT_BY_ID, SORT_BY_PROFIT, SORT_BY_TRANSPORTED, SORT_BY_CONVOIS, SORT_BY_DISTANCE, MAX_SORT_MODES };
	static sort_modes_t sort_mode;
	virtual bool sort( vector_tpl<scrollitem_t *>&v, int, void * ) const OVERRIDE;
	// normal items
	line_scrollitem_t( linehandle_t l ) : gui_scrolled_list_t::const_text_scrollitem_t( NULL, COL_ORANGE ) { line = l; }
	COLOR_VAL get_color() OVERRIDE;
	linehandle_t get_line() const { return line; }
	void set_color(COLOR_VAL) OVERRIDE { assert(false); }
	char const* get_text() const OVERRIDE;
	void set_text(char const*) OVERRIDE;
	bool is_valid() OVERRIDE { return line.is_bound(); }	//  can be used to indicate invalid entries
	bool is_editable() { return true; }
};

#endif
