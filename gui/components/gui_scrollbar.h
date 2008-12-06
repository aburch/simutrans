#ifndef gui_scrollbar_h
#define gui_scrollbar_h

#include "../../ifc/gui_action_creator.h"
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

private:
	enum type type;

	// the following three values are from host (e.g. list), NOT actual size.
	sint32 knob_offset; // offset from top-left
	sint32 knob_size; // size of scrollbar knob
	sint32 knob_area; // size of area where knob moves in

	/**
	 * number of pixels to scroll with arrow button press. default: 11 pixels
	 */
	sint32 knob_scroll_amount;
	button_t button_def[3];

	void reposition_buttons();
	void button_press(sint32 number); // arrow button
	void space_press(sint32 updown); // space in slidebar hit
	sint32 slider_drag(sint32 amount); // drags slider. returns dragged amount.

	/**
	 * real position of knob in pixels, counting from zero
	 */
	int real_knob_position() {
		if (type == vertical) {
			return button_def[2].gib_pos().y-12;
		} else /* horizontal */ {
			return button_def[2].gib_pos().x-12;
		}
	}

public:
	// type is either scrollbar_t::horizontal or scrollbar_t::vertical
	scrollbar_t(enum type type);

	/**
	 * Vorzugsweise sollte diese Methode zum Setzen der Größe benutzt werden,
	 * obwohl groesse public ist.
	 * @author Hj. Malthaner
	 */
	void setze_groesse(koord groesse);

	void setze_scroll_amount(sint32 sa) { knob_scroll_amount = sa; }

	/**
	 * size is visible size, area is total size in pixels of _parent_.
	 */
	void setze_knob(sint32 knob_size, sint32 knob_area);

	sint32 gib_knob_offset() const {return knob_offset;}

	void setze_knob_offset(sint32 v) { knob_offset = v; reposition_buttons(); }

	void infowin_event(const event_t *ev);

	void zeichnen(koord pos);
};

#endif
