/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LABELLIST_STATS_T_H
#define GUI_LABELLIST_STATS_T_H


#include "../tpl/vector_tpl.h"
#include "components/gui_component.h"
#include "components/gui_button.h"


namespace labellist {
    enum sort_mode_t { by_name=0, by_koord, by_player, SORT_MODES };
};

/**
 * Curiosity list stats display
 * @author Hj. Malthaner
 */
class labellist_stats_t : public gui_world_component_t
{
private:
	vector_tpl<koord> labels;
	uint32 line_selected;

	uint32 last_world_labels;
	labellist::sort_mode_t sortby;
	bool sortreverse, filter;

public:
	labellist_stats_t(labellist::sort_mode_t sortby, bool sortreverse, bool filter);

	void get_unique_labels(labellist::sort_mode_t sortby, bool reverse, bool filter);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Recalc the size required to display everything and set size.
	 */
	void recalc_size();

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void draw(scr_coord offset) OVERRIDE;
};

#endif
