/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_KENNFARBE_H
#define GUI_KENNFARBE_H


#include "../utils/cbuffer.h"
#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_textarea.h"
#include "components/gui_button.h"

class choose_color_button_t;

/**
 * Company colors window
 * Dialog to set the player's color
 */
class farbengui_t : public gui_frame_t, action_listener_t
{
	private:
		player_t *player;
		cbuffer_t buf;
		gui_textarea_t txt;

		choose_color_button_t* player_color_1[28];
		choose_color_button_t* player_color_2[28];

	public:
		farbengui_t(player_t *player_);

		/**
		 * Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 */
		const char * get_help_filename() const OVERRIDE { return "color.txt"; }

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
