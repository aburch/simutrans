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
	slope = (uint8)hang_t::flach;
}

wasser_t::wasser_t(karte_t *welt, koord pos) : grund_t(welt, koord3d(pos, welt->gib_grundwasser()))
{
	slope = (uint8)hang_t::flach;
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
wasser_t::calc_bild()
{
	setze_bild( grund_besch_t::gib_ground_tile(0,gib_hoehe() ) );
	setze_hoehe( welt->gib_grundwasser() );
	// artifical walls from here on ...
	grund_t::calc_back_bild(gib_hoehe()/16,0);
}



void *
wasser_t::operator new(size_t /*s*/)
{
//	assert(s==sizeof(wasser_t));
	return static_cast<wasser_t *>(freelist_t::gimme_node(sizeof(wasser_t)));
}


void
wasser_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(wasser_t),p);
}
