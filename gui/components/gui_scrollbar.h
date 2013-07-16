/* This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_scrollbar_h
#define gui_scrollbar_h

#include "gui_action_creator.h"
#include "../../simevent.h"
#include "gui_button.h"


/**
 * Scrollbar class
 * scrollbar can be horizontal or vertical
 *
 * @author Niels Roest, additions by Hj. Malthaner
 */
class scrollbar_t :
	public gui_action_creator_t,
	public gui_komponente_t
{
public:
	enum type { vertical, horizontal };

	// width/height of bar part
	//static sint16 BAR_SIZE;

private:
	enum type type;

	// private button indexes
	enum button_element_index {
		left_top_arrow_index,
		right_bottom_arrow_index,
		knob_index,
		background_index
	};

	// the following three values are from host (e.g. list), NOT actual size.
	sint32 knob_offset; // offset from top-left
	sint32 knob_size;   // size of scrollbar knob
	sint32 knob_area;   // size of area where knob moves in

	/**
	 * number of pixels to scroll with arrow button press. default: 11 pixels
	 */
	sint32 knob_scroll_amount;
	bool knob_scroll_discrete;  // if true, knob_offset forced to be integer multiples of knob_scroll_amount

	button_t button_def[4];

	void reposition_buttons();
	void button_press(sint32 number); // arrow button
	void space_press(sint32 updown); // space in slidebar hit
	sint32 slider_drag(sint32 amount); // drags slider. returns dragged amount.

	/**
	 * real position of knob in pixels, counting from zero
	 */
	int real_knob_position() {
		if (type == vertical) {
			return button_def[knob_index].get_pos().y-12;
		} else /* horizontal */ {
			return button_def[knob_index].get_pos().x-12;
		}
	}

public:
	// type is either scrollbar_t::horizontal or scrollbar_t::vertical
	scrollbar_t(enum type type);

	void set_groesse(koord groesse) OVERRIDE;

	void set_scroll_amount(const sint32 sa) { knob_scroll_amount = sa; }
	void set_scroll_discrete(const bool sd) { knob_scroll_discrete = sd; }

	/**
	 * size is visible size, area is total size in pixels of _parent_.
	 */
	void set_knob(sint32 knob_size, sint32 knob_area);

	sint32 get_knob_offset() const {return knob_offset;}

	void set_knob_offset(sint32 v) { knob_offset = v; reposition_buttons(); }

	bool infowin_event(event_t const*) OVERRIDE;

	void zeichnen(koord pos);
};

#endif
