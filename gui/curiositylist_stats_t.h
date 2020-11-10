/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CURIOSITYLIST_STATS_T_H
#define GUI_CURIOSITYLIST_STATS_T_H


#include "../tpl/vector_tpl.h"
#include "components/gui_component.h"
#include "components/gui_button.h"

class gebaeude_t;

namespace curiositylist {
    enum sort_mode_t { by_name=0, by_paxlevel, by_pax_arrived/*, by_maillevel*/, SORT_MODES };
};

/**
 * Where curiosity (attractions) stats are calculated for list dialog
 * @author Hj. Malthaner
 */
class curiositylist_stats_t : public gui_world_component_t
{
private:
	vector_tpl<gebaeude_t*> attractions;
	uint32 line_selected;

	uint32 last_world_curiosities;
	curiositylist::sort_mode_t sortby;
	bool sortreverse;
	bool filter_own_network;

public:
	curiositylist_stats_t(curiositylist::sort_mode_t sortby, bool sortreverse, bool own_network);

	void get_unique_attractions(curiositylist::sort_mode_t sortby, bool reverse, bool own_network);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Recalc the size required to display everything and set size (size).
	 */
	void recalc_size();

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void draw(scr_coord offset) OVERRIDE;
};

#endif
