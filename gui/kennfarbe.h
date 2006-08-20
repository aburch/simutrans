/*
 * kennfarbe.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_kennfarbe_h
#define gui_kennfarbe_h

#ifndef gui_infowin_h
#include "infowin.h"
#endif


/**
 * Hierueber kann der Spieler seine Kennfarbe einstellen
 *
 * @author Hj. Malthaner
 */
class farbengui_t : public infowin_t
{
public:
    farbengui_t(karte_t *welt);


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const;


    /**
     * in top-level fenstern wird der Name in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     * @author Hj. Malthaner
     */
    const char *gib_name() const;


    /**
     * gibt den Besitzer zurück
     *
     * @author Hj. Malthaner
     */
    spieler_t* gib_besitzer() const;

    /**
     * Jedes Objekt braucht ein Bild.
     *
     * @author Hj. Malthaner
     * @return Die Nummer des aktuellen Bildes für das Objekt.
     */
    int gib_bild() const;


     /**
     * @return Einen Beschreibungsstext für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird, NULL wenn kein Fenster angezeigt
     * werden soll
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    void info(cbuffer_t & buf) const;


    /**
     * @return gibt wunschgroesse für das beobachtungsfenster zurueck
     */
    koord gib_fenstergroesse() const;

    void infowin_event(const event_t *ev);
};

#endif
