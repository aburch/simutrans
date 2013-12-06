/*
 * Untergrund für Gebäude in Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "../simconst.h"

#include "../besch/grund_besch.h"
#include "../dataobj/loadsave.h"

#include "grund.h"
#include "fundament.h"


fundament_t::fundament_t(loadsave_t *file, koord pos ) : grund_t(koord3d(pos,0) )
{
	rdwr(file);
	slope = (uint8)hang_t::flach;
}


fundament_t::fundament_t(koord3d pos, hang_t::typ hang ) : grund_t(pos)
{
	set_bild( IMG_LEER );
	if(hang) {
		pos = get_pos();
		pos.z += hang_t::max_diff(hang);
		set_pos( pos );
	}
	slope = (uint8)hang_t::flach;
}


void fundament_t::calc_bild_internal()
{
	slope = 0;
	if (is_visible()) {
		set_bild( grund_besch_t::get_ground_tile(this) );
	}
	else {
		set_bild(IMG_LEER);
	}
	grund_t::calc_back_bild(get_disp_height(),0);
}
