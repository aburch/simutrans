#ifndef brueckenboden_h
#define brueckenboden_h

#include "grund.h"

class brueckenboden_t : public grund_t
{
    hang_t::typ grund_hang;
    hang_t::typ weg_hang;

public:
    brueckenboden_t(karte_t *welt, loadsave_t *file);
    brueckenboden_t(karte_t *welt, koord3d pos, int grund_hang, int weg_hang);

    virtual void rdwr(loadsave_t *file);

    virtual int gib_weg_yoff() const;

    inline bool ist_bruecke() const { return true; };

    virtual hang_t::typ gib_grund_hang() const { return grund_hang; };
    virtual hang_t::typ gib_weg_hang() const { return weg_hang; };

    void calc_bild();

    /**
     * ground info, needed for stops
     * @author prissi
     */
    virtual bool zeige_info();

    inline const char *gib_name() const {return "Brueckenboden";};
    inline enum typ gib_typ() const {return brueckenboden;};

    void * operator new(size_t s);
    void operator delete(void *p);
};

#endif
