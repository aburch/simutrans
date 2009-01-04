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
	} else {
		return "Boden";
	}
}


void boden_t::calc_bild_internal()
{
	if(ist_im_tunnel()) {
		clear_back_bild();
		set_bild(IMG_LEER);
	}
#ifdef COVER_TILES
	else 	if(get_flag(grund_t::is_cover_tile)) {
		grund_t::calc_back_bild(get_hoehe()/Z_TILE_STEP,0);

		// this covers some other ground. MUST be flat!
		strasse_t *weg = static_cast<strasse_t *>(get_weg(road_wt));
		if(weg && weg->hat_gehweg()) {
			set_bild(skinverwaltung_t::fussweg->get_bild_nr(0));
		}
		else {
			set_bild( grund_besch_t::get_ground_tile(0, get_hoehe() ) );
		}
DBG_MESSAGE("boden_t::calc_bild()","at pos %i,%i,%i", get_pos().x,get_pos().y,get_pos().z );
		set_flag(grund_t::draw_as_ding);

		if(welt->lookup(get_pos().get_2d())->get_kartenboden()!=this) {
dbg->fatal("boden_t::calc_bild()","covered tile not ground?!?");
		}
	}
#endif
	else {
		uint8 slope_this =  get_grund_hang();
		weg_t *weg = get_weg(road_wt);

#ifndef DOUBLE_GROUNDS
		if(weg  &&  weg->hat_gehweg()) {
			set_bild(skinverwaltung_t::fussweg->get_bild_nr(slope_this));
		} else {
			set_bild( grund_besch_t::get_ground_tile(slope_this,get_pos().z) );
		}
#else
		if(weg && dynamic_cast<strasse_t *>(weg)->hat_gehweg()) {
			set_bild(skinverwaltung_t::fussweg->get_bild_nr(grund_besch_t::slopetable[slope_this]));
		} else {
			set_bild( grund_besch_t::get_ground_tile(slope_this,get_hoehe() ) );
		}
#endif
		grund_t::calc_back_bild(get_hoehe()/Z_TILE_STEP,slope_this);

#ifdef COVER_TILES
		if(welt->lookup(get_pos().get_2d())->get_kartenboden()!=this) {
DBG_MESSAGE("boden_t::calc_bild()","covered at pos %i,%i,%i", get_pos().x,get_pos().y,get_pos().z );
			clear_flag(grund_t::draw_as_ding);
			set_flag(grund_t::is_cover_tile);
		}
#endif
	}
}
