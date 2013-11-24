/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */
#ifndef _SCEN_INFO_H_
#define _SCEN_INFO_H_


#include "gui_frame.h"
#include "components/gui_flowtext.h"
#include "components/gui_tab_panel.h"
#include "components/gui_scrollpane.h"

class dynamic_string;
/**
 * All messages since the start of the program
 * @author prissi
 */
class scenario_info_t : public gui_frame_t, private action_listener_t
{
private:
	gui_tab_panel_t	tabs;

	gui_flowtext_t info, goal, rule, result, about, error, debug_msg;

	gui_scrollpane_t scrolly_info;
	gui_scrollpane_t scrolly_goal;
	gui_scrollpane_t scrolly_rule;
	gui_scrollpane_t scrolly_result;
	gui_scrollpane_t scrolly_about;
	gui_scrollpane_t scrolly_debug;
	gui_scrollpane_t scrolly_error;


	void update_dynamic_texts(gui_flowtext_t &flow, dynamic_string &text, scr_size size, bool init);


public:
	scenario_info_t();

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	/**
	 * resize window in response to a resize event
	 * @author Hj. Malthaner
	 */
	void resize(const scr_coord delta);

	void draw(scr_coord pos, scr_size size);

	void update_scenario_texts(bool init);

	void open_result_tab();
};

#endif
