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

#include "../tpl/vector_tpl.h"
#include "components/gui_component.h"

class fabrik_t;


namespace factorylist {
    enum sort_mode_t { by_name=0, by_available, by_output, by_maxprod, by_status, by_power, by_sector, SORT_MODES, by_input, by_transit,  };	// the last two not used
};

/**
 * Factory list stats display
 * @author Hj. Malthaner
 */
class factorylist_stats_t : public gui_world_component_t
{
private:
	vector_tpl<fabrik_t*> fab_list;
	uint32 line_selected;

	factorylist::sort_mode_t sortby;
	bool sortreverse;
	bool filter_own_network;

public:
	factorylist_stats_t(factorylist::sort_mode_t sortby, bool sortreverse, bool own_network);

	void sort(factorylist::sort_mode_t sortby, bool sortreverse, bool own_network);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Recalc the size required to display everything and set size (size).
	 */
	void recalc_size();

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void draw(scr_coord offset);
};

#endif
