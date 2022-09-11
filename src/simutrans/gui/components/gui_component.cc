/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_component.h"
#include "../../display/scr_coord.h"

#include "../../world/simworld.h"

karte_ptr_t gui_world_component_t::welt;

void gui_component_t::align_to(gui_component_t* component_par, scr_coord offset_par )
{
	if (component_par) {

		scr_coord new_pos    = get_pos();
		scr_size new_size    = get_size();
		scr_coord target_pos = component_par->get_pos();
		scr_size target_size = component_par->get_size();

		new_pos.y = target_pos.y + offset_par.y + (target_size.h - new_size.h) / 2;
		new_pos.x = target_pos.x + target_size.w + offset_par.x;

		// apply new position and size
		set_pos(new_pos);
		set_size(new_size);
	}

}
