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
	rdwr(file);
}

fundament_t::fundament_t(karte_t *welt, koord3d pos) : grund_t(welt, pos)
{
	setze_bild( -1 );
}


/**
 * Auffforderung, ein Infofenster zu öffnen.
 * @author Hj. Malthaner
 */
void fundament_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
	}
}


void
fundament_t::calc_bild()
{
	const koord k = gib_pos().gib_2d();
	const uint8 slope_this =  welt->get_slope(k);
	const uint8 natural_slope_this = welt->calc_natural_slope(k);

	grund_t::calc_bild();

	int back_bild = IMG_LEER;

	const planquadrat_t *left  = welt->lookup(k - koord(1,0));
	const planquadrat_t *right = welt->lookup(k - koord(0,1));

	if(left!=NULL  && right!=NULL) {
		grund_t * lgr = left->gib_kartenboden();
		grund_t * rgr = right->gib_kartenboden();

		const uint8 slope_left =  welt->get_slope(k - koord(1,0));
		const uint8 slope_right = welt->get_slope(k - koord(0,1));

		const int lhdiff = lgr->ist_wasser() ? -1 : lgr->gib_hoehe() - gib_hoehe();
		const int lrdiff = rgr->ist_wasser() ? -1 : rgr->gib_hoehe() - gib_hoehe();

		int slope_wall = 0;

		if(lhdiff>=0) {
			// add corners of left slope
			sint8 llhdiff = ((slope_left&2)!=0) - ((slope_this&1)!=0) + lhdiff;
			sint8 lhhdiff = ((slope_left&4)!=0) - ((slope_this&8)!=0) + lhdiff;
			slope_wall = (llhdiff>0)*4 + (lhhdiff>0)*8;
			// only down-slopes in table
			// thus upslopes are converted to solid walls
			if(lhdiff>0  &&  slope_wall!=0) {
				slope_wall = 12;
			}
		}

		if(lrdiff>=0) {
			// add corner of right slope
			sint8 hlhdiff = ((slope_right&1)!=0) - ((slope_this&8)!=0) + lrdiff;
			sint8 hhhdiff = ((slope_right&2)!=0) - ((slope_this&4)!=0) + lrdiff;
			slope_wall |= (hlhdiff>0)*2 + (hhhdiff>0)*1;
			// only down-slopes in table
			// thus upslopes are converted to solid walls
			if(lrdiff>0  &&  (slope_wall&3)>0) {
				slope_wall |= 3;
			}
		}

		if(slope_wall!=0) {
				back_bild = grund_besch_t::boden->gib_bild(38 + double_slope_table[slope_wall]);
		}
	}

	setze_back_bild(back_bild);

	setze_bild( -1 );
}


void * fundament_t::operator new(size_t /*s*/)
{
	return (fundament_t *)freelist_t::gimme_node(sizeof(fundament_t));
}


void fundament_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(fundament_t),p);
}
