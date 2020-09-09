#ifndef gui_colorbox_h
#define gui_colorbox_h

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

	const char * tooltip;

	scr_size max_size;
public:
	gui_colorbox_t(PIXVAL c = 0);

	void init(PIXVAL color_par, scr_size size, bool yesno) {
		set_color(color_par);
		set_size(size);
		set_size_fixed(yesno);
	}

	void draw(scr_coord offset);

	void set_color(PIXVAL c)
	{
		color = c;
	}

	scr_size get_min_size() const;

	scr_size get_max_size() const;

	void set_size(scr_size size) OVERRIDE { width = size.w; height = size.h; };
	void set_size_fixed(bool yesno) { size_fixed = yesno; };

	void set_tooltip(const char * t);

	void set_max_size(scr_size s)
	{
		max_size = s;
	}
};

#endif
