/*
 * convoi_info_t.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "gui_frame.h"
#include "gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "button.h"
#include "gui_label.h"                  // 09-Dec-2001      Markus Weber    Added
#include "world_view_t.h"
#include "ifc/action_listener.h"
#include "../convoihandle_t.h"

#include "../utils/cbuffer_t.h"

class gui_chart_t;

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
    world_view_t view;
    gui_label_t sort_label;
    gui_textinput_t input;
    gui_speedbar_t filled_bar;
    gui_speedbar_t speed_bar;
    gui_speedbar_t route_bar;
    gui_chart_t *chart;
    button_t button;
    button_t follow_button;
    button_t go_home_button;
#ifdef HAVE_KILL
    button_t kill_button;
#endif
    button_t filterButtons[7];

    button_t sort_button;
    button_t details_button;
    button_t toggler;

    bool btoggled;
    bool bFilterIsActive[7];

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

	sort_mode_t sortby;

	static const char *sort_text[SORT_MODES];
	static sort_mode_t global_sortby;
public:

    /**
     * Konstruktor.
     * @author Hj. Malthaner
     */
    convoi_info_t(convoihandle_t cnv);

    /**
     * Destruktor.
     * @author Hj. Malthaner
     */
    ~convoi_info_t();

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    virtual const char * gib_hilfe_datei() const {return "convoiinfo.txt"; }

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord pos, koord gr);



    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *komp);


    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    virtual void resize(const koord delta);
};
