/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_stadt_info_h
#define gui_stadt_info_h

#include "../simcity.h"

#include "gui_frame.h"
#include "components/gui_chart.h"
#include "components/gui_textinput.h"
#include "components/action_listener.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_tab_panel.h"

class stadt_t;

/**
 * Dies stellt ein Fenster mit den Informationen
 * ueber eine Stadt dar.
 *
 * @author Hj. Malthaner
 */
class stadt_info_t : public gui_frame_t, private action_listener_t
{
private:
	char name[256];

	stadt_t *stadt;

	gui_textinput_t name_input;

	gui_tab_panel_t year_month_tabs;

	gui_chart_t chart, mchart;

	button_t filterButtons[MAX_CITY_HISTORY];
	bool bFilterIsActive[MAX_CITY_HISTORY];

public:

  stadt_info_t(stadt_t *stadt);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    const char * gib_hilfe_datei() const {return "citywindow.txt";}

  /**
   * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
   * das Fenster, d.h. es sind die Bildschirmkoordinaten des Fensters
   * in dem die Komponente dargestellt wird.
   */
  void zeichnen(koord pos, koord gr);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
