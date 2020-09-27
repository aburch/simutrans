/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_SPEEDBAR_H
#define GUI_COMPONENTS_GUI_SPEEDBAR_H


#include "gui_component.h"
#include "../../tpl/slist_tpl.h"


class gui_speedbar_t : public gui_component_t
{
private:
	struct info_t {
		PIXVAL color;
		const sint32 *value;
		sint32 last;
	};

	slist_tpl <info_t> values;

	sint32 base;
	bool vertical;

public:
	gui_speedbar_t() { base = 100; vertical = false; }

	void add_color_value(const sint32 *value, PIXVAL color);

	void set_base(sint32 base);

	void set_vertical(bool vertical) { this->vertical = vertical; }

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset) OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;
};


// route progress bar
class gui_routebar_t : public gui_component_t
{
private:
	const sint32 *value;
	sint32 base;
	uint8 state;

public:
	gui_routebar_t() { base = 100; state = 0; }
	void set_base(sint32 base);
	void init(const sint32 *value, uint8 state);

	void set_state(uint8 state);

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset);

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;
};

#endif
