/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CITYLIST_STATS_T_H
#define GUI_CITYLIST_STATS_T_H


#include "components/gui_aligned_container.h"
#include "components/gui_label.h"
#include "components/gui_scrolled_list.h"
#include "../simcity.h"

#include "components/gui_image.h"

class stadt_t;



// City list stats display
class citylist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	stadt_t* city;

	gui_label_buf_t lb_name, lb_region;
	gui_label_buf_t label;
	gui_image_t electricity, alert;
	gui_label_updown_t fluctuation_city;
	void update_label();

public:
	enum sort_mode_t { SORT_BY_NAME=0, SORT_BY_SIZE, SORT_BY_GROWTH, SORT_BY_REGION, SORT_MODES };
	static sort_mode_t sort_mode;
	static bool sortreverse, filter_own_network;
	static uint16 name_width;

public:
	citylist_stats_t(stadt_t *);

	void draw( scr_coord pos) OVERRIDE;

	char const* get_text() const OVERRIDE { return city->get_name(); }
	bool is_valid() const OVERRIDE;
	bool infowin_event(const event_t *) OVERRIDE;
	void set_size(scr_size size) OVERRIDE;

	static bool compare(const gui_component_t *a, const gui_component_t *b);
};

#endif
