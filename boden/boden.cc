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

#include "../dataobj/freelist.h"
#include "../dataobj/umgebung.h"

#include "boden.h"
#include "wege/strasse.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../simtools.h"
#include "../simimg.h"

#include "../tpl/ptrhashtable_tpl.h"


boden_t::boden_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
	rdwr(file);
//DBG_DEBUG("boden_t::rdwr()", "loaded at %i,%i with %i dinge.", gib_pos().x, gib_pos().y, obj_count());
}

boden_t::boden_t(karte_t *welt, koord3d pos,hang_t::typ sl) : grund_t(welt, pos)
{
	slope = sl;
}


const char *boden_t::gib_name() const
{
	if(gib_halt().is_bound()) {
		return gib_halt()->gib_name();
	} else if(ist_uebergang()) {
		return "Kreuzung";
	} else if(hat_wege()) {
		return gib_weg_nr(0)->gib_name();
	} else {
		return "Boden";
	}
}


bool
boden_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
		return true;
	}
	else {
		if(umgebung_t::ground_info  ||  hat_wege()) {
			// there is some info!
			grund_info_t* gi = grund_infos.get(this);
			if (gi == NULL) {
				gi = new grund_info_t(this);
				grund_infos.put(this, gi);
			}
			create_win(-1, -1, gi, w_autodelete);
			return true;
		}
	}
	return false;
}


void
boden_t::calc_bild()
{
	grund_t::calc_bild();

	if(ist_im_tunnel()) {
		clear_back_bild();
		setze_bild(IMG_LEER);
		if(flags&has_way1) {
			((weg_t *)obj_bei(0))->setze_bild(IMG_LEER);
		}
		if(flags&has_way2) {
			((weg_t *)obj_bei(1))->setze_bild(IMG_LEER);
		}
	}
#ifdef COVER_TILES
	else 	if(get_flag(grund_t::is_cover_tile)) {
		grund_t::calc_back_bild(gib_hoehe()/Z_TILE_STEP,0);

		// this covers some other ground. MUST be flat!
		strasse_t *weg = static_cast<strasse_t *>(gib_weg(road_wt));
		if(weg && weg->hat_gehweg()) {
			setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(0));
		}
		else {
			setze_bild( grund_besch_t::gib_ground_tile(0, gib_hoehe() ) );
		}
DBG_MESSAGE("boden_t::calc_bild()","at pos %i,%i,%i", gib_pos().x,gib_pos().y,gib_pos().z );
		set_flag(grund_t::draw_as_ding);

		if(welt->lookup(gib_pos().gib_2d())->gib_kartenboden()!=this) {
dbg->fatal("boden_t::calc_bild()","covered tile not ground?!?");
		}
	}
#endif
	else {
		uint8 slope_this =  gib_grund_hang();
		weg_t *weg = gib_weg(road_wt);

#ifndef DOUBLE_GROUNDS
		if(weg && dynamic_cast<strasse_t *>(weg)->hat_gehweg()) {
			setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(slope_this));
		} else {
			setze_bild( grund_besch_t::gib_ground_tile(slope_this,gib_pos().z) );
		}
#else
		if(weg && dynamic_cast<strasse_t *>(weg)->hat_gehweg()) {
			setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(grund_besch_t::slopetable[slope_this]));
		} else {
			setze_bild( grund_besch_t::gib_ground_tile(slope_this,gib_hoehe() ) );
		}
#endif
		grund_t::calc_back_bild(gib_hoehe()/Z_TILE_STEP,slope_this);

#ifdef COVER_TILES
		if(welt->lookup(gib_pos().gib_2d())->gib_kartenboden()!=this) {
DBG_MESSAGE("boden_t::calc_bild()","covered at pos %i,%i,%i", gib_pos().x,gib_pos().y,gib_pos().z );
			clear_flag(grund_t::draw_as_ding);
			set_flag(grund_t::is_cover_tile);
		}
#endif
	}
}



void * boden_t::operator new(size_t /*s*/)
{
//	assert(s==sizeof(boden_t));
	return freelist_t::gimme_node(sizeof(boden_t));
}


void boden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(boden_t),p);
}
