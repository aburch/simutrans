/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_map_preview.h"
#include "../../world/simworld.h"

gui_map_preview_t::gui_map_preview_t() :
	gui_component_t()
{
	map_data = NULL;
	set_size (scr_size( MAP_PREVIEW_SIZE_X,MAP_PREVIEW_SIZE_Y ));
}
