#ifndef dings_wolke_t
#define dings_wolke_t



#include "../ifc/sync_steppable.h"

class ding_t;
class karte_t;


/**
 * Basisklasse für Wölkchen für Simutrans.
 * @author Hj. Malthaner
 * @version $Revision: 1.10 $
 */
class wolke_t : public ding_t
{
private:
    unsigned long insta_zeit;	// Wolken verschwinden, wenn alter max. erreicht

protected:

    wolke_t(karte_t *welt);
    wolke_t(karte_t *welt, koord3d pos, int xoff, int yoff, int bild);

public:
    inline int gib_insta_zeit() const {return insta_zeit;};

    const char *gib_name() const {return "Wolke";};
    enum ding_t::typ gib_typ() const {return wolke;};

    void zeige_info();

    void rdwr(loadsave_t *file);
};


/**
 * Bildsynchron bewegte Wölkchen für Simutrans.
 * @author Hj. Malthaner
 */
class sync_wolke_t : public wolke_t, public sync_steppable
{
private:
    sint16 base_y_off;
    sint16 base_image;

public:
    sync_wolke_t(karte_t *welt, loadsave_t *file);
    sync_wolke_t(karte_t *welt, koord3d pos, int xoff, int yoff, int bild);


    /**
     * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
     * @author Hj. Malthaner
     */
    virtual void sync_prepare() {};

    bool sync_step(long delta_t);
    enum ding_t::typ gib_typ() const {return sync_wolke;};

    void rdwr(loadsave_t *file);

    /**
     * Wird aufgerufen, wenn wolke entfernt wird. Entfernt wolke aus
     * der Verwaltuing synchroner Objekte.
     * @author Hj. Malthaner
     */
    virtual void entferne(spieler_t *sp);

#ifdef FREELIST_NEW
    void * operator new(size_t s);
    void operator delete(void *p);
#endif
};


/**
 * Asynchron bewegte Wölkchen für Simutrans. Bewegt sich schlecht, braucht aber
 * weniger Rechenzeit als synchrone Wolken.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.10 $
 */
class async_wolke_t : public wolke_t
{
public:
    async_wolke_t(karte_t *welt, loadsave_t *file);
    async_wolke_t(karte_t *welt, koord3d pos, int xoff, int yoff, int bild);

    bool step(long delta_t);
    enum ding_t::typ gib_typ() const {return async_wolke;};
};

#endif
