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
#include "../mm/mempool.h"

#include "../gui/ground_info.h"

#include "boden.h"
#include "wege/strasse.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../simtools.h"

mempool_t * boden_t::mempool = new mempool_t(sizeof(boden_t) );

bool boden_t::show_grid = false;


/**
 * Toggle ground grid display
 * @author Hj. Malthaner
 */
void boden_t::toggle_grid()
{
  const int groesse = welt->gib_groesse();

  show_grid = !show_grid;


  for(int j=0; j<groesse; j++) {
    for(int i=0; i<groesse; i++) {

      const planquadrat_t *plan = welt->lookup(koord(i,j));
      const int boden_count = plan->gib_boden_count();

      for(int schicht=0; schicht<boden_count; schicht++) {

	grund_t *gr = plan->gib_boden_bei(schicht);
	gr->calc_bild();
      }
    }
  }
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
	const int groesse = welt->gib_groesse();
	for(int j=0; j<groesse; j++) {
		for(int i=0; i<groesse; i++) {

			const planquadrat_t *plan = welt->lookup(koord(i,j));
			const int boden_count = plan->gib_boden_count();

			for(int schicht=0; schicht<boden_count; schicht++) {

				grund_t *gr = plan->gib_boden_bei(schicht);
				gr->calc_bild();
			}
		}
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


void
boden_t::zeige_info()
{
    if(gib_halt().is_bound()) {
        gib_halt()->zeige_info();
    } else {
	if(!grund_infos->get(this)) {
	    grund_infos->put(this, new grund_info_t(welt, this));
	}
        create_win(-1, -1, grund_infos->get(this), w_autodelete);
    }
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
		setze_bild(grund_besch_t::boden->gib_bild(hang + offset));
	}

	int back_bild = -1;
	const koord k = gib_pos().gib_2d();

	const planquadrat_t *left  = welt->lookup(k - koord(1,0));
	const planquadrat_t *right = welt->lookup(k - koord(0,1));
	const int height = gib_pos().z;

	if(left && right) {
		grund_t * lgr = left->gib_kartenboden();
		grund_t * rgr = right->gib_kartenboden();

		const int lhdiff = lgr->ist_wasser() ? -1 : lgr->gib_hoehe() - height;
		const int lrdiff = rgr->ist_wasser() ? -1 : rgr->gib_hoehe() - height;

		const uint8 slope_this =  welt->get_slope(k);
		const uint8 slope_left =  welt->get_slope(k - koord(1,0));
		const uint8 slope_right = welt->get_slope(k - koord(0,1));

		uint8 idl = ((slope_left & 4) >> 1) + ((slope_left & 2) >> 1);
		uint8 idr = ((slope_right & 1) << 1) + ((slope_right & 2) >> 1);

		// Hajo: cases for height difference
		if(lhdiff > 0) {
			idl = 3;
		} else if(lhdiff < 0) {
			idl = 0;
		}

		if(lrdiff > 0) {
			idr = 3;
		} else if(lrdiff < 0) {
			idr = 0;
		}


		if(slope_this != 0 &&
			slope_this != 3 && slope_this != 6 &&
			slope_this != 9 && slope_this != 12) {
			idl = idr = 0;
		}

		const uint8 both = (idl << 2) + idr;
		if(both != 0) {
			// printf("idl=%d idr=%d both=%x dslope=%d\n", idl, idr, both, double_slope_table[both]);
			back_bild = grund_besch_t::boden->gib_bild(38 + double_slope_table[both]);
		}
	}
	setze_back_bild(back_bild);
}


void * boden_t::operator new(size_t /*s*/)
{
    return mempool->alloc();
}


void boden_t::operator delete(void *p)
{
    mempool->free( p );
}
