/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_OWNER_H
#define GUI_OWNER_H


#include "gui_component.h"

// colorbox for shared ownership
class gui_owners_t : public gui_component_t
{
protected:
	uint16 num_players;
	uint16 player_mask;

public:
	void set_owners(uint16 m);

	gui_owners_t() { set_owners(0); }

	void draw(scr_coord offset) OVERRIDE;

	scr_size get_min_size() const OVERRIDE;
	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};


#endif
