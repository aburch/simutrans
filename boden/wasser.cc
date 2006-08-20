/* wasser.cc
 *
 * Wasser-Untergrund für Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "wasser.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../simhalt.h"
#include "../mm/mempool.h"
#include "../besch/grund_besch.h"


mempool_t * wasser_t::mempool = new mempool_t(sizeof(wasser_t) );

wasser_t::wasser_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
    rdwr(file);
    step_nr = simrand(5);
    wasser_t::step();
}

wasser_t::wasser_t(karte_t *welt, koord pos) : grund_t(welt, koord3d(pos, welt->gib_grundwasser()))
{
    step_nr = simrand(5);
    wasser_t::step();
}


void
wasser_t::zeige_info()
{
    if(gib_halt().is_bound()) {
        gib_halt()->zeige_info();
    }
}


void
wasser_t::step()
{
    // Hajo: it's not good to modify too much values, so it's better only to
    // read step_nr and do a little calculation.
    setze_bild( grund_besch_t::wasser->gib_bild(
	hang_t::flach, step_nr+(welt->gib_zeit_ms()>>11)));
}

void
wasser_t::calc_bild()
{
    setze_hoehe( welt->gib_grundwasser() );
    setze_weg_bild(-1);
    setze_back_bild(-1);
}


ribi_t::ribi wasser_t::gib_weg_ribi(weg_t::typ typ) const
{
    const weg_t *weg = gib_weg(typ);

    if(weg) {
	return weg->gib_ribi();
    } else if(typ == weg_t::wasser) {
	return ribi_t::alle;
    } else {
	return ribi_t::keine;
    }
}


void *
wasser_t::operator new(size_t /*s*/)
{
//    printf("new wasser_t\n");

    return mempool->alloc();
}


void
wasser_t::operator delete(void *p)
{
//    printf("delete wasser_t\n");

    mempool->free( p );
}
