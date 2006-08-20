#ifndef dings_tunnel_h
#define dings_tunnel_h

#include "../simdings.h"

class tunnel_besch_t;

class tunnel_t : public ding_t
{
    const tunnel_besch_t *besch;    // NULL für die unterirdischen!

public:
    tunnel_t(karte_t *welt, loadsave_t *file);
    tunnel_t(karte_t *welt, koord3d pos, spieler_t *sp, const tunnel_besch_t *besch);

    const char *gib_name() const {return "Tunnelmuendung";};
    enum ding_t::typ gib_typ() const {return tunnel;};

    virtual int gib_bild() const;
    virtual int gib_after_bild() const;


    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     */
    virtual void info(cbuffer_t & buf) const;


    virtual void rdwr(loadsave_t *file);


    /**
     * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
     * um das Aussehen des Dings an Boden und Umgebung anzupassen
     *
     * @author Hj. Malthaner
     */
    virtual void laden_abschliessen();

    /**
     * Normally step is disabled; enable only for removal!
     * @author prissi
     */
    virtual bool step(long /*delta_t*/) {return false;};
};

#endif
