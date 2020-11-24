/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CURIOSITYLIST_STATS_T_H
#define GUI_CURIOSITYLIST_STATS_T_H


#include "components/gui_aligned_container.h"
#include "components/gui_colorbox.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_image.h"

class gebaeude_t;

namespace curiositylist {
    enum sort_mode_t { by_name=0, by_paxlevel, by_pax_arrived, /*by_city,*/ by_region, SORT_MODES };
};

/**
 * Where curiosity (attractions) stats are calculated for list dialog
 */
class curiositylist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	gebaeude_t* attraction;

	enum { no_networks = 0, someones_network = 1, own_network = 2 };

	gui_label_t lb_name;

public:
	static uint8 sort_mode, region_filter;
	static bool sortreverse, filter_own_network;
	static bool compare(const gui_component_t *a, const gui_component_t *b);

	curiositylist_stats_t(gebaeude_t *att);

	gui_image_t img_enabled[4];

	char const* get_text() const OVERRIDE;
	bool is_valid() const OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	*/
	void draw(scr_coord offset) OVERRIDE;
};

#endif
