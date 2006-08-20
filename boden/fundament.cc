/* fundament.cc
 *
 * Untergrund für Gebäude in Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "../simhalt.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simimg.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../dataobj/freelist.h"
#include "../dataobj/loadsave.h"

#include "grund.h"
#include "fundament.h"


fundament_t::fundament_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
	setze_bild( IMG_LEER );
	rdwr(file);
	slope = (uint8)hang_t::flach;
}

fundament_t::fundament_t(karte_t *welt, koord3d pos) : grund_t(welt, pos)
{
	setze_bild( IMG_LEER );
	if(welt->calc_natural_slope(pos.gib_2d())!=0) {
		int new_h=welt->max_hgt(pos.gib_2d());
/*
		if(welt->lookup(koord(1,0)+pos.gib_2d())) {
			int h = welt->lookup(koord(1,0)+pos.gib_2d())->gib_kartenboden()->gib_hoehe();
			if(h+32<new_h) {
				new_h = h+32;
			}
		}
		if(welt->lookup(koord(0,1)+pos.gib_2d())) {
			int h = welt->lookup(koord(0,1)+pos.gib_2d())->gib_kartenboden()->gib_hoehe();
			if(h+32<new_h) {
				new_h = h+32;
			}
		}
*/
		setze_hoehe( new_h );
	}
	slope = (uint8)hang_t::flach;
}


/**
 * Auffforderung, ein Infofenster zu öffnen.
 * @author Hj. Malthaner
 */
bool fundament_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
		return true;
	}
	return false;
}



void
fundament_t::calc_bild()
{
	slope = 0;
	grund_t::calc_bild();
	grund_t::calc_back_bild(gib_hoehe()/16,0);
}



void * fundament_t::operator new(size_t /*s*/)
{
	return (fundament_t *)freelist_t::gimme_node(sizeof(fundament_t));
}


void fundament_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(fundament_t),p);
}
