#ifndef dings_zeiger_h
#define dings_zeiger_h

#include "../simdings.h"

/**
 * Objekte dieser Klasse dienen sowohl als Landschaftszeiger im UI
 * als auch zur Markierung/Reservierung von Planquadraten.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.7 $
 */
class zeiger_t : public ding_t
{
private:
    ribi_t::ribi richtung;

public:

    void setze_richtung(ribi_t::ribi r);

    ribi_t::ribi gib_richtung() const {return richtung;};


    zeiger_t(karte_t *welt, loadsave_t *file);
    zeiger_t(karte_t *welt, koord3d pos, spieler_t *sp);

    ~zeiger_t();


    /**
     * Zeiger bewegen sich manchmal in grossen Sprüngen und brauchen
     * deswegen eine andere Behandlung beim Neuzeichnen. D.h. ein
     * größerer Bereich muss 'dirty' markiert werden
     * @author Hj. Malthaner
     */
    void zeiger_t::setze_pos(koord3d k);

    const char *gib_name() const {return "Zeiger";};
    enum ding_t::typ gib_typ() const {return zeiger;};
};

#endif
