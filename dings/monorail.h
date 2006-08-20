#ifndef dings_monorail_h
#define dings_monorail_h

#include "../besch/monorail_besch.h"

/** monorail.h
 *
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

class monorail_t : public ding_t
{
    const weg_besch_t *besch;
protected:
    void rdwr(loadsave_t *file);
public:
    monorail_t(karte_t *welt, loadsave_t *file);
    monorail_t(karte_t *welt, koord3d pos, int y_off, spieler_t *sp,
	      const monorail_besch_t *besch, monorail_besch_t::img_t img);
    ~monorail_t();

    virtual int gib_after_bild() const {return besch->gib_vordergrund(img); };

    const char *gib_name() const {return "Bruecke";};
    enum ding_t::typ gib_typ() const {return monorail;};

	const monorail_besch_t *gib_besch() const { return besch; };

    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     */
    virtual void info(cbuffer_t & buf) const;
};
#endif
