/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_BUTTON_H
#define GUI_COMPONENTS_GUI_BUTTON_H


#include "gui_action_creator.h"
#include "gui_component.h"
#include "../../simcolor.h"
#include "../../dataobj/koord.h"
#include "../../dataobj/koord3d.h"
#include "../../display/simimg.h"

class karte_ptr_t;

/**
 * Class for buttons in Windows
 * Defines all button types: Normal (roundbox), Checkboxes (square), Arrows, Scrollbars
 */
class button_t :
	public gui_action_creator_t,
	public gui_component_t
{

public:
	/* the button with the postfix state do not automatically change their state like the normal button do
	 * the _state buttons must be changed by the caller!
	 * _automatic buttons do everything themselves, i.e. depress/release alternately
	 *
	 * square:        a checkbox with text on the right side next to it
	 * box:           button which is used for many selection purposes, can have colored background
	 * roundbox:      button for "load", "cancel" and such options
	 * arrow-buttons: buttons with arrows, cannot have text
	 * repeat arrows: calls the caller until the mouse is released
	 * flexible:      flag, can be set to box, square to get infinitely enlarging buttons
	 */
	enum type {
		square = 1,
		box,
		roundbox,
		imagebox,
		sortarrow,
		arrowleft,
		arrowright,
		arrowup,
		arrowdown,
		repeatarrowleft,
		repeatarrowright,
		posbutton,
		TYPE_MASK = (1 << 7) - 1,

		state            = 1 << 7,
		square_state     = square     | state,
		box_state        = box        | state,
		roundbox_state   = roundbox   | state,
		imagebox_state   = imagebox   | state,
		sortarrow_state  = sortarrow  | state,
		arrowright_state = arrowright | state,

		automatic           = 1 << 8,
		square_automatic    = square_state | automatic,
		box_state_automatic = box_state    | automatic,
		imagebox_automatic  = imagebox     | automatic,
		sortarrow_automatic = sortarrow    | automatic,
		posbutton_automatic = posbutton    | automatic,

		flexible = 1 << 9
	};

protected:
	/**
	 * Hide the base class init() version to force use of
	 * the extended init() version for buttons.
	 */
	using gui_component_t::init;

private:
	/**
	 * Tooltip for this button
	 */
	const char * tooltip, *translated_tooltip;

	enum type type;

	/**
	 * if buttons is disabled show only grey label
	 */
	uint8 b_enabled:1;
	uint8 b_no_translate:1;

	/**
	 * The displayed text of the button
	 * direct access provided to avoid translations
	 */
	const char *text;
	const char *translated_text;

	union {
		koord3d targetpos;
		image_id img;
		uint32 button_click_time;
	};

	// any click will go to this world
	static karte_ptr_t welt;

	void draw_focus_rect( scr_rect, scr_coord_val offset = 1);

	// Hide these
	button_t(const button_t&);        // forbidden
	void operator =(const button_t&); // forbidden

public:
	PIXVAL background_color;
	PIXVAL text_color;

	bool pressed;

	button_t();

	/**
	 * Initializes the button. Sets the size depending on type.
	 */
	void init(enum type typ, const char *text, scr_coord pos=scr_coord(0,0), scr_size size = scr_size::invalid);

	void set_typ(enum type typ);
	enum type get_type() const { return this->type; }

	const char * get_text() const {return text;}

	/**
	 * Set the displayed text of the button
	 */
	void set_text(const char * text);

	/**
	 * Set position for posbuttons, will be returned on calling listener
	 */
	void set_targetpos( const koord k ); // assuming this is on map ground
	void set_targetpos3d( const koord3d k ) { targetpos = k; }

	// only relevant for imagebox
	void set_image(image_id id) { img = id; }

	/**
	 * Set the displayed text of the button when not to translate
	 */
	void set_no_translate(bool b) { b_no_translate = b; }

	/**
	 * Sets the tooltip of this button
	 */
	void set_tooltip(const char * tooltip);

	/**
	 * @return true when x, y is within button area, i.e. the button was clicked
	 * @return false when x, y is outside button area
	 */
	bool getroffen(int x, int y) OVERRIDE;

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw the component
	 */
	void draw(scr_coord offset) OVERRIDE;

	/**
	 * Max-size: infinity for checkboxes, equal to size for the other types.
	 */
	scr_size get_max_size() const OVERRIDE;

	/**
	 * Min-size: equal to the size (set by init).
	 */
	scr_size get_min_size() const OVERRIDE;

	void enable(bool true_false_par = true) { b_enabled = true_false_par; }

	void disable() { enable(false); }

	bool enabled() { return b_enabled; }

	// a button can only be focusable when it is enabled
	bool is_focusable() OVERRIDE { return b_enabled && gui_component_t::is_focusable(); }

	void update_focusability();

};

ENUM_BITSET(enum button_t::type)

#endif
