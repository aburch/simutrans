/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CHAT_FRAME_H
#define GUI_CHAT_FRAME_H


#include "simwin.h"

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_flowtext.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_textinput.h"

#include "components/action_listener.h"

#include "../player/simplay.h"
#include "../simmesg.h"

#define MAX_CHAT_TABS (3)


/**
 * Chat window
 */
class chat_frame_t : public gui_frame_t, private action_listener_t
{
private:
	char ibuf[256];
	gui_aligned_container_t cont_tab_whisper;

	gui_scrolled_list_t	cont_chat_log[3];
	gui_tab_panel_t tabs;
	gui_textinput_t input;
	gui_label_buf_t lb_now_online, lb_whisper_target;
	gui_label_t lb_channel;
	button_t opaque_bt, bt_send_pos;
	gui_combobox_t cb_direct_chat_targets;

	uint32 last_count = 0; // of messages in list
	sint8 old_player_nr = 0;

public:
	chat_frame_t();

	// update the content
	void fill_list();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char* get_help_filename() const OVERRIDE { return "chat.txt"; }

	void activate_whisper_to(const char* recipient);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void set_dirty() { resize(scr_size(0, 0)); }

	void rdwr(loadsave_t*) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_chatframe; }

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	// theoretically, map roatation should not happen with netwrok games ...
	void map_rotate90(sint16 /*new_ysize*/) OVERRIDE { fill_list(); }
};

#endif
