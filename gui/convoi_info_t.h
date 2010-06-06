/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/ding_view_t.h"
#include "components/action_listener.h"
#include "../convoihandle_t.h"
#include "../linehandle_t.h"

#include "../utils/cbuffer_t.h"


/**
 * Displays an information window for a convoi
 *
 * @author Hj. Malthaner
 * @date 22-Aug-01
 */
class convoi_info_t : public gui_frame_t, private action_listener_t
{
public:
	enum sort_mode_t { by_destination=0, by_via=1, by_amount_via=2, by_amount=3, SORT_MODES=4 };

private:
	gui_scrollpane_t scrolly;
	gui_textarea_t text;
	ding_view_t view;
	gui_label_t sort_label;
	gui_textinput_t input;
	gui_speedbar_t filled_bar;
	gui_speedbar_t speed_bar;
	gui_speedbar_t route_bar;
	gui_chart_t chart;
	button_t button;
	button_t follow_button;
	button_t go_home_button;
	button_t no_load_button;
	button_t filterButtons[7];

	button_t sort_button;
	button_t details_button;
	button_t toggler;

	button_t line_button;	// got to line ...
	bool line_bound;

	convoihandle_t cnv;
	sint32 mean_convoi_speed;
	sint32 max_convoi_speed;

	// current pointer to route ...
	sint32 cnv_route_index;

	/**
	* Buffer for freight info text string.
	* @author Hj. Malthaner
	*/
	cbuffer_t freight_info;

	static bool route_search_in_progress;
	static const char *sort_text[SORT_MODES];

public:
	convoi_info_t(convoihandle_t cnv);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author V. Meyer
	 */
	const char * get_hilfe_datei() const { return "convoiinfo.txt"; }

	/**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);

	/**
	 * resize window in response to a resize event
	 * @author Hj. Malthaner
	 */
	void resize(const koord delta);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);
};
