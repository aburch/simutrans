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

#include "../dataobj/umgebung.h"



int wasser_t::stage = 0;
bool wasser_t::change_stage = false;



// for animated waves
void wasser_t::prepare_for_refresh()
{
	if(!welt->is_fast_forward()  &&  umgebung_t::water_animation>0) {
		int new_stage = (welt->get_zeit_ms() / umgebung_t::water_animation) % grund_besch_t::water_animation_stages;
		wasser_t::change_stage = (new_stage != stage);
		wasser_t::stage = new_stage;
	}
	else {
		wasser_t::change_stage = false;
	}
}



bool wasser_t::zeige_info()
{
	if(get_halt().is_bound()) {
		get_halt()->zeige_info();
		return true;
	}
	return false;
}



void
wasser_t::calc_bild_internal()
{
	set_hoehe( welt->get_grundwasser() );
	slope = hang_t::flach;

	sint16 zpos = min( welt->lookup_hgt(get_pos().get_2d()), welt->get_grundwasser() ); // otherwise slope will fail ...

	if (grund_t::underground_mode==grund_t::ugm_level && grund_t::underground_level < zpos) {
		set_bild(IMG_LEER);
	}
	else {
		set_bild( grund_besch_t::get_ground_tile(0,zpos) );
	}

	// artifical walls from here on ...
	grund_t::calc_back_bild(welt->get_grundwasser()/Z_TILE_STEP,0);
}
