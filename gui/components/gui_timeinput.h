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
#include "gui_numberinput.h"


/**
 * An input field for game time (internally as ticks)
 * The actual input is 0...65535, which is minute precision and then shifted by bit_shift_per_month-16
 */
class gui_timeinput_t :
	public gui_numberinput_t
{
private:
	// the input field
	gui_label_t time_out;

	// entry displayed for
	const char *null_text;

	char textbuffer[64];

	sint32 repeat_steps;
	static scr_coord_val text_width;

	bool b_enabled;

public:
	gui_timeinput_t(const char *null_text);

	sint32 get_ticks() { return get_value(); }
	void set_ticks(uint16 t) { set_value(t); }

	void set_null_text( const char *t ) { null_text = t; }

	void rdwr( loadsave_t *file );

	void draw(scr_coord offset) OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	scr_size get_size() const OVERRIDE;

	void set_size(scr_size) OVERRIDE;
};

#endif
