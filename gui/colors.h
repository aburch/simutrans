#ifndef color_gui_h
#define color_gui_h

#ifndef gui_button_h
#include "button.h"
#endif

#ifndef gui_infowin_h
#include "infowin.h"
#endif


/**
 * Menü zur Änderung der Anzeigeeinstellungen.
 * @author Hj. Malthaner
 * @version $Revision: 1.10 $
 */
class color_gui_t : public infowin_t
{
private:
    vector_tpl<button_t> buttons;

public:

    color_gui_t(karte_t *welt);

    /* return NULL->orange frame *
     * @author Hj. Malthaner
     */
    virtual spieler_t* gib_besitzer() const { return NULL; }

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const;


    const char *gib_name() const;

    int bild() const;
    void info(cbuffer_t & buf) const;

    koord gib_fenstergroesse() const;
    void infowin_event(const event_t *ev);

    /**
     *
     * @author Hj. Malthaner
     * @return ein Vector mit Buttons fuer das Beobachtungsfenster
     */
    vector_tpl<button_t> *gib_fensterbuttons();

    void zeichnen(koord pos, koord gr);
};

#endif
