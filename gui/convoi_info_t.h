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

#ifndef cbuffer_t_h
#include "../utils/cbuffer_t.h"
#endif

class gui_chart_t;

/**
 * Displays an information window for a convoi
 *
 * @author Hj. Malthaner
 * @date 22-Aug-01
 */
class convoi_info_t : public gui_frame_t, private action_listener_t
{
private:
    gui_scrollpane_t scrolly;
    gui_textarea_t text;
    gui_textinput_t input;
    gui_speedbar_t filled_bar;
    gui_speedbar_t speed_bar;
    gui_chart_t *chart;
    world_view_t view;
    button_t button;
    button_t go_home_button;
    button_t kill_button;
    button_t filterButtons[7];
    button_t toggler;
    bool btoggled;
    bool bFilterIsActive[7];

    convoihandle_t cnv;

    /**
     * Buffer for freight info text string.
     * @author Hj. Malthaner
     */
    cbuffer_t freight_info;

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
