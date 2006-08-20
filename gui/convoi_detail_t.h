/*
 * convoi_detail_t.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "gui_frame.h"
#include "gui_container.h"
#include "gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "button.h"
#include "gui_label.h"                  // 09-Dec-2001      Markus Weber    Added
#include "world_view_t.h"
#include "ifc/action_listener.h"
#include "../convoihandle_t.h"

#include "../dataobj/koord.h"

#include "../utils/cbuffer_t.h"

class gui_chart_t;

/**
 * One element of the vehicle list display
 * @autor prissi
 */
class gui_vehicleinfo_t : public gui_container_t
{
private:
    /**
     * Handle des anzuzeigenden Convois.
     * @author Hj. Malthaner
     */
    convoihandle_t cnv;

public:
    /**
     * Konstruktor.
     * @param cnv das Handle für den anzuzeigenden Convoi.
     * @author Hj. Malthaner
     */
    gui_vehicleinfo_t(convoihandle_t cnv);

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *);

    /**
     * Zeichnet die Komponente
     * @author Hj. Malthaner
     */
    void zeichnen(koord offset) const;
};




/**
 * Displays an information window for a convoi
 *
 * @author Hj. Malthaner
 * @date 22-Aug-01
 */
class convoi_detail_t : public gui_frame_t
{
public:
	enum sort_mode_t { by_destination=0, by_via=1, by_amount_via=2, by_amount=3, SORT_MODES=4 };

private:
	gui_scrollpane_t scrolly;
	gui_vehicleinfo_t veh_info;

	convoihandle_t cnv;
public:

    /**
     * Konstruktor.
     * @author Hj. Malthaner
     */
    convoi_detail_t(convoihandle_t cnv);


    /**
     * Destruktor.
     * @author Hj. Malthaner
     */
    ~convoi_detail_t();


    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord pos, koord gr);



    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    virtual void resize(const koord delta);
};
