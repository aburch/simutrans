/*
 * schedule_list.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "gui_frame.h"
#include "gui_container.h"
#include "gui_scrollpane.h"
#include "gui_label.h"
#include "components/gui_resizer.h"                             // 28-Dec-2001  Markus Weber    Added
#include "ifc/action_listener.h"                                // 28-Dec-2001  Markus Weber    Added
#include "button.h"

class spieler_t;
class vehikel_t;
class ware_besch_t;

/**
 * Displays a scrollable list of all lines
 * @author hsiegeln
 */
class line_frame_t : public gui_frame_t , private action_listener_t
{
private:
    spieler_t *owner;

    karte_t	*welt;

    /*
     * All gui elements of this dialog:
     */
    gui_container_t cont;
    gui_scrollpane_t scrolly;
    gui_resizer_t vresize;

public:
    /**
     * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
     * @author Hj. Malthaner
     */
    line_frame_t(spieler_t *sp, karte_t *welt);

    /**
     * Destruktor.
     * @author Hj. Malthaner
     */
    ~line_frame_t();

    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author V. Meyer
     */
    virtual void infowin_event(const event_t *ev);

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    virtual bool action_triggered(gui_komponente_t *);

    /**
     * This method is called if the size of the window should be changed
     * @author Markus Weber
     */
    void resize(koord size_change);                       // 28-Dec-01        Markus Weber Added

    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord pos, koord gr);

    /**
     * Displays the current list, checking sorting and filter settings.
     * @author V. Meyer
     */
    void display_list(void);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author V. Meyer
     */
    virtual const char * gib_hilfe_datei() const {return "convoi.txt"; }

};
