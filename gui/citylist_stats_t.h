/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where the citylist status are calculated (for graphs and statistics)
 */

#ifndef CITYLIST_STATS_T_H
#define CITYLIST_STATS_T_H

#include "components/gui_scrolled_list.h"
#include "../display/simgraph.h"
#include "../simcity.h"

class karte_ptr_t;
class stadt_t;


// City list stats display
class citylist_stats_t : public gui_scrolled_list_t::scrollitem_t
{
private:
	stadt_t* city;
	bool mouse_over;  // flag to remember, whether last presse mopuse button on this

	static scr_coord_val h;

	static bool compare( gui_scrolled_list_t::scrollitem_t *a, gui_scrolled_list_t::scrollitem_t *b );

public:
	enum sort_mode_t { SORT_BY_NAME=0, SORT_BY_SIZE, SORT_BY_GROWTH, SORT_MODES, SORT_REVERSE=0x80 };
	static sort_mode_t sort_mode;

public:
	citylist_stats_t(stadt_t *);

	scr_coord_val get_h() const { return h; }

	scr_coord_val draw( scr_coord pos, scr_coord_val width, bool is_selected, bool has_focus );
	char const* get_text() const { return city->get_name(); }
	bool sort( vector_tpl<scrollitem_t *> &, int, void * );
	bool infowin_event(const event_t *);
};

#endif
