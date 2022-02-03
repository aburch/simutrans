/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_divider.h"
#include "../gui_theme.h"
#include "../../display/simgraph.h"


void gui_divider_t::draw(scr_coord offset)
{
	display_img_stretch( gui_theme_t::divider, scr_rect( get_pos()+offset, get_size() ) );
}


scr_size gui_divider_t::get_min_size() const
{
	return gui_theme_t::gui_divider_size;
}


scr_size gui_divider_t::get_max_size() const
{
	return scr_size(scr_size::inf.w, gui_theme_t::gui_divider_size.h);
}
