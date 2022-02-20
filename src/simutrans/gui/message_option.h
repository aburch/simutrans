/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_MESSAGE_OPTION_H
#define GUI_MESSAGE_OPTION_H


#include "../simmesg.h"
#include "simwin.h"

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "../utils/cbuffer.h"


#define MAX_MESSAGE_OPTION_TEXTLEN (64)

class message_option_t : public gui_frame_t, private action_listener_t
{
private:
	button_t buttons[4*message_t::MAX_MESSAGE_TYPE];
	gui_label_t text_lbl[message_t::MAX_MESSAGE_TYPE];
	sint32 ticker_msg, window_msg, auto_msg, ignore_msg;
	char option_texts[message_t::MAX_MESSAGE_TYPE][MAX_MESSAGE_OPTION_TEXTLEN];


public:
	message_option_t();

	const char * get_help_filename() const OVERRIDE {return "mailbox.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_message_options; }
};

#endif
