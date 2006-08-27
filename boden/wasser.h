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
    uint8 step_nr;

public:
    wasser_t(karte_t *welt, loadsave_t *file);
    wasser_t(karte_t *welt, koord pos);

    inline bool ist_wasser() const { return true; }

	// returns all directions for waser and none for the rest ...
    ribi_t::ribi gib_weg_ribi(weg_t::typ typ) const { return (typ==weg_t::wasser) ? ribi_t::alle :ribi_t::keine; }
    ribi_t::ribi gib_weg_ribi_unmasked(weg_t::typ typ) const  { return (typ==weg_t::wasser) ? ribi_t::alle :ribi_t::keine; }

	// no slopes for water
    bool setze_grund_hang(hang_t::typ) { slope=0; return false; }

    /**
     * Öffnet ein Info-Fenster für diesen Boden
     * @author Hj. Malthaner
     */
    virtual bool zeige_info();

    void step();
    void calc_bild();
    inline const char *gib_name() const {return "Wasser";}
    inline enum typ gib_typ() const {return wasser;}

    void * operator new(size_t s);
    void operator delete(void *p);
};

#endif
