/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCHEDULE_LIST_H
#define GUI_SCHEDULE_LIST_H


#include "gui_frame.h"
#include "components/gui_combobox.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_convoiinfo.h"
#include "components/gui_waytype_tab_panel.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/minivec_tpl.h"
#include "../simline.h"
#include "../descriptor/goods_desc.h"

class player_t;

/**
 * Window displaying information about all schedules and lines.
 */
class schedule_list_gui_t : public gui_frame_t, public action_listener_t
{
private:
	player_t *player;

	button_t bt_new_line, bt_delete_line, bt_single_line;
	gui_scrolled_list_t scl;
	gui_textinput_t inp_filter;
	button_t filterButtons[MAX_LINE_COST];
	gui_waytype_tab_panel_t tabs;

	gui_combobox_t freight_type_c, sort_type_c;
	button_t sorteddir;

	uint32 old_line_count;

	// to filter for line names
	char schedule_filter[256], old_schedule_filter[256];

	vector_tpl<const goods_desc_t *> viewable_freight_types;
	bool is_matching_freight_catg( const minivec_tpl<uint8> &goods_catg_index );

	uint8 current_sort_mode;
	void build_line_list(simline_t::linetype filter);

	// last active line
	linehandle_t line;

public:
	/// last selected line per tab
	static linehandle_t selected_line[MAX_PLAYER_COUNT][simline_t::MAX_LINE_TYPE];

	schedule_list_gui_t(player_t* player_);

	const char* get_help_filename() const OVERRIDE { return "linemanagement.txt"; }

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// following: rdwr stuff
	void rdwr( loadsave_t *file ) OVERRIDE;
	uint32 get_rdwr_id() OVERRIDE;
};

#endif
