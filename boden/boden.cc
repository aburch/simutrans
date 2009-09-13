/*
 * Natur-Untergrund für Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simhalt.h"
#include "../simwin.h"
#include "../simskin.h"
#include "../simintr.h"

#include "../gui/ground_info.h"
#include "../gui/karte.h"
#include "../dataobj/umgebung.h"

#include "boden.h"
#include "wege/strasse.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../simtools.h"
#include "../simimg.h"

#include "../tpl/ptrhashtable_tpl.h"


boden_t::boden_t(karte_t *welt, koord3d pos,hang_t::typ sl) : grund_t(welt, pos)
{
	slope = sl;
}


const char *boden_t::get_name() const
{
	if(get_halt().is_bound()) {
		return get_halt()->get_name();
	} else if(ist_uebergang()) {
		return "Kreuzung";
	} else if(hat_wege()) {
		return get_weg_nr(0)->get_name();
	}
	else {
		return "Boden";
	}
}


void boden_t::calc_bild_internal()
{
		uint8 slope_this =  get_disp_slope();
		weg_t *weg = get_weg(road_wt);

#ifndef DOUBLE_GROUNDS

		if (is_visible()) {
			if(weg  &&  weg->hat_gehweg()) {
				set_bild(skinverwaltung_t::fussweg->get_bild_nr(slope_this));
			}
			else {
				set_bild(grund_besch_t::get_ground_tile(slope_this,get_disp_height()) );
			}
		}
		else
		{
			set_bild(IMG_LEER);
		}
#else
		if(weg && dynamic_cast<strasse_t *>(weg)->hat_gehweg()) {
			set_bild(skinverwaltung_t::fussweg->get_bild_nr(grund_besch_t::slopetable[slope_this]));
		}
		else {
			set_bild( grund_besch_t::get_ground_tile(slope_this,get_hoehe() ) );
		}
#endif
		grund_t::calc_back_bild(get_disp_height()/Z_TILE_STEP,slope_this);
}
