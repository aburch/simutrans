/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_DIVIDER_H
#define GUI_COMPONENTS_GUI_DIVIDER_H


#include "gui_component.h"
#include "../../display/simgraph.h"
#include "../../simskin.h"

class skinverwaltung_t;

/**
 * A horizontal divider line
 *
 * @date 30-Oct-01
 * @author Markus Weber
 */
class gui_divider_t : public gui_component_t
{
public:
	gui_divider_t() { size.h = D_DIVIDER_HEIGHT; }

	void init( scr_coord xy, scr_coord_val width, scr_coord_val height = D_DIVIDER_HEIGHT ) {
		set_pos( xy );
		set_size( scr_size( width, height ) );
	};

	void set_width(scr_coord_val width) { set_size(scr_size(width, size.h)); }

	scr_size get_size() const { return scr_size(size.w,max(size.h, D_DIVIDER_HEIGHT)); }

	void draw(scr_coord offset)
	{
		display_img_stretch( gui_theme_t::divider, scr_rect( get_pos()+offset, get_size() ) );
	}
};

#endif
