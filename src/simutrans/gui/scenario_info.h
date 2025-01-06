/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCENARIO_INFO_H
#define GUI_SCENARIO_INFO_H


#include "gui_frame.h"
#include "simwin.h"
#include "components/gui_flowtext.h"
#include "components/gui_tab_panel.h"

class dynamic_string;
/**
 * All messages since the start of the program
 */
class scenario_info_t : public gui_frame_t, private action_listener_t
{
private:
	gui_tab_panel_t tabs;

	gui_flowtext_t info, goal, rule, result, about, error, debug_msg;

	uint32 hash_text;

	bool update_dynamic_texts(gui_flowtext_t *flow, dynamic_string *text, scr_size size, bool init);

	uint16 get_tab_index(const char* which);
public:
	scenario_info_t();

	/**
	 * This method is called if an action is triggered
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 */
	bool action_triggered( gui_action_creator_t *comp, value_t extra) OVERRIDE;

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void update_scenario_texts();

	void open_tab(const char* which);

	int get_open_tab() { return tabs.get_active_tab_index(); }

	uint32 get_rdwr_id() OVERRIDE { return magic_scenario_info; }
	void rdwr( loadsave_t *file ) OVERRIDE;
};

#endif
