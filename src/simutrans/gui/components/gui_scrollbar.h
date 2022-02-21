/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_SCROLLBAR_H
#define GUI_COMPONENTS_GUI_SCROLLBAR_H


#include "gui_action_creator.h"
#include "../../simevent.h"
#include "gui_button.h"
#include "../gui_theme.h"


/**
 * Scrollbar class
 * scrollbar can be horizontal or vertical
 */
class scrollbar_t :
	public gui_action_creator_t,
	public gui_component_t
{

public:
	enum type_t {
		vertical,  ///< Vertical scrollbar
		horizontal ///< Horizontal scrollbar
	};

	enum visible_mode_t {
		show_never,    ///< Never show the scrollbar
		show_always,   ///< Always show the scrollbar, even at maximum
		show_disabled, ///< Disable scrollbar at maximum value
		show_auto      ///< Hide scrollbar at maximum value, else show
	};

private:

	// private button indexes
	enum button_element_index {
		left_top_arrow_index,
		right_bottom_arrow_index
	};

	type_t         type;
	visible_mode_t visible_mode; // Show, hide or auto hide
	bool           full;         // Scrollbar is full
	bool dragging; // to handle event even when outside the knob ...

	// the following three values are from host (e.g. list), NOT actual size.
	sint32 knob_offset; // offset from top-left
	sint32 knob_size;
	sint32 total_size;

	/**
	 * number of elements to scroll with arrow button press. default: 11
	 */
	sint32 knob_scroll_amount;
	bool   knob_scroll_discrete;  // if true, knob_offset forced to be integer multiples of knob_scroll_amount

	// arrow buttons
	button_t button_def[2];
	// size of button area
	scr_rect knobarea, sliderarea;

	void reposition_buttons();
	void scroll( sint32 amout_to_scroll );

public:
	// type is either scrollbar_t::horizontal or scrollbar_t::vertical
	scrollbar_t(type_t type);

	void set_size(scr_size size) OVERRIDE;

	void set_scroll_amount(sint32 sa) { knob_scroll_amount = sa; }
	void set_scroll_discrete(const bool sd) { knob_scroll_discrete = sd; }

	/**
		* knob_size is visible size, total_size is total size (of whatever unit)
		* Scroolbars are not directly related to pixels!
		*/
	void set_knob(sint32 knob_size, sint32 total_size);

	sint32 get_knob_offset() const {
		// return clamped offset if really desired
		return knob_offset - (knob_scroll_discrete  &&  total_size!=knob_offset+knob_size  ?  (knob_offset % knob_scroll_amount) : 0);
	}
	void set_knob_offset(sint32 v) { knob_offset = v; reposition_buttons(); }

	void set_visible_mode(visible_mode_t vm) { visible_mode = vm; }
	visible_mode_t get_visible_mode() { return visible_mode; }

	bool infowin_event(event_t const*) OVERRIDE;

	void draw(scr_coord pos) OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;
};

#endif
