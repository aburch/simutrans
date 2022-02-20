/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_AI_OPTION_H
#define GUI_AI_OPTION_H


#include "../simmesg.h"

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"
#include "../utils/cbuffer.h"

class player_t;
class ai_t;

/**
 * AI behavior options from AI finance window
 */
class ai_option_t : public gui_frame_t, private action_listener_t
{
private:
	button_t buttons[4];
	gui_numberinput_t construction_speed;
	ai_t *ai;

public:
	ai_option_t(player_t *player);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE {return "players.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
