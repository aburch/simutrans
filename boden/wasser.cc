/*
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
wasser_t::calc_bild_internal()
{
	setze_hoehe( welt->gib_grundwasser() );
	sint16 zpos = min( welt->lookup_hgt(gib_pos().gib_2d()), welt->gib_grundwasser() ); // otherwise slope will fail ...
	setze_bild( grund_besch_t::gib_ground_tile(0,zpos) );
	slope = hang_t::flach;
	// artifical walls from here on ...
	grund_t::calc_back_bild(welt->gib_grundwasser()/Z_TILE_STEP,0);
}
