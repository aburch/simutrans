#ifndef boden_wege_dock_h
#define boden_wege_dock_h

#include "weg.h"

/**
 * Am Dock können Schiffe anlegen
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.4 $
 */

class dock_t : public weg_t
{

public:
    dock_t(karte_t *welt, loadsave_t *file);
    dock_t(karte_t *welt);

    virtual int calc_bild(koord3d pos) const;

    inline const char *gib_typ_name() const {return "Dock";};
    inline typ gib_typ() const {return wasser;};
};

#endif
