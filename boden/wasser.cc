/* wasser.cc
 *
 * Wasser-Untergrund für Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "wasser.h"

#include "../simdebug.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../simhalt.h"
#include "../besch/grund_besch.h"

#include "../dataobj/freelist.h"

wasser_t::wasser_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
    rdwr(file);
    step_nr = simrand(5);
    wasser_t::step();
//DBG_DEBUG("wasser_t::wasser_t()","rdwr of wasser at %i,%i with img %i.",gib_pos().x,gib_pos().y,grund_besch_t::wasser->gib_bild(hang_t::flach, step_nr+(welt->gib_zeit_ms()>>11)));
}

wasser_t::wasser_t(karte_t *welt, koord pos) : grund_t(welt, koord3d(pos, welt->gib_grundwasser()))
{
    step_nr = simrand(5);
    wasser_t::step();
}


bool
wasser_t::zeige_info()
{
    if(gib_halt().is_bound()) {
        gib_halt()->zeige_info();
        return true;
    }
    return false;
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
	return (wasser_t *)freelist_t::gimme_node(sizeof(wasser_t));
}


void
wasser_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(wasser_t),p);
}
