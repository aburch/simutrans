/*
 * Convoi information, name and status color
 */

#ifndef convoy_scrollitem_h
#define convoy_scrollitem_h

#include "components/gui_scrolled_list.h"
#include "../convoihandle_t.h"

/**
 * Container for list entries - consisting of text and color
 */
class convoy_scrollitem_t : public gui_scrolled_list_t::const_text_scrollitem_t
{
private:
	convoihandle_t cnv;
public:
	convoy_scrollitem_t( convoihandle_t c ) : gui_scrolled_list_t::const_text_scrollitem_t( NULL, COL_ORANGE ) { cnv = c; }
	COLOR_VAL get_color() OVERRIDE;
	convoihandle_t get_convoy() const { return cnv; }
	void set_color(COLOR_VAL) OVERRIDE { assert(false); }
	char const* get_text() const OVERRIDE;
	void set_text(char const*) OVERRIDE;
	bool is_editable() { return true; }
	bool is_valid() OVERRIDE { return cnv.is_bound(); }	//  can be used to indicate invalid entries
};

#endif
