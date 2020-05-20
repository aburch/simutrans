/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CITYLIST_STATS_T_H
#define GUI_CITYLIST_STATS_T_H


#include "components/gui_component.h"
#include "../tpl/vector_tpl.h"

class karte_ptr_t;
class stadt_t;


namespace citylist {
	enum sort_mode_t { by_name=0, by_size, by_growth, SORT_MODES };
};


// City list stats display
class citylist_stats_t : public gui_world_component_t
{
private:
	vector_tpl<stadt_t*> city_list;
	uint32 line_selected;

	citylist::sort_mode_t sortby;
	bool sortreverse;
	bool filter_own_network;

public:
	static char total_bev_string[128];

	citylist_stats_t(citylist::sort_mode_t sortby, bool sortreverse, bool own_network);

	void sort(citylist::sort_mode_t sortby, bool sortreverse, bool own_network);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Recalc the size required to display everything and set size.
	 */
	void recalc_size();

	// Draw the component
	void draw(scr_coord offset) OVERRIDE;
};

#endif
