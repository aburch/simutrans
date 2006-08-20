#ifndef dings_dummy_h
#define dings_dummy_h

#include "../simdings.h"


/**
 * prissi: a dummy typ for old things, which are now ignored
 */
class dummy_ding_t : public ding_t
{
public:
    dummy_ding_t(karte_t *welt, loadsave_t *file);
    dummy_ding_t(karte_t *welt, koord3d pos, spieler_t *sp);

    /**
     * @return Gibt den Namen des Objekts zurück.
     */
    const char *gib_name() const {return "Dummy";};

    /**
     * @return Gibt den typ des Objekts zurück.
     */
    inline enum ding_t::typ gib_typ() const {return ding_t::undefined;};

    void zeige_info() {};  // dummies do not have an info

    virtual void laden_abschliessen() {};

    void * operator new(size_t s);
    void operator delete(void *p);
};


#endif
