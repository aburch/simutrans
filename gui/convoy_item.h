/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CONVOY_ITEM_H
#define GUI_CONVOY_ITEM_H


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
	PIXVAL get_color() OVERRIDE;
	convoihandle_t get_convoy() const { return cnv; }
	void set_color(PIXVAL) OVERRIDE { assert(false); }
	char const* get_text() const OVERRIDE;
	void set_text(char const*) OVERRIDE;
	bool is_editable() OVERRIDE { return true; }
	bool is_valid() OVERRIDE { return cnv.is_bound(); }	//  can be used to indicate invalid entries
};

#endif
