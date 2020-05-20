/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HALT_LIST_STATS_H
#define GUI_HALT_LIST_STATS_H


#include "components/gui_component.h"
#include "../halthandle_t.h"


/**
 * @author Hj. Malthaner
 */
class halt_list_stats_t : public gui_world_component_t
{
private:
	halthandle_t halt;

public:
	halt_list_stats_t() : halt() {}
	halt_list_stats_t(halthandle_t halt_) : halt(halt_) { size.h = 28; }
	const halthandle_t get_halt() const { return halt; }

	bool infowin_event(event_t const*) OVERRIDE;

	void draw(scr_coord offset) OVERRIDE;
};

#endif
