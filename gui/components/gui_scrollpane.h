/*
 * this is a scrolling area in which subcomponents are drawn
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_scrollpane_h
#define gui_scrollpane_h


#include "gui_component.h"
#include "gui_scrollbar.h"

class gui_scrollpane_t : public gui_component_t
{
private:
	/**
	 * The scrolling component
	 * @author Hj. Malthaner
	 */
	gui_component_t *comp;
	scr_size old_komp_size;

	/**
	 * Scrollbar X/Y
	 * @author Hj. Malthaner
	 */
	scrollbar_t scroll_x, scroll_y;

	bool b_show_scroll_x:1;
	bool b_show_scroll_y:1;
	bool b_has_size_corner:1;
	bool b_has_bottom_margin:1; // set true when D_MARGIN_BOTTOM is below scrolly and scroll_x is hidden and has_size_corner to enlarge scroll_y

	void recalc_sliders(scr_size size);

public:
	/**
	 * @param comp, the scrolling component
	 * @author Hj. Malthaner
	 */
	gui_scrollpane_t(gui_component_t *comp);

	/**
	 * This method MUST be used to set the size of scrollpanes.
	 * @author Hj. Malthaner
	 */
	void set_size(scr_size size) OVERRIDE;

	/**
	 * Set the position of the Scrollbars
	 * @author Hj. Malthaner
	 */
	void set_scroll_position(int x, int y);

	scr_rect get_client( void );

	int get_scroll_x() const;
	int get_scroll_y() const;

	void set_scroll_amount_x(const sint32 sa) { scroll_x.set_scroll_amount(sa); }
	void set_scroll_amount_y(const sint32 sa) { scroll_y.set_scroll_amount(sa); }

	void set_scroll_discrete_x(const bool sd) { scroll_x.set_scroll_discrete(sd); }
	void set_scroll_discrete_y(const bool sd) { scroll_y.set_scroll_discrete(sd); }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the component
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord offset);

	void set_show_scroll_x(bool yesno) { b_show_scroll_x = yesno; }

	void set_show_scroll_y(bool yesno) { b_show_scroll_y = yesno; }

	void set_scrollbar_mode(scrollbar_t::visible_mode_t mode) { scroll_x.set_visible_mode(mode); scroll_y.set_visible_mode(mode); }

	koord get_client_size();

	void set_bottom_margin(bool yesno) { b_has_bottom_margin = yesno; }

	void set_size_corner(bool yesno) { b_has_size_corner = yesno; }

	/**
	 * Returns true if the hosted component is focusable
	 * @author Knightly
	 */
	virtual bool is_focusable() { return comp->is_focusable(); }

	/**
	 * returns element that has the focus
	 */
	gui_component_t *get_focus() { return comp->get_focus(); }

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 * @author Knightly
	 */
	virtual scr_coord get_focus_pos() { return pos + ( comp->get_focus_pos() - scr_coord( scroll_x.get_knob_offset(), scroll_y.get_knob_offset() ) ); }
};

#endif
