/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_FACTORYLIST_FRAME_T_H
#define GUI_FACTORYLIST_FRAME_T_H


#include "simwin.h"
#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "factorylist_stats_t.h"


/*
 * Factory list window
 */
class factorylist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static const char *sort_text[factorylist::SORT_MODES];

	gui_combobox_t sortedby;
	button_t sorteddir;
	button_t filter_by_owner;
	gui_combobox_t filterowner;

	gui_scrolled_list_t scrolly;

	static char name_filter[256];
	char last_name_filter[256];
	gui_textinput_t name_filter_input;

	void fill_list();

	uint32 old_factories_count;

public:
	factorylist_frame_t();

	const char *get_help_filename() const OVERRIDE {return "factorylist_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE { fill_list(); }

	void rdwr(loadsave_t* file) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_factorylist; }
};

#endif
