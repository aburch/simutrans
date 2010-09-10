/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_send_message_frame_h
#define gui_send_message_frame_h


#include "components/action_listener.h"
#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_textinput.h"
#include "components/gui_label.h"


class send_message_frame_t : public gui_frame_t, action_listener_t
{
private:
	char ibuf[256];

protected:
	spieler_t *sp;

	gui_textinput_t input;

public:
	send_message_frame_t( spieler_t *sp );

	virtual bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
