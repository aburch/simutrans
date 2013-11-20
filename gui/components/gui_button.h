/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Defines all button types: Normal (roundbox), Checkboxes (square), Arrows, Scrollbars
 */

#ifndef gui_button_h
#define gui_button_h

#include "gui_action_creator.h"
#include "gui_komponente.h"
#include "../../simcolor.h"
#include "../../display/simimg.h"


/**
 * Class for buttons in Windows
 *
 * @author Hj. Malthaner, Niels Roest
 * @date December 2000
 */
class button_t :
	public gui_action_creator_t,
	public gui_komponente_t
{

public:
	/* the button with the postfix state do not automatically change their state like the normal button do
	 * the _state buttons must be changed by the caller!
	 * _automatic buttons do everything themselves, i.e. depress/release alternately
	 *
	 * square:        button with text on the right side next to it
	 * box:           button with is used for many selection purposes; can have colored background
	 * roundbox:      button for "load" cancel and such options
	 * arrow-buttons: buttons with arrows, cannot have text
	 * repeat arrows: calls the caller until the mouse is released
	 * scrollbar:     well you guess it. Not used by gui_frame_t things ...
	 */
	enum type {
		square=1, box, roundbox, arrowleft, arrowright, arrowup, arrowdown, repeatarrowleft, repeatarrowright, posbutton,
		square_state=129, box_state, roundbox_state, arrowleft_state, arrowright_state, arrowup_state, arrowdown_state, scrollbar_horizontal_state, scrollbar_vertical_state, repeatarrowleft_state, repeatarrowright_state,
		square_automatic=257
	};

protected:
	/**
	 * Hide the base class init() version to force use of
	 * the extended init() version for buttons.
	 * @author Max Kielland
	 */
	using gui_komponente_t::init;

private:
	/**
	 * Tooltip for this button
	 * @author Hj. Malthaner
	 */
	const char * tooltip, *translated_tooltip;

	enum type type;

	/**
	 * if buttons is disabled show only grey label
	 * @author hsiegeln
	 */
	uint8 b_enabled:1;
	uint8 b_no_translate:1;

	/**
	 * The displayed text of the button
	 * direct access provided to avoid translations
	 * @author Hj. Malthaner
	 */
	union {
		const char * text;
		struct { sint16 x,y; } targetpos;
	};
	const char *translated_text;

	void draw_focus_rect( scr_rect, scr_coord_val offset = 1);

	// Hide these
	button_t(const button_t&);        // forbidden
	void operator =(const button_t&); // forbidden

public:
	COLOR_VAL background_color; //@author hsiegeln
	COLOR_VAL text_color;

	bool pressed;
	scr_coord_val text_offset_x;

	button_t();

	void init(enum type typ, const char *text, scr_coord pos=scr_coord(0,0), scr_size size = scr_size::invalid);

	void set_typ(enum type typ);
	enum type get_type() const { return this->type; }

	const char * get_text() const {return text;}

	/**
	 * Set the displayed text of the button
	 * @author Hj. Malthaner
	 */
	void set_text(const char * text);

	/**
	 * Get/Set text to position
	 * @author prissi
	 */
	void set_targetpos(const koord k ) { targetpos.x = k.x; targetpos.y = k.y; }

	/**
	 * Set the displayed text of the button when not to translate
	 * @author Hj. Malthaner
	 */
	void set_no_translate(bool b) { b_no_translate = b; }

	/**
	 * Sets the tooltip of this button
	 * @author Hj. Malthaner
	 */
	void set_tooltip(const char * tooltip);

	/**
	 * @return true when x, y is within button area, i.e. the button was clicked
	 * @return false when x, y is outside button area
	 * @author Hj. Malthaner
	 */
	bool getroffen(int x, int y) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the component
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord offset);

	void enable(bool true_false_par = true) { b_enabled = true_false_par; }

	void disable() { enable(false); }

	bool enabled() { return b_enabled; }

	// Knightly : a button can only be focusable when it is enabled
	virtual bool is_focusable() { return b_enabled && gui_komponente_t::is_focusable(); }

	void update_focusability();

};
#endif
