#ifndef gui_optionen_h
#define gui_optionen_h


#ifndef gui_infowin_h
#include "infowin.h"
#endif


/**
 * Hauptmenüfenster für alle Einstellungen zueinem laufenden Spiel.
 * @author Hj. Malthaner
 * @version $Revision: 1.8 $
 */
class optionen_gui_t : public infowin_t
{
private:
    vector_tpl<button_t> buttons;

public:
    optionen_gui_t(karte_t *welt);


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "options.txt";};


    const char *gib_name() const;

    void info(cbuffer_t & buf) const;


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

    koord gib_fenstergroesse() const;
    void infowin_event(const event_t *ev);

    vector_tpl<button_t> *gib_fensterbuttons();

    void zeichnen(koord pos, koord gr);
};

#endif
