#ifndef tunnelboden_h
#define tunnelboden_h

#include "boden.h"

class tunnelboden_t : public boden_t
{
private:
    static mempool_t *mempool;

    hang_t::typ hang_typ;
public:
    tunnelboden_t(karte_t *welt, loadsave_t *file);
    tunnelboden_t(karte_t *welt, koord3d pos, hang_t::typ hang_typ);

    virtual void rdwr(loadsave_t *file);

    inline bool ist_tunnel() const { return true; };

    virtual hang_t::typ gib_grund_hang() const { return hang_typ; };
    virtual hang_t::typ gib_weg_hang() const { return hang_t::flach; };

    void calc_bild();

    inline const char *gib_name() const {return "Tunnelboden";};
    inline enum typ gib_typ() const {return tunnelboden;};

    void * operator new(size_t s);
    void operator delete(void *p);
};

#endif
