/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CURIOSITYLIST_STATS_T_H
#define GUI_CURIOSITYLIST_STATS_T_H


#include "components/gui_aligned_container.h"
#include "components/gui_colorbox.h"
#include "components/gui_scrolled_list.h"

class gebaeude_t;

namespace curiositylist {
	enum sort_mode_t {
		by_name=0,
		by_paxlevel,
//		by_maillevel,
		SORT_MODES
	};
};

/**
 * Where curiosity (attractions) stats are calculated for list dialog
 */
class curiositylist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	gebaeude_t* attraction;
	gui_colorbox_t indicator;

public:
	static curiositylist::sort_mode_t sortby;
	static bool sortreverse;
	static bool compare(const gui_component_t *a, const gui_component_t *b );

	curiositylist_stats_t(gebaeude_t *att);

	char const* get_text() const OVERRIDE;
	bool is_valid() const OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	void draw(scr_coord offset) OVERRIDE;
};

#endif
