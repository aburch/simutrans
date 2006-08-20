#ifndef message_option_h
#define message_option_h

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
class message_option_t : public infowin_t
{
private:
    vector_tpl<button_t> *buttons;
    int ticker_msg, window_msg, auto_msg, ignore_msg;

public:
    message_option_t(karte_t *welt);
    ~message_option_t();


    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "mailbox.txt";};


    const char *gib_name() const;

    /**
     * gibt den Besitzer zurück
     *
     * @author Hj. Malthaner
     */
    spieler_t* gib_besitzer() const;

    int gib_bild() const;
    koord gib_bild_offset() const;
    void info(cbuffer_t & buf) const;

    koord gib_fenstergroesse() const;
    void infowin_event(const event_t *ev);
    vector_tpl<button_t> *gib_fensterbuttons();
};

#endif
