/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_SCROLLPANE_H
#define GUI_COMPONENTS_GUI_SCROLLPANE_H


#include "gui_component.h"
#include "gui_scrollbar.h"

class loadsave_t;

/*
 * this is a scrolling area in which subcomponents are drawn
 */
class gui_scrollpane_t : public gui_component_t
{
private:
	scr_size old_comp_size;

	/**
	 * Scrollbar X/Y
	 */
	scrollbar_t scroll_x, scroll_y;

	bool b_show_scroll_x:1;
	bool b_show_scroll_y:1;
	bool b_has_size_corner:1;
	bool maximize:1;

	// for oversized entries
	scr_coord_val max_width;

protected:
	/**
	 * The scrolling component
	 */
	gui_component_t *comp;

	void recalc_sliders(scr_size size);

public:
	/**
	 * @param comp the scrolling component
	 */
	gui_scrollpane_t(gui_component_t *comp, bool b_scroll_x = false, bool b_scroll_y = true);

	void set_component(gui_component_t *comp) { this->comp = comp; }

	/**
	* this is the maximum width a scrollbar requests as minimum size
	* default is the stadard width of a dialoge (4*button width+3*space)
	* @param width the minimum width it should strech to
	*/
	virtual void set_min_width( scr_coord_val width ) { max_width = width; }

	/**
	 * This method MUST be used to set the size of scrollpanes.
	 */
	void set_size(scr_size size) OVERRIDE;

	/**
	 * Request other pane-size.
	 * @returns realized size.
	 */
	scr_size request_size(scr_size request);

	/**
	 * Set the position of the Scrollbars
	 */
	void set_scroll_position(int x, int y);

	scr_rect get_client( void ) OVERRIDE;

	int get_scroll_x() const;
	int get_scroll_y() const;

	void set_scroll_amount_x(const sint32 sa) { scroll_x.set_scroll_amount(sa); }
	void set_scroll_amount_y(const sint32 sa) { scroll_y.set_scroll_amount(sa); }

	void set_scroll_discrete_x(const bool sd) { scroll_x.set_scroll_discrete(sd); }
	void set_scroll_discrete_y(const bool sd) { scroll_y.set_scroll_discrete(sd); }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset) OVERRIDE;

	void set_show_scroll_x(bool yesno) { b_show_scroll_x = yesno; }

	void set_show_scroll_y(bool yesno) { b_show_scroll_y = yesno; }

	void set_scrollbar_mode(scrollbar_t::visible_mode_t mode) { scroll_x.set_visible_mode(mode); scroll_y.set_visible_mode(mode); }

	void set_size_corner(bool yesno) { b_has_size_corner = yesno; }

	/**
	 * Returns true if the hosted component is focusable
	 */
	bool is_focusable() OVERRIDE { return comp->is_focusable(); }

	/**
	 * returns element that has the focus
	 */
	gui_component_t *get_focus() OVERRIDE { return comp->get_focus(); }

	/**
	 * Adjust scrollbars to make focused element visible if necessary
	 */
	void show_focused();

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 */
	scr_coord get_focus_pos() OVERRIDE { return pos + ( comp->get_focus_pos() - scr_coord( scroll_x.get_knob_offset(), scroll_y.get_knob_offset() ) ); }

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	/// load/save scrollbar positions
	void rdwr( loadsave_t *file );

	bool is_marginless() const OVERRIDE { return maximize; }
	void set_maximize(bool b) { maximize = b; }
};

#endif
