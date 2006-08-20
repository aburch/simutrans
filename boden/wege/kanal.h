#ifndef boden_wege_dock_h
#define boden_wege_dock_h

#include "weg.h"
#include "kanal.h"
#include "../../dataobj/loadsave.h"
#include "../../utils/cbuffer_t.h"

/**
 * Am Dock können Schiffe anlegen
 *
 * @author Hj. Malthaner
 * @version $Revision: 1.4 $
 */

class kanal_t : public weg_t
{

public:
    kanal_t(karte_t *welt, loadsave_t *file);
    kanal_t(karte_t *welt);

    virtual void calc_bild(koord3d) { weg_t::calc_bild(); }
    inline const char *gib_typ_name() const {return "Kanal";};
    inline typ gib_typ() const {return wasser;};
    virtual void rdwr(loadsave_t *file);
    void info(cbuffer_t & buf) const;
};

#endif
