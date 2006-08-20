/*
 * gui_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef gui_gui_frame_h
#define gui_gui_frame_h

#ifndef ifc_gui_fenster_h
#include "../ifc/gui_fenster.h"
#endif

#ifndef gui_container_h
#include "gui_container.h"
#endif


/**
 * Eine Klasse für Fenster mit Komponenten.
 * Anders als die anderen Fensterklasen in Simutrans ist dies
 * ein richtig Komponentenorientiertes Fenster, das alle
 * aktionen an die Komponenten delegiert.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.10 $
 */
class gui_frame_t : virtual public gui_fenster_t
{
private:
    gui_container_t container;

    const char * name;
    koord groesse;


    /**
     * Min. size of the window
     * @author Markus Weber
     * @date   11-May-2002
     */
    koord min_windowsize;


    enum resize_modes resize_mode ;      //25-may-02	markus weber added

    fensterfarben farben;

    bool opaque;


protected:

    /**
     * resize window in response to a resize event
     * @author Markus Weber, Hj. Malthaner
     * @date   11-May-2002
     */
    virtual void resize(const koord delta);

public:

    /**
     * Konstruktor
     * @author Hj. Malthaner
     */
    gui_frame_t(const char *name);


    /**
     * Konstruktor
     * @param name Fenstertitel
     * @param color Besitzerfarbe
     * @author Hj. Malthaner
     */
    gui_frame_t(const char *name, int color);


    /**
     * Fügt eine Komponente zum Fenster hinzu.
     * @author Hj. Malthaner
     */
    void add_komponente(gui_komponente_t *komp);


    /**
     * Entfernt eine Komponente aus dem Container.
     * @author Hj. Malthaner
     */
    void remove_komponente(gui_komponente_t *komp);



    /**
     * Der Name wird in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     * @author Hj. Malthaner
     */
    virtual const char * gib_name() const;


    /**
     * setzt den Namen (Fenstertitel)
     * @author Hj. Malthaner
     */
    void setze_name(const char *name);


    /**
     * setzt die Transparenz
     * @author Hj. Malthaner
     */
    void setze_opaque(bool janein);


    /**
     * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
     * zurück
     * @author Hj. Malthaner
     */
    virtual fensterfarben gib_fensterfarben() const;


    /**
     * @return gibt wunschgroesse für das Darstellungsfenster zurueck
     * @author Hj. Malthaner
     */
    virtual koord gib_fenstergroesse() const;


    /**
     * Setzt die Fenstergroesse
     * @author Hj. Malthaner
     */
    virtual void setze_fenstergroesse(koord groesse);


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *ev);


    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord pos, koord gr);


    /**
     * Set resize mode
     * @author Markus Weber
     * @date   11-May-2002
     */
    void set_resizemode(enum resize_modes mode) {resize_mode = mode;};



    /**
     * Get resize mode
     * @author Markus Weber
     * @date   25-May-2002
     */
    enum resize_modes get_resizemode(void) {return resize_mode;};;


    /**
     * Set minimum size of the window
     * @author Markus Weber
     * @date   11-May-2002
     */
    void set_min_windowsize(koord size);


    /**
     * @return returns the usable width and heigth of the window
     * @author Markus Weber
     * @date   11-May-2002
    */
    virtual koord get_client_windowsize() const;



};

#endif
