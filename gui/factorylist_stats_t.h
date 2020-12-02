/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FACTORYLIST_STATS_T_H
#define GUI_FACTORYLIST_STATS_T_H


#include "../tpl/vector_tpl.h"
#include "components/gui_component.h"

class fabrik_t;


namespace factorylist {
    enum sort_mode_t { by_name=0, by_available, by_output, by_maxprod, by_status, by_power, by_sector, by_staffing, by_operation_rate, by_region, SORT_MODES, by_input, by_transit  };	// the last two not used
};

/**
 * Factory list stats display
 * Where factory stats are calculated for list dialog
 */
class factorylist_stats_t : public gui_world_component_t
{
private:
	vector_tpl<fabrik_t*> fab_list;
	uint32 line_selected;

	factorylist::sort_mode_t sortby;
	bool sortreverse;
	bool filter_own_network;
	uint8 filter_goods_catg;

public:
	factorylist_stats_t(factorylist::sort_mode_t sortby, bool sortreverse, bool own_network, uint8 goods_catg_index);

	void sort(factorylist::sort_mode_t sortby, bool sortreverse, bool own_network, uint8 goods_catg_index);

	uint8 display_mode = 0;
	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Recalc the size required to display everything and set size (size).
	 */
	void recalc_size();

/*	char const* get_text() const OVERRIDE { return fab->get_name(); }
	bool infowin_event(const event_t *) OVERRIDE;
	bool is_valid() const OVERRIDE;
	void set_size(scr_size size) OVERRIDE;
*/

	void draw(scr_coord offset) OVERRIDE;
};

#endif
