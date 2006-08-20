/* dock.cc
 *
 * Eine Sorte Wasser die zu einer Haltestelle gehört
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simworld.h"
#include "../../simimg.h"
#include "../../besch/grund_besch.h"

#include "dock.h"

dock_t::dock_t(karte_t *welt, loadsave_t *file) :  weg_t(welt)
{
    rdwr(file);
}


dock_t::dock_t(karte_t *welt) : weg_t (welt)
{
    setze_ribi(ribi_t::alle);
}

int dock_t::calc_bild(koord3d /*pos*/) const
{
    return grund_besch_t::wasser->gib_bild(hang_t::flach);
}
