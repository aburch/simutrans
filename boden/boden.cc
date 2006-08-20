/* boden.cc
 *
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

#include "boden.h"
#include "wege/strasse.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../simtools.h"
#include "../simimg.h"


bool boden_t::show_grid = false;


/**
 * Toggle ground grid display
 * @author Hj. Malthaner
 */
void boden_t::toggle_grid()
{
	show_grid = !show_grid;

	for(int y=0; y<welt->gib_groesse_y(); y++) {
		for(int x=0; x<welt->gib_groesse_x(); x++) {

			const planquadrat_t *plan = welt->lookup(koord(x,y));
			const int boden_count = plan->gib_boden_count();

			for(int schicht=0; schicht<boden_count; schicht++) {
				grund_t *gr = plan->gib_boden_bei(schicht);
				gr->calc_bild();
			}
		}

		INT_CHECK("simworld 1890");
	}
	// recalc old settings (since gr->calc_bild() will recalculate height)
	reliefkarte_t::gib_karte()->set_mode( reliefkarte_t::gib_karte()->get_mode());
}



/**
 * Toggle ground seasons
 * @author Hj. Malthaner
 */
void boden_t::toggle_season(int season)
{
	// change grounds to winter?
	if(season==2  &&  grund_besch_t::winter_boden!=NULL) {
		grund_besch_t::boden = grund_besch_t::winter_boden;
	}
	else {
		grund_besch_t::boden = grund_besch_t::standard_boden;
	}
	// same for coast lines
	if(season==2  &&  grund_besch_t::winter_ufer!=NULL) {
		grund_besch_t::ufer = grund_besch_t::winter_ufer;
	}
	else {
		grund_besch_t::ufer = grund_besch_t::standard_ufer;
	}

	// now redraw ground image
	for(int y=0; y<welt->gib_groesse_y(); y++) {
		for(int x=0; x<welt->gib_groesse_x(); x++) {

			const planquadrat_t *plan = welt->lookup(koord(x,y));
			const int boden_count = plan->gib_boden_count();

			for(int schicht=0; schicht<boden_count; schicht++) {
				grund_t *gr = plan->gib_boden_bei(schicht);
				gr->calc_bild();
			}
		}

		INT_CHECK("simworld 1890");
	}
}




boden_t::boden_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
    rdwr(file);
//DBG_DEBUG("boden_t::rdwr()", "loaded at %i,%i with %i dinge.", gib_pos().x, gib_pos().y, obj_count());
}


boden_t::boden_t(karte_t *welt, koord3d pos) : grund_t(welt, pos)
{
}


boden_t::~boden_t()
{
}


const char *boden_t::gib_name() const
{
    if(gib_halt().is_bound()) {
	return gib_halt()->gib_name();
    } else if(ist_uebergang()) {
	return "Kreuzung";
    } else  if(hat_wege()) {
	return gib_weg_name();
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
		if(hat_wege()) {	// if this is true, then all land info is shown
			// there is some info!
			if(!grund_infos->get(this)) {
				grund_infos->put(this, new grund_info_t(welt, this));
			}
			create_win(-1, -1, grund_infos->get(this), w_autodelete);
			return true;
		}
	}
	return false;
}


void
boden_t::calc_bild()
{
	grund_t::calc_bild();

	if(get_flag(grund_t::is_cover_tile)) {
		grund_t::calc_back_bild(gib_hoehe()/16,0);

		// this covers some other ground. MUST be flat!
		strasse_t *weg = static_cast<strasse_t *>(gib_weg(weg_t::strasse));
		if(weg && weg->hat_gehweg()) {
			setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(0));
		}
		else {
			setze_bild( grund_besch_t::boden->gib_bild(0) );
		}
DBG_MESSAGE("boden_t::calc_bild()","at pos %i,%i,%i", gib_pos().x,gib_pos().y,gib_pos().z );
		set_flag(grund_t::draw_as_ding);

		if(welt->lookup(gib_pos().gib_2d())->gib_kartenboden()!=this) {
DBG_MESSAGE("boden_t::calc_bild()","ERROR: covered tile not ground?!?");
		}
	}
	else {
		const koord k = gib_pos().gib_2d();
		uint8 slope_this =  gib_grund_hang();

		const int min_h=welt->min_hgt(k);
		const int max_h=welt->max_hgt(k);

		weg_t *weg = gib_weg(weg_t::strasse);

#ifndef DOUBLE_GROUNDS
		if(weg && dynamic_cast<strasse_t *>(weg)->hat_gehweg()) {
			setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(slope_this));
		} else if(gib_hoehe() == welt->gib_grundwasser()) {
			setze_bild(grund_besch_t::ufer->gib_bild(slope_this));
		} else {
			const int offset = show_grid * 19;
			sint8 hang = slope_this;

			if(hang == 0) {
				const int chance = simrand(1000);
				// Hajo: variant tiles
				if(chance < 20) {
					hang += 15;
				} else if(chance < 80) {
					hang += 16;
				} else if(chance < 300) {
					hang += 17;
				} else if(chance < 310) {
					hang += 18;
				}
			}
			int bild=IMG_LEER;
			bild = grund_besch_t::boden->gib_bild(hang + offset);
			setze_bild( bild );
		}
#else
		if(weg && dynamic_cast<strasse_t *>(weg)->hat_gehweg()) {
			setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(grund_besch_t::slopetable[slope_this]));
		} else if(max_h-min_h==16  &&  min_h==welt->gib_grundwasser()) {
			setze_bild(grund_besch_t::ufer->gib_bild(grund_besch_t::ufer->get_double_hang(slope_this)));
		} else if(min_h<welt->gib_grundwasser()) {
			setze_bild(grund_besch_t::ufer->gib_bild(grund_besch_t::ufer->get_double_hang(slope_this)));
		} else {
			setze_bild( grund_besch_t::boden->gib_bild(grund_besch_t::boden->get_double_hang(slope_this)) );
		}
#endif
		grund_t::calc_back_bild(gib_hoehe()/16,slope_this);

		if(welt->lookup(gib_pos().gib_2d())->gib_kartenboden()!=this) {
DBG_MESSAGE("boden_t::calc_bild()","covered at pos %i,%i,%i", gib_pos().x,gib_pos().y,gib_pos().z );
			clear_flag(grund_t::draw_as_ding);
			clear_flag(grund_t::is_cover_tile);
		}
	}
}



void * boden_t::operator new(size_t /*s*/)
{
	return (boden_t *)freelist_t::gimme_node(sizeof(boden_t));
}


void boden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(boden_t),p);
}
