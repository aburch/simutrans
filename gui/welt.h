/*
 * welt.h
 *
 * dialog zur Eingabe der Werte fuer die Kartenerzeugung
 *
 * Hj. Malthaner
 *
 * April 2000
 */


#ifndef welt_gui_h
#define welt_gui_h

#ifndef gui_infowin_h
#include "infowin.h"
#endif


class einstellungen_t;
struct event_t;

/**
 * Ein Dialog mit Einstellungen fuer eine neue Karte
 *
 * @author Hj. Malthaner, Niels Roest
 */
class welt_gui_t : public infowin_t
{
private:
    vector_tpl<button_t> buttons;

    einstellungen_t * sets;

    enum { preview_size = 64 };

    /**
     * Mini Karten-Preview
     * @author Hj. Malthaner
     */
    unsigned char karte[preview_size*preview_size];

    bool load_heightfield;
    bool load;
    bool start;
    bool close;
    bool quit;

    /**
     * Berechnet Preview-Karte neu. Inititialisiert RNG neu!
     * @author Hj. Malthaner
     */
    void  update_preview();

public:
    welt_gui_t(karte_t *welt, einstellungen_t *sets);
    ~welt_gui_t();


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "new_world.txt";};


    bool gib_load_heightfield() const {return load_heightfield;};
    bool gib_load() const {return load;};
    bool gib_start() const {return start;};
    bool gib_close() const {return close;};
    bool gib_quit() const {return quit;};

    einstellungen_t *gib_sets() const {return sets;};

    const char *gib_name() const;


    /**
     * Jedes Objekt braucht ein Bild.
     *
     * @author Hj. Malthaner
     * @return Die Nummer des aktuellen Bildes für das Objekt.
     */
    int gib_bild() const;


    /**
     * Das Bild kann im Fenster <FC>ber Offsets plaziert werden
     *
     * @author Hj. Malthaner
     * @return den x,y Offset des Bildes im Infofenster
     */
    koord gib_bild_offset() const;


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
     *
     * @author Hj. Malthaner
     * @return gibt wunschgroesse für das beobachtungsfenster zurueck
     */
    koord gib_fenstergroesse() const;


    /**
     *
     * @author Hj. Malthaner
     * @return einen Vector von Buttons fuer das Beobachtungsfenster
     */
    vector_tpl<button_t> *gib_fensterbuttons();


    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    void infowin_event(const event_t *ev);


    /**
     * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
     * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
     * in dem die Komponente dargestellt wird.
     *
     * @author Hj. Malthaner
     */
    void zeichnen(koord pos, koord gr);
};

#endif
