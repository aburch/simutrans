/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_CURIOSITYLIST_FRAME_H
#define GUI_CURIOSITYLIST_FRAME_H


#include "simwin.h"
#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"


/**
 * Curiosity list window
 */
class curiositylist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	gui_combobox_t sortedby;
	button_t sorteddir;
	gui_scrolled_list_t scrolly;
	gui_aligned_container_t list;
	button_t filter_by_owner;
	gui_combobox_t filterowner;

	uint32 attraction_count;

	void fill_list();

	static char name_filter[256];
	char last_name_filter[256];
	gui_textinput_t name_filter_input;

public:
	curiositylist_frame_t();

	const char *get_help_filename() const OVERRIDE {return "curiositylist_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE { fill_list(); }

	void rdwr(loadsave_t* file) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_curiositylist; }
};

#endif
