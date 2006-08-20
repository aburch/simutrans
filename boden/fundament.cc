/* fundament.cc
 *
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

#include "../dataobj/freelist.h"

#include "grund.h"
#include "fundament.h"


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

fundament_t::fundament_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
	setze_bild( -1 );
	setze_back_bild( -1 );
	rdwr(file);
}

fundament_t::fundament_t(karte_t *welt, koord3d pos) : grund_t(welt, pos)
{
	setze_bild( -1 );
	setze_back_bild( -1 );
}


/**
 * Auffforderung, ein Infofenster zu öffnen.
 * @author Hj. Malthaner
 */
bool fundament_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
		return true;
	}
	return false;
}


void
fundament_t::calc_bild()
{
	grund_t::calc_bild();
#ifndef DOUBLE_GROUNDS
	const koord k = gib_pos().gib_2d();
	const uint8 slope_this = welt->get_slope(k);

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

	setze_bild( grund_besch_t::fundament->gib_bild(welt->get_slope(k)) );
	setze_back_bild(back_bild);
#endif
}


void * fundament_t::operator new(size_t /*s*/)
{
	return (fundament_t *)freelist_t::gimme_node(sizeof(fundament_t));
}


void fundament_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(fundament_t),p);
}
