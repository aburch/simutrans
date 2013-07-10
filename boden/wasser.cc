/*
 * Wasser-Untergrund für Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "wasser.h"

#include "../simworld.h"

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


void wasser_t::calc_bild_internal()
{
	set_hoehe( welt->get_grundwasser() );
	slope = hang_t::flach;

	sint16 zpos = min( welt->lookup_hgt(get_pos().get_2d()), welt->get_grundwasser() ); // otherwise slope will fail ...

	if (grund_t::underground_mode==grund_t::ugm_level && grund_t::underground_level < zpos) {
		set_bild(IMG_LEER);
	}
	else {
		set_bild( min( welt->get_grundwasser()-zpos, grund_besch_t::water_depth_levels ) /*grund_besch_t::get_ground_tile(0,zpos)*/ );
	}
	// test for ribis
	ribi = ribi_t::keine;
	grund_t *grw = welt->lookup_kartenboden(pos.get_2d() + koord(-1,0)); // west
	if (grw  &&  (grw->ist_wasser()  ||  grw->hat_weg(water_wt))) {
		ribi |= ribi_t::west;
	}
	grund_t *grn = welt->lookup_kartenboden(pos.get_2d() + koord(0,-1)); // nord
	if (grn  &&  (grn->ist_wasser()  ||  grn->hat_weg(water_wt))) {
		ribi |= ribi_t::nord;
	}
	grund_t *gre = welt->lookup_kartenboden(pos.get_2d() + koord(1,0)); // ost
	if (gre  &&  (gre->ist_wasser()  ||  gre->hat_weg(water_wt))) {
		ribi |= ribi_t::ost;
	}
	grund_t *grs = welt->lookup_kartenboden(pos.get_2d() + koord(0,1)); // sued
	if (grs  &&  (grs->ist_wasser()  ||  grs->hat_weg(water_wt))) {
		ribi |= ribi_t::sued;
	}

	// artifical walls from here on ...
	grund_t::calc_back_bild(welt->get_grundwasser(), 0);
}


void wasser_t::rotate90()
{
	grund_t::rotate90();
	ribi = ribi_t::rotate90(ribi);
}
