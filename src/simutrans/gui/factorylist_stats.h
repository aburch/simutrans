/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FACTORYLIST_STATS_H
#define GUI_FACTORYLIST_STATS_H


#include "components/gui_colorbox.h"
#include "components/gui_image.h"
#include "components/gui_label.h"
#include "components/gui_scrolled_list.h"
#include "../simfab.h"

class fabrik_t;


namespace factorylist {
	enum sort_mode_t {
		by_name = 0,
		by_available,
		by_output,
		by_maxprod,
		by_status,
		by_power,
		SORT_MODES,

		// the last two are unused
		by_input,
		by_transit
	};
};

/**
 * Factory list stats display
 * Where factory stats are calculated for list dialog
 */
class factorylist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	fabrik_t *fab;
	gui_colorbox_t indicator;
	gui_image_t boost_electric, boost_passenger, boost_mail;
	gui_label_buf_t label;

	void update_label();
public:
	static sint16 sort_mode;
	static bool reverse;

	factorylist_stats_t(fabrik_t *);

	void draw( scr_coord pos) OVERRIDE;

	char const* get_text() const OVERRIDE { return fab->get_name(); }
	bool infowin_event(const event_t *) OVERRIDE;
	bool is_valid() const OVERRIDE;
	void set_size(scr_size size) OVERRIDE;

	static bool compare(const gui_component_t *a, const gui_component_t *b );
};


#endif
