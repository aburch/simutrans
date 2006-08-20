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
	}
	// recalc old settings (since gr->calc_bild() will recalculate height)
	reliefkarte_t::gib_karte()->set_mode( reliefkarte_t::gib_karte()->get_mode());
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
		if(hat_wege()) {
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


/**
 * Needed to synchronize map ground with map height. Should do
 * nothing for ground types which are not linked to map height
 * @author Hj. Malthaner
 */
void boden_t::sync_height()
{
    // planquadrathoehe = minimale planquadrateckpunkthoehe

    setze_hoehe( welt->min_hgt(gib_pos().gib_2d()) );
}


static const uint8 double_slope_table[16] = {
  0,     // 0 - unused entry, should be blank!
    4,   // 1
  3,     // 2
    5,   // 3
  1,     // 4
   11,   // 5
  12,    // 6
    9,   // 7

  2,     // 8
   14,   // 9
  13,    // 10
   10,   // 11
  0,     // 12
    6,   // 13
  7,     // 14
    8,   // 15
};




void
boden_t::calc_bild()
{
	const koord k = gib_pos().gib_2d();
	uint8 slope_this =  welt->get_slope(k);
	const uint8 natural_slope_this = welt->calc_natural_slope(k);

	grund_t::calc_bild();

	weg_t *weg = gib_weg(weg_t::strasse);

	if(weg && dynamic_cast<strasse_t *>(weg)->hat_gehweg()) {
		setze_bild(skinverwaltung_t::fussweg->gib_bild_nr(gib_grund_hang()));
	} else if(gib_hoehe() == welt->gib_grundwasser()) {
		setze_bild(grund_besch_t::ufer->gib_bild(gib_grund_hang()));
	} else {
		const int offset = show_grid * 19;
		int hang = gib_grund_hang();

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
		if(slope_this!=0  &&  slope_this!=natural_slope_this) {
			bild = grund_besch_t::boden->gib_bild(53+(slope_this/3));
//DBG_DEBUG("slope","%i",slope_this);
		}
		if(bild==IMG_LEER) {
			bild = grund_besch_t::boden->gib_bild(hang + offset);
		}
		setze_bild( bild );
	}

	int back_bild = IMG_LEER;

	const planquadrat_t *left  = welt->lookup(k - koord(1,0));
	const planquadrat_t *right = welt->lookup(k - koord(0,1));

	if(left!=NULL  && right!=NULL) {
		grund_t * lgr = left->gib_kartenboden();
		grund_t * rgr = right->gib_kartenboden();

		const int hh = gib_hoehe()/16;
		int slope_wall = 0;

		if(!lgr->ist_wasser()) {
			// only, if not water to the left
			const int lh = lgr->gib_hoehe()/16;
			const uint8 slope_left =  welt->get_slope(k - koord(1,0));

			// add corners of left slope
			sint16 leftheights[2] = { ((slope_left&2)!=0)+lh, ((slope_left&4)!=0)+lh };
			sint8 llhdiff = (leftheights[0]) - (((slope_this&1)!=0)+hh);
			sint8 lhhdiff = (leftheights[1]) - (((slope_this&8)!=0) + hh);
			slope_wall = (llhdiff>0)*4 + (lhhdiff>0)*8;
			// only down-slopes in table
			// thus upslopes are converted to solid walls
			if(slope_wall>0  &&  leftheights[0]==leftheights[1]) {
				slope_wall = 12;
			}
		}

		if(!rgr->ist_wasser()) {
			const int lr = rgr->gib_hoehe()/16;
			const uint8 slope_right = welt->get_slope(k - koord(0,1));

			// add corner of right slope
			sint16 backheights[2] = { ((slope_right&1)!=0)+lr, ((slope_right&2)!=0)+lr };
			sint8 hlhdiff = (backheights[0]) - (((slope_this&8)!=0) + hh);
			sint8 hhhdiff = (backheights[1]) - (((slope_this&4)!=0) + hh);
			slope_wall |= (hlhdiff>0)*2 + (hhhdiff>0)*1;
			// only down-slopes in table
			// thus upslopes are converted to solid walls
			if((slope_wall&3)>0  &&  backheights[0]==backheights[1]) {
					slope_wall |= 3;
			}
		}

		// found a slope?
		if(slope_wall!=0) {
				back_bild = grund_besch_t::boden->gib_bild(38 + double_slope_table[slope_wall]);
		}
	}

	setze_back_bild(back_bild);
}


void * boden_t::operator new(size_t /*s*/)
{
	return (boden_t *)freelist_t::gimme_node(sizeof(boden_t));
}


void boden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(boden_t),p);
}
