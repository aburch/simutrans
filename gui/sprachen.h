#ifndef gui_sprachen_h
#define gui_sprachen_h

#ifndef gui_infowin_h
#include "infowin.h"
#endif


class spieler_t;
class karte_t;

/**
 * Sprachauswahldialog
 *
 * @author Hj. Maltahner, Niels Roest
 */
class sprachengui_t : public infowin_t
{
private:

    vector_tpl<button_t> *buttons;

public:

    /**
     * Causes the required fonts for currently selected
     * language to be loaded
     * @author Hj. Malthaner
     */
    static void init_font_from_lang();

    sprachengui_t(karte_t *welt);

    ~sprachengui_t();


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "language.txt";};


    /**
     * in top-level fenstern wird der Name in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     * @author Hj. Malthaner
     */
    const char *gib_name() const {return "Sprache";};


    /**
     * Jedes Objekt braucht ein Bild.
     *
     * @author Hj. Malthaner
     * @return Die Nummer des aktuellen Bildes für das Objekt.
     */
    int gib_bild() const;


    /**
     * Das Bild kann im Fenster über Offsets plaziert werden
     *
     * @author Hj. Malthaner
     * @return den x,y Offset des Bildes im Infofenster
     */
    koord gib_bild_offset() const;


    /**
     *
     * @author Hj. Malthaner
     * @return gibt wunschgroesse für das beobachtungsfenster zurueck
     */
    koord gib_fenstergroesse() const;


    /**
     *
     * @author Hj. Malthaner
     * @return eine NULL-Terminierte Liste von Buttons fuer das
     * Beobachtungsfenster
     */
    virtual vector_tpl<button_t> *gib_fensterbuttons();


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
