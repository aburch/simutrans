/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
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

	uint8 height = D_INDICATOR_HEIGHT;
	uint8 width = D_INDICATOR_WIDTH;
	bool size_fixed = false;
	bool show_frame = true;

	const char * tooltip;

	scr_size max_size;
public:
	gui_colorbox_t(PIXVAL c = 0);

	void init(PIXVAL color_par, scr_size size, bool fixed=false, bool show_frame=true) {
		set_color(color_par);
		set_size(size);
		set_size_fixed(fixed);
		set_show_frame(show_frame);
	}

	void draw(scr_coord offset) OVERRIDE;

	void set_color(PIXVAL c)
	{
		color = c;
	}

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	void set_size(scr_size size) OVERRIDE { width = size.w; height = size.h; max_size =size; };
	void set_size_fixed(bool yesno) { size_fixed = yesno; };
	void set_show_frame(bool yesno) { show_frame = yesno; };

	void set_tooltip(const char * t);

	void set_max_size(scr_size s)
	{
		max_size = s;
	}
};

#endif
