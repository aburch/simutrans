#ifndef gui_spieler_h
#define gui_spieler_h

#ifndef gui_infowin_h
#include "infowin.h"
#endif

class spieler_t;
class karte_t;

/**
 * Ein Menü zur Wahl der automatischen Spieler.
 * @author Hj. Malthaner
 * @version $Revision: 1.7 $
 */
class ki_kontroll_t : public infowin_t
{
private:
    vector_tpl<button_t> *buttons;

public:
    ki_kontroll_t(karte_t *welt);
    ~ki_kontroll_t();


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "players.txt";};


    const char *gib_name() const;

    /**
     * gibt den Besitzer zurück
     *
     * @author Hj. Malthaner
     */
    spieler_t* gib_besitzer() const;

    int gib_bild() const;
    void info(cbuffer_t & buf) const;

    koord gib_fenstergroesse() const;
    void infowin_event(const event_t *ev);
    vector_tpl<button_t> *gib_fensterbuttons();
};

#endif
