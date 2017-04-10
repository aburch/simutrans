/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where factory stats are calculated for list dialog
 */

#ifndef factorylist_stats_t_h
#define factorylist_stats_t_h

#include "components/gui_scrolled_list.h"
#include "../display/simgraph.h"
#include "../simfab.h"

class karte_ptr_t;
class fabrik_t;


namespace factorylist {
    enum sort_mode_t { by_name=0, by_available, by_output, by_maxprod, by_status, by_power, SORT_MODES, by_input, by_transit,  };	// the last two not used
};

/**
 * Factory list stats display
 * @author Hj. Malthaner
 */
// City list stats display
class factorylist_stats_t : public gui_scrolled_list_t::scrollitem_t
{
private:
	fabrik_t *fab;
	bool mouse_over;  // flag to remember, whether last pressed mouse button on this

	static scr_coord_val h;

	static bool compare( gui_scrolled_list_t::scrollitem_t *a, gui_scrolled_list_t::scrollitem_t *b );

public:
	static int sort_mode;
	static bool reverse;

public:
	factorylist_stats_t(fabrik_t *);

	scr_coord_val get_height() const { return h; }

	scr_coord_val draw( scr_coord pos, scr_coord_val width, bool is_selected, bool has_focus );
	char const* get_text() const { return fab->get_name(); }
	bool sort( vector_tpl<scrollitem_t *> &, int, void * ) const;
	bool infowin_event(const event_t *);
};


#endif
