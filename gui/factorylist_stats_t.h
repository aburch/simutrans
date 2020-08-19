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
    enum sort_mode_t { by_name=0, by_available, by_output, by_maxprod, by_status, by_power, by_sector, by_staffing, by_operation_rate, SORT_MODES, by_input, by_transit,  };	// the last two not used
};

/**
 * Factory list stats display
 * Where factory stats are calculated for list dialog
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
	uint8 filter_goods_catg;

public:
	factorylist_stats_t(factorylist::sort_mode_t sortby, bool sortreverse, bool own_network, uint8 goods_catg_index);

	void sort(factorylist::sort_mode_t sortby, bool sortreverse, bool own_network, uint8 goods_catg_index);

	bool display_operation_stats = false;
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
