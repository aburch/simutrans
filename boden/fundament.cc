/* fundament.cc
 *
 * Untergrund für Gebäude in Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "fundament.h"
#include "../simhalt.h"

#include "../dataobj/freelist.h"


fundament_t::fundament_t(karte_t *welt, loadsave_t *file) : boden_t(welt,file)
{
	// already handled by boden_t()
}

fundament_t::fundament_t(karte_t *welt, koord3d pos) : boden_t(welt, pos)
{
	setze_bild( -1 );
}


/**
 * Auffforderung, ein Infofenster zu öffnen.
 * @author Hj. Malthaner
 */
void fundament_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
	}
}


void
fundament_t::calc_bild()
{
	boden_t::calc_bild();
//	setze_bild( -1 );
}


void * fundament_t::operator new(size_t /*s*/)
{
	return (fundament_t *)freelist_t::gimme_node(sizeof(fundament_t));
}


void fundament_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(fundament_t),p);
}
