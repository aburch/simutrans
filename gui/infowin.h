/*
 * infowin.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_infowin_h
#define gui_infowin_h


#ifndef ifc_gui_fenster_h
#include "../ifc/gui_fenster.h"
#endif

#ifndef koord3d_h
#include "../dataobj/koord3d.h"
#endif

#ifndef tpl_vector_h
#include "../tpl/vector_tpl.h"
#endif

#ifndef gui_button_h
#include "button.h"
#endif

#ifndef world_view_t_h
#include "world_view_t.h"
#endif


class spieler_t;
class button_t;
class karte_t;
class cbuffer_t;

/**
 * Dies ist eine abstrakte Basisklasse für Infofenster.
 * Sie implementiert zeichnen().
 * Zeichnen() benutzt die anderen Methoden zum einholen der notwendigen
 * Informationen für das Aussehen.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.14 $
 */
class infowin_t : virtual public gui_fenster_t
{
private:
    world_view_t view;

protected:
    void unpress_buttons();

    void draw_buttons(const koord pos,
		      const int button_farbe);

    int calc_fensterhoehe_aus_info() const;

    /**
     * Zeiger auf die Karte zu der der Untergrund gehoert.
     * Der Zeiger ist eine Klassenvariable um Speicher zu sparen, sollen
     * mehrere Karten unterschieden werden, muss er eine Instanzvariable
     * werden.
     *
     * @author Hj. Malthaner
     */
    static karte_t *welt;

public:

    infowin_t(karte_t *welt) : view(welt, koord::invalid) {this->welt = welt;};


    virtual ~infowin_t();

    /**
     * @return Einen Beschreibungsstext für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird, NULL wenn kein Fenster angezeigt
     * werden soll
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual void info(cbuffer_t & buf) const = 0;

    /**
     * Gibt den Besitzer zurück. Das ist, wenn welt gesetzt ist, der Spieler
     * Nr. 0, ansonsten NULL.
     *
     * @author Hj. Malthaner
     */
    virtual spieler_t* gib_besitzer() const;


    /**
     * @return Die aktuelle Planquadrat-Koordinate des Objekts
     *
     * @author Hj. Malthaner
     */
    virtual koord3d gib_pos() const;

    /**
     * Jedes Objekt braucht ein Bild.
     *
     * @author Hj. Malthaner
     * @return Die Nummer des aktuellen Bildes für das Objekt.
     */
    virtual int gib_bild() const;

    /**
     * Das Bild kann im Fenster über Offsets plaziert werden
     *
     * @author Hj. Malthaner
     * @return den x,y Offset des Bildes im Infofenster
     */
    virtual koord gib_bild_offset() const;

    /**
     *
     * @author Hj. Malthaner
     * @return gibt wunschgroesse für das beobachtungsfenster zurueck
     */
    virtual koord gib_fenstergroesse() const;

    /**
     *
     * @author Hj. Malthaner
     * @return eine NULL-Terminierte Liste von Buttons fuer das
     * Beobachtungsfenster
     */
    virtual vector_tpl<button_t> *gib_fensterbuttons();

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
     *
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);

    /**
     * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
     * zurück
     *
     * @author Hj. Malthaner
     */
    fensterfarben gib_fensterfarben() const;

};


#endif
