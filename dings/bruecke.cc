/*
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../boden/grund.h"
#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
#include "../boden/grund.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simmem.h"
#include "../bauer/brueckenbauer.h"
#include "../bauer/wegbauer.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "bruecke.h"



bruecke_t::bruecke_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	rdwr(file);
}



bruecke_t::bruecke_t(karte_t *welt, koord3d pos, spieler_t *sp,
		     const bruecke_besch_t *besch, bruecke_besch_t::img_t img) :
 ding_t(welt, pos)
{
	this->besch = besch;
	this->img = img;
	setze_besitzer( sp );
	spieler_t::accounting( gib_besitzer(), -besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
}



void bruecke_t::calc_bild()
{
	grund_t *gr=welt->lookup(gib_pos());
	if(gr) {
		// if we are on the bridge, put the image into the ground, so we can have two ways ...
		if(gr->gib_weg_nr(0)) {
			if(img>=bruecke_besch_t::N_Start  &&  img<=bruecke_besch_t::W_Start) {
				// must take the upper value for the start of the bridge
				gr->gib_weg_nr(0)->setze_bild(besch->gib_hintergrund(img, gib_pos().z+Z_TILE_STEP >= welt->get_snowline()));
			}
			else {
				gr->gib_weg_nr(0)->setze_bild(besch->gib_hintergrund(img, gib_pos().z >= welt->get_snowline()));
			}
		}
		setze_yoff( -gr->gib_weg_yoff() );
	}
}



void bruecke_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	const char *s = NULL;

	if(file->is_saving()) {
		s = besch->gib_name();
	}
	file->rdwr_str(s, "");
	file->rdwr_enum(img, "");

	if(file->is_loading()) {
		besch = brueckenbauer_t::gib_besch(s);
		if(besch==NULL) {
			besch = brueckenbauer_t::gib_besch(translator::compatibility_name(s));
		}
		if(besch==NULL) {
			dbg->warning( "bruecke_t::rdwr()", "unknown bridge \"%s\" at (%i,%i) will be replaced with best match!", s, gib_pos().x, gib_pos().y );
		}
		guarded_free(const_cast<char *>(s));
	}
}



// correct speed and maitainace
void bruecke_t::laden_abschliessen()
{
	grund_t *gr = welt->lookup(gib_pos());
	if(besch==NULL) {
		weg_t *weg = gr->gib_weg_nr(0);
		if(weg) {
			besch = brueckenbauer_t::find_bridge(weg->gib_waytype(),weg->gib_max_speed(),0);
		}
		if(besch==NULL) {
			dbg->fatal("bruecke_t::rdwr()","Unknown bridge type at (%i,%i)", gib_pos().x, gib_pos().y );
		}
	}

	spieler_t *sp=gib_besitzer();
	if(sp) {
		// change maintainance
		if(besch->gib_waytype()!=powerline_wt) {
			weg_t *weg = gr->gib_weg(besch->gib_waytype());
			if(weg==NULL) {
				dbg->error("bruecke_t::laden_abschliessen()","Bridge without way at(%s)!", gr->gib_pos().gib_str() );
				weg = weg_t::alloc( besch->gib_waytype() );
				gr->neuen_weg_bauen( weg, 0, welt->gib_spieler(1) );
			}
			weg->setze_max_speed(besch->gib_topspeed());
			weg->setze_besitzer(sp);
			spieler_t::add_maintenance( sp, -weg->gib_besch()->gib_wartung());
		}
		spieler_t::add_maintenance( sp,  besch->gib_wartung() );
	}
}



// correct speed and maitainace
void bruecke_t::entferne( spieler_t *sp2 )
{
	spieler_t *sp = gib_besitzer();
	if(sp) {
		// on bridge => do nothing but change maintainance
		const grund_t *gr = welt->lookup(gib_pos());
		if(gr) {
			weg_t *weg = gr->gib_weg( besch->gib_waytype() );
			if(weg) {
				weg->setze_max_speed( weg->gib_besch()->gib_topspeed() );
				spieler_t::add_maintenance( sp,  weg->gib_besch()->gib_wartung());
			}
		}
		spieler_t::add_maintenance( sp,  -besch->gib_wartung() );
	}
	spieler_t::accounting( sp2, -besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION );
}



// rotated segment names
static bruecke_besch_t::img_t rotate90_img[12]= {
	bruecke_besch_t::OW_Segment, bruecke_besch_t::NS_Segment,
	bruecke_besch_t::O_Start, bruecke_besch_t::W_Start, bruecke_besch_t::S_Start, bruecke_besch_t::N_Start,
	bruecke_besch_t::O_Rampe, bruecke_besch_t::W_Rampe, bruecke_besch_t::S_Rampe, bruecke_besch_t::N_Rampe,
	bruecke_besch_t::OW_Pillar, bruecke_besch_t::NS_Pillar
};

void bruecke_t::rotate90()
{
	setze_yoff(0);
	ding_t::rotate90();
	// the rotated image parameter is just one in front/back
	img = rotate90_img[img];
}
