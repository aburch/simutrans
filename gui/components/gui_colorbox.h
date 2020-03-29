/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_COLORBOX_H
#define GUI_COMPONENTS_GUI_COLORBOX_H


#include "gui_component.h"
#include "../../simcolor.h"

/**
 * Draws a simple colored box.
 */
class gui_colorbox_t : public gui_component_t
{
	PIXVAL color;

	scr_size max_size;
public:
	gui_colorbox_t(PIXVAL c = 0);

	void draw(scr_coord offset) OVERRIDE;

	void set_color(PIXVAL c)
	{
		color = c;
	}

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	void set_max_size(scr_size s)
	{
		max_size = s;
	}
};

#endif
