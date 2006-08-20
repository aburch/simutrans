#ifndef gui_messagebox_h
#define gui_messagebox_h

#ifndef gui_infowin_h
#include "infowin.h"
#endif

/**
 * Eine Klasse für Nachrichtenfenster.
 * @author Hj. Malthaner
 */
class nachrichtenfenster_t : public infowin_t
{
private:
    const char * meldung;
    int bild;
    koord bild_offset;

public:
    nachrichtenfenster_t(karte_t *welt, const char *text);


    nachrichtenfenster_t(karte_t *welt, const char *text, int bild);


    nachrichtenfenster_t::nachrichtenfenster_t(karte_t *welt, const char *text, int bild, koord off);


    /**
     * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
     * zurück
     * @author Hj. Malthaner
     */
    //fensterfarben gib_fensterfarben() const;


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
     * in top-level fenstern wird der Name in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     * @author Hj. Malthaner
     */
    const char *gib_name() const;


    void info(cbuffer_t & buf) const;
};

#endif
