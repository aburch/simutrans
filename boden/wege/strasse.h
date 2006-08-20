#ifndef boden_wege_strasse_h
#define boden_wege_strasse_h

#include "weg.h"

/**
 * Auf der Strasse können Autos fahren.
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.5 $
 */

class strasse_t : public weg_t
{
    bool gehweg;

public:
	static const weg_besch_t *default_strasse;

    void setze_gehweg(bool janein);
    inline bool hat_gehweg() const {return gehweg; };

    strasse_t(karte_t *welt, loadsave_t *file);
    strasse_t(karte_t *welt);
    strasse_t(karte_t *welt,int top_speed);

    /**
     * @return Infotext zur Schiene
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;

    virtual void calc_bild(koord3d) { weg_t::calc_bild(); }

    inline const char *gib_typ_name() const {return "Strasse";};
    inline typ gib_typ() const {return strasse;};

    virtual void rdwr(loadsave_t *file);
};

#endif
