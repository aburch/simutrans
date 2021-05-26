/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LABELLIST_FRAME_T_H
#define GUI_LABELLIST_FRAME_T_H


#include "simwin.h"
#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_textinput.h"


/**
 * label list window
 */
class labellist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	gui_combobox_t sortedby;
	button_t sorteddir;
	button_t filter;

	gui_scrolled_list_t scrolly;

	static char name_filter[256];
	char last_name_filter[256];
	gui_textinput_t name_filter_input;

	uint32 label_count;

public:
	labellist_frame_t();

	const char *get_help_filename() const OVERRIDE {return "labellist_filter.txt"; }

	bool action_triggered( gui_action_creator_t *comp,value_t /* */) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	uint32 count_label();

	void fill_list();

	void map_rotate90( sint16 ) OVERRIDE { fill_list(); }

	void rdwr(loadsave_t* file) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_labellist; }
};

#endif
