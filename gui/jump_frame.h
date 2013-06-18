/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Maintains the list of custon labels assigned to different palces on the map
 */

#ifndef gui_jumpframe_h
#define gui_jumpframe_h

#include "components/action_listener.h"
#include "gui_frame.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"
#include "components/gui_button.h"


class karte_t;

class jump_frame_t : public gui_frame_t, action_listener_t
{
	char buf[64];
	gui_textinput_t input;
	gui_divider_t divider1;
	button_t jumpbutton;
	karte_t *welt;

public:
	jump_frame_t(karte_t *welt);

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char * get_hilfe_datei() const { return "jump_frame.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
