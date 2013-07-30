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
		enum type_t {
			vertical,  //@< Vertical scrollbar
			horizontal //@< Horizontal scrollbar
		};

		enum visible_mode_t {
			show_never,  //@< Never show the scrollbar
			show_always, //@< Always show the scrollbar, even at maximum
			show_auto    //@< Hide scrollbar at maximum value, else show
		};

	private:

		// private button indexes
		enum button_element_index {
			left_top_arrow_index,
			right_bottom_arrow_index,
			knob_index,
			background_index
		};

		type_t         type;
		visible_mode_t visible_mode;
		bool           full;

		// the following three values are from host (e.g. list), NOT actual size.
		scr_coord_val knob_offset; // offset from top-left
		scr_coord_val knob_size;   // size of scrollbar knob
		scr_coord_val knob_area;   // size of area where knob moves in

		/**
		 * number of pixels to scroll with arrow button press. default: 11 pixels
		 */
		scr_coord_val knob_scroll_amount;
		bool          knob_scroll_discrete;  // if true, knob_offset forced to be integer multiples of knob_scroll_amount

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
				return button_def[knob_index].get_pos().y-button_t::gui_scrollbar_size.y;
			} else /* horizontal */ {
				return button_def[knob_index].get_pos().x-button_t::gui_scrollbar_size.x;
			}
		}

	public:
		// type is either scrollbar_t::horizontal or scrollbar_t::vertical
		scrollbar_t(type_t type);

		void set_groesse(koord groesse) OVERRIDE;

		void set_scroll_amount(const scr_coord_val sa) { knob_scroll_amount = sa; }
		void set_scroll_discrete(const bool sd) { knob_scroll_discrete = sd; }

		/**
		 * size is visible size, area is total size in pixels of _parent_.
		 */
		void set_knob(scr_coord_val knob_size, scr_coord_val knob_area);
		scr_coord_val get_knob_offset() const {return knob_offset;}
		void set_knob_offset(scr_coord_val v) { knob_offset = v; reposition_buttons(); }

		void set_visible_mode(visible_mode_t vm) { visible_mode = vm; }
		visible_mode_t get_visible_mode(void) { return visible_mode; }

		bool infowin_event(event_t const*) OVERRIDE;

		void zeichnen(koord pos);
};

#endif
