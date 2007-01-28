#ifndef dings_lagerhaus_h
#define dings_lagerhaus_h

#include "../simware.h"
#include "gebaeude.h"

#error "Unsupported!"

class lagerhaus_t : public gebaeude_t
{
private:

    static int max_lager;

    ware_t lager [warenbauer::MAX_WAREN];

public:


    lagerhaus_t(karte_t *welt, loadsave_t *file);
    lagerhaus_t(karte_t *welt, koord3d pos, spieler_t *sp);

    ~lagerhaus_t();

    /**
     * in top-level fenstern wird der Name in der Titelzeile dargestellt
     * @return den nicht uebersetzten Namen der Komponente
     */
    virtual const char* gib_name() const { return "Lagerhaus"; }

    enum ding_t::typ gib_typ() const { return lagerhaus; }

    bool nimmt_an(int wtyp) const;

    bool gibt_ab(int wtyp) const;

    /**
     * holt ware ab
     * @return abgeholte menge
     */
    int hole_ab(int wtyp, int menge );

    /**
     * liefert ware an
     * @return angenommene menge
     */
    int liefere_an(int wtyp, int menge );

    char * info_lagerbestand(char *buf) const;

    void rdwr(loadsave_t *file);
};

#endif
