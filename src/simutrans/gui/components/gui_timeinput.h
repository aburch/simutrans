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
#include "gui_aligned_container.h"
#include "gui_numberinput.h"


/**
 * An input field for game time (internally as ticks)
 * The actual input is 0...65535, which is minute precision and then shifted by bit_shift_per_month-16
 */
class gui_timeinput_t :
	public gui_action_creator_t,
	public gui_aligned_container_t,
	public action_listener_t
{
private:
	gui_numberinput_t days, hours, minutes;

	bool b_enabled, b_absolute;

public:
	gui_timeinput_t(const char *null_text);

	sint32 get_ticks();
	void set_ticks(uint16 t,bool absolute);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr( loadsave_t *file );

	void draw(scr_coord offset) OVERRIDE;

	void enable(bool b) { b_enabled = b;  }
};

#endif
