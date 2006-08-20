#ifndef boden_wasser_h
#define boden_wasser_h

#include "grund.h"

/**
 * Der Wasser-Untergrund modelliert Fluesse und Seen in Simutrans.
 *
 * @author Hj. Malthaner
 */

class wasser_t : public grund_t
{
private:

    static mempool_t *mempool;

    int step_nr;

public:
    wasser_t(karte_t *welt, loadsave_t *file);
    wasser_t(karte_t *welt, koord pos);

    inline bool ist_wasser() const { return true; };

    virtual hang_t::typ gib_grund_hang() const { return hang_t::flach; };


    /**
     * Ermittelt die Richtungsbits für den weg vom Typ 'typ'.
     * Liefert 15 wenn kein weg des Typs vorhanden ist und wegtyp 'wasser' ist.
     * Ein Weg kann ggf.
     * auch 0 als Richtungsbits liefern, deshalb kann die Anwesenheit eines
     * Wegs nicht hierüber, sondern mit gib_weg(), ermittelt werden.
     *
     * @author Hj. Malthaner
     */
    virtual ribi_t::ribi gib_weg_ribi(weg_t::typ typ) const;


    /**
     * Öffnet ein Info-Fenster für diesen Boden
     * @author Hj. Malthaner
     */
    virtual void zeige_info();


    void step();
    void calc_bild();
    inline const char *gib_name() const {return "Wasser";};
    inline enum typ gib_typ() const {return wasser;};


    void * operator new(size_t s);
    void operator delete(void *p);
};

#endif
