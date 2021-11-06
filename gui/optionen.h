/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_OPTIONEN_H
#define GUI_OPTIONEN_H


#include "simwin.h"
#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"


/*
 * Settings in the game
 *
 * Dialog for game options/Main menu
 */
class optionen_gui_t : public gui_frame_t, action_listener_t
{
	private:
		button_t option_buttons[12];

	public:
		optionen_gui_t();

		/**
		 * Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 */
		const char * get_help_filename() const OVERRIDE {return "options.txt";}

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

		void rdwr(loadsave_t *) OVERRIDE {}

		uint32 get_rdwr_id() OVERRIDE { return magic_optionen_gui_t; }
};

#endif
