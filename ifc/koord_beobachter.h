#ifndef _koord_beobachter
#define _koord_beobachter

#include "../simtypes.h"

class koord_beobachter
{
public:
    virtual bool neue_koord(koord3d k) = 0;
    virtual void wieder_koord(koord3d k) = 0;

    virtual bool ist_uebergang_ok(koord3d pos1, koord3d pos2) = 0;
};

#endif
