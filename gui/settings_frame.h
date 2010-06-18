/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef settings_frame_h
#define settings_frame_h


#include "gui_frame.h"
#include "components/gui_tab_panel.h"
#include "components/gui_button.h"
#include "components/gui_scrollpane.h"

#include "settings_stats.h"
#include "components/action_listener.h"

class einstellungen_t;


/**
 * All messages since the start of the program
 * @author prissi
 */
class settings_frame_t : public gui_frame_t, action_listener_t
{
private:
	einstellungen_t	*sets;
	gui_tab_panel_t	tabs;

	settings_general_stats_t general;
	gui_scrollpane_t scrolly_general;
	settings_economy_stats_t economy;
	gui_scrollpane_t scrolly_economy;
	settings_routing_stats_t routing;
	gui_scrollpane_t scrolly_routing;
	settings_costs_stats_t   costs;
	gui_scrollpane_t scrolly_costs;

	gui_tab_panel_t	 tabs_experimental;
	settings_experimental_general_stats_t exp_general;
	gui_scrollpane_t scrolly_exp_general;
	settings_experimental_revenue_stats_t exp_revenue;
	gui_scrollpane_t scrolly_exp_revenue;

	button_t revert_to_default, revert_to_last_save;

public:
	settings_frame_t(einstellungen_t *sets);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "settings.txt";}

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

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author V. Meyer
	 */
	bool infowin_event(const event_t *ev);
};

#endif
