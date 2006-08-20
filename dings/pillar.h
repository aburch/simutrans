#ifndef dings_pillar_h
#define dings_pillar_h

#include "../simdings.h"
#include "../besch/bruecke_besch.h"

/** bruecke.h
 *
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

class loadsave_t;
class karte_t;
class cbuffer_t;

class pillar_t : public ding_t
{
	const bruecke_besch_t *besch;
	uint8 dir;
protected:
    void rdwr(loadsave_t *file);
public:
    pillar_t(karte_t *welt, loadsave_t *file);
    pillar_t(karte_t *welt, koord3d pos, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img, int hoehe);
    ~pillar_t();

    const char *gib_name() const {return "Pillar";};
    enum ding_t::typ gib_typ() const {return ding_t::pillar;};

    const char * ist_entfernbar(const spieler_t *sp);
    const bruecke_besch_t *gib_besch() const { return besch; };

    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     */
    virtual void info(cbuffer_t & buf) const;
};
#endif
