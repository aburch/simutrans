/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_TIMEINPUT_H
#define GUI_COMPONENTS_GUI_TIMEINPUT_H


#include "../../simtypes.h"
#include "../../display/scr_coord.h"
#include "gui_action_creator.h"
#include "gui_label.h"
#include "gui_button.h"
#include "gui_component.h"
#include "gui_scrolled_list.h"


/**
 * An input field for game time (internally as ticks)
 * The actual input is 0...65535, which is minute precision and then shifted by bit_shift_per_month-16
 */
class gui_timeinput_t :
	public gui_action_creator_t,
	public gui_scrolled_list_t::scrollitem_t
{
private:
	// the input field
	gui_label_t time_out;

	// arrow buttons for increasing / decr.
	button_t bt_left, bt_right;

	uint16 ticks;

	char textbuffer[64];

	sint32 repeat_steps;
	static scr_coord_val text_width;

	bool b_enabled;

public:
	gui_timeinput_t();

	void set_size(scr_size size) OVERRIDE;

	sint32 get_ticks() const { return ticks; }
	void set_ticks(uint16);

	bool infowin_event(event_t const*) OVERRIDE;

	void enable() { b_enabled = true; set_focusable(true); bt_left.enable(); bt_right.enable(); }
	void disable() { b_enabled = false; set_focusable(false); bt_left.disable(); bt_right.disable(); }
	bool enabled() const { return b_enabled; }
	bool is_focusable() OVERRIDE { return b_enabled && gui_component_t::is_focusable(); }
	void enable( bool yesno ) {
		if( yesno && !gui_component_t::is_focusable() ) {
			enable();
		}
		else if( !yesno  &&  gui_component_t::is_focusable() ) {
			disable();
		}
	}

	void draw(scr_coord offset) OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	char const *get_text() const OVERRIDE { return textbuffer; }

	void rdwr( loadsave_t *file );

	static bool compare( const gui_component_t *a, const gui_component_t *b );
};

#endif
