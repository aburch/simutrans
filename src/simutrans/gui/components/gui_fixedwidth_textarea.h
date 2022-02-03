/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_FIXEDWIDTH_TEXTAREA_H
#define GUI_COMPONENTS_GUI_FIXEDWIDTH_TEXTAREA_H


#include "gui_component.h"
#include "gui_container.h"
#include "../../display/simgraph.h"

class cbuffer_t;

/**
 * A fixed-width, automatically line-wrapping text-area,
 * optionally with a reserved area in the upper right corner.
 * It does *not* add 10px margins from the top and the left.
 * Core code borrowed from ding_infowin_t::calc_draw_info() with adaptation.
 */
class gui_fixedwidth_textarea_t : public gui_component_t
{
private:
	/// Pointer to the text to be displayed. The text is *not* copied.
	cbuffer_t* buf;

	/// Reserved area in the upper right corner
	scr_size reserved_area;

	/**
	 * For calculating text height and/or displaying the text.
	 */
	scr_size calc_display_text(const scr_coord offset, const bool draw) const;

public:
	gui_fixedwidth_textarea_t(cbuffer_t* buf, const sint16 width);

	// call this after buffer size changes
	void recalc_size();

	// this sets a maximum width. The actual width may be smaller, depending on the text
	void set_width(scr_coord_val width);

	void set_reserved_area(const scr_size area);

	void draw(scr_coord offset) OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;
};

#endif
