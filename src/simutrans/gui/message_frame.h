/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_MESSAGE_FRAME_H
#define GUI_MESSAGE_FRAME_H


#include "simwin.h"

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_tab_panel.h"
#include "components/gui_textinput.h"

#include "components/action_listener.h"



/**
 * All messages since the start of the program
 */
class message_frame_t : public gui_frame_t, private action_listener_t
{
private:
	char ibuf[256];
	gui_scrolled_list_t scrolly;
	gui_tab_panel_t tabs;     // tab panel for filtering messages
	gui_textinput_t input;
	button_t opaque_bt, option_bt, copy_bt;
	vector_tpl<sint32> tab_categories;

	uint32 last_count; // of messages in list
	sint32 message_type;  // message type for filtering; -1 indicates no filtering
	void fill_list();
	void filter_list(sint32 type);

public:
	message_frame_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE {return "mailbox.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr(loadsave_t *) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_messageframe; }

	void draw(scr_coord pos, scr_size size) OVERRIDE;
};

#endif
