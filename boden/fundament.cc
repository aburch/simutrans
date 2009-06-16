/*
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
#include "../dataobj/loadsave.h"

#include "grund.h"
#include "fundament.h"


fundament_t::fundament_t(karte_t *welt, loadsave_t *file, koord pos ) : grund_t(welt, koord3d(pos,0) )
{
	rdwr(file);
	slope = (uint8)hang_t::flach;
}

fundament_t::fundament_t(karte_t *welt, koord3d pos, hang_t::typ hang ) : grund_t(welt, pos)
{
	set_bild( IMG_LEER );
	if(hang) {
		pos = get_pos();
		pos.z += Z_TILE_STEP;
		set_pos( pos );
	}
	slope = (uint8)hang_t::flach;
}


/**
 * Auffforderung, ein Infofenster zu öffnen.
 * @author Hj. Malthaner
 */
bool fundament_t::zeige_info()
{
	if(get_halt().is_bound()) {
		get_halt()->zeige_info();
		return true;
	}
	return false;
}



void
fundament_t::calc_bild_internal()
{
	slope = 0;
	if (is_visible()) {
		set_bild( grund_besch_t::get_ground_tile(0,get_pos().z) );
	} else {
		set_bild(IMG_LEER);
	}
	grund_t::calc_back_bild(get_disp_height()/Z_TILE_STEP,0);
}
