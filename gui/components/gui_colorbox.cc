/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_colorbox.h"
#include "../gui_theme.h"
#include "../../display/simgraph.h"

gui_colorbox_t::gui_colorbox_t(PIXVAL c)
{
	color = c;
	max_size = scr_size(scr_size::inf.w, D_INDICATOR_HEIGHT);
}


scr_size gui_colorbox_t::get_min_size() const
{
	return scr_size(D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT);
}


scr_size gui_colorbox_t::get_max_size() const
{
	return scr_size(max_size.w, D_INDICATOR_HEIGHT);
}


void gui_colorbox_t::draw(scr_coord offset)
{
	offset += pos;
	display_ddd_box_clip_rgb(offset.x, offset.y, size.w, D_INDICATOR_HEIGHT, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));
	display_fillbox_wh_clip_rgb(offset.x + 1, offset.y + 1, size.w - 2, D_INDICATOR_HEIGHT-2, color, true);
}
