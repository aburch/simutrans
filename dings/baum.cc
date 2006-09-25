/*
 * baum.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simplay.h"
#include "../simmem.h"
#include "../simtools.h"

#include "../boden/grund.h"

#include "../besch/baum_besch.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/freelist.h"


#include "baum.h"

static const int baum_bild_alter[12] =
{
    0,1,2,3,3,3,3,3,3,4,4,4
};

bool baum_t::hide = false;


/*************************** first the static function for the baum_t and baum_besch_t administration ***************/


/*
 * Diese Tabelle ermöglicht das Auffinden dient zur Auswahl eines Baumtypen
 */
slist_tpl<const baum_besch_t *> baum_t::baum_typen;

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<const baum_besch_t *> baum_t::besch_names;

// total number of trees
// the same for a certain climate
int baum_t::gib_anzahl_besch(climate cl)
{
	uint16 total_number=0;
	slist_iterator_tpl<const baum_besch_t *> iter(baum_typen);
	while(iter.next()) {
		if(iter.get_current()->is_allowed_climate(cl)) {
			total_number ++;
		}
	}
	return total_number;
}



/**
 * tree planting function - it takes care of checking suitability of area
 */
bool baum_t::plant_tree_on_coordinate(karte_t * welt, koord pos, const uint8 maximum_count)
{
	const planquadrat_t *plan = welt->lookup(pos);
	if(plan) {
		grund_t * gr = plan->gib_kartenboden();
		if(gr!=NULL  &&
			gib_anzahl_besch(welt->get_climate(gr->gib_hoehe()))>0  &&
			gr->ist_natur()  &&
			gr->obj_count()<maximum_count)
		{
			gr->obj_add( new baum_t(welt, gr->gib_pos()) ); //plants the tree
			return true; //tree was planted - currently unused value is not checked
		}
	}
	return false;
}



bool
baum_t::alles_geladen()
{
	if(besch_names.count() == 0) {
		DBG_MESSAGE("baum_t", "No trees found - feature disabled");
	}
	return true;
}



bool
baum_t::register_besch(baum_besch_t *besch)
{
	baum_typen.append(besch);
	besch_names.put(besch->gib_name(), const_cast<baum_besch_t *>(besch));
	return true;
}



// calculates tree position on a tile
// takes care of slopes
void
baum_t::calc_off()
{
	int liob;
	int reob;
	switch (welt->lookup( gib_pos().gib_2d())->gib_kartenboden()->gib_grund_hang() ) {
		case 0:
			liob=simrand(30)-15;
			reob=simrand(30)-15;
			setze_xoff( reob - liob  );
			setze_yoff( (reob + liob)/2 );
			break;

		case 1:
		case 4:
		case 5:
		case 8:
		case 9:
		case 12:
		case 13:
			liob=simrand(16)-8;
			reob=simrand(16)-8;
			setze_xoff( reob - liob  );
			setze_yoff( reob + liob );
			break;

		case 2:
		case 3:
		case 6:
		case 7:
		case 10:
		case 11:
		case 14:
		case 15:
			liob=simrand(16)-8;
			reob=simrand(16)-8;
			setze_xoff( reob + liob  );
			setze_yoff(-10-(reob - liob)/2 );
			break;
	}
}



bool
baum_t::calc_bild(const unsigned long alter)
{
	// alter/2048 is the age of the tree
	int baum_alter = baum_bild_alter[min(alter>>6, 11u)];
	const int seasons = besch->gib_seasons();
	int season=0;

	if(seasons>1) {
		// two possibilities
		if(besch->gib_seasons()==2) {
			// only summer and winter
			season = welt->get_snowline()<=gib_pos().z-gib_yoff();
		}
		else {
			// summer autumn winter spring
			season = welt->gib_jahreszeit();
			if(welt->get_snowline()<=gib_pos().z) {
				// change to winter
				season = 2;
			}
			else if(welt->get_snowline()<=gib_pos().z+16  &&  season==0) {
				// snowline crossing in summer
				// so at least some weeks spring/autumn
				season = welt->get_last_month() <=5 ? 3 : 1;
			}
		}
	}

	const image_id bild_neu = besch->gib_bild(season, baum_alter )->gib_nummer();
	if(bild_neu!=gib_bild()) {
		setze_bild(0, bild_neu);
		return true;
	}
	return false;
}



inline void
baum_t::calc_bild()
{
    calc_bild(welt->get_current_month() - geburt);
}



/* also checks for distribution values
 * @author prissi
 */
const baum_besch_t *
baum_t::gib_aus_liste(climate cl)
{
	slist_tpl<const baum_besch_t *> auswahl;
	slist_iterator_tpl<const baum_besch_t *>  iter(baum_typen);
	int weight = 0;

	while(iter.next()) {
		if(iter.get_current()->is_allowed_climate(cl)) {
			auswahl.append(iter.get_current());
			weight += iter.get_current()->gib_distribution_weight();
		}
	}
	// now weight their distribution
	if( auswahl.count()>0  &&  weight>0) {
		const int w=simrand(weight);
		weight = 0;
		for( unsigned i=0; i<auswahl.count();  i++  ) {
			weight += auswahl.at(i)->gib_distribution_weight();
			if(weight>w) {
				return auswahl.at(i);
			}
		}
	}
	return NULL;
}



baum_t::baum_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	besch = NULL;
	rdwr(file);
	if(gib_besch()) {
		calc_bild();
	}
	// no steps, new month will do this for us
	step_frequency = 0;
}



baum_t::baum_t(karte_t *welt, koord3d pos) : ding_t(welt, pos)
{
	// Hajo: auch aeltere Baeume erzeugen
	geburt = welt->get_current_month() - simrand(400);

	const grund_t *gr = welt->lookup(pos);
	if(gr) {
		besch = gib_aus_liste(welt->get_climate(pos.z));
	}

	assert(besch);

	calc_off();
	calc_bild();

	// no steps, new month will do this for us
	step_frequency = 0;
}



baum_t::baum_t(karte_t *welt, koord3d pos, const baum_besch_t *besch) : ding_t(welt, pos)
{
	geburt = welt->get_current_month();

	this->besch = besch;
	calc_off();
	calc_bild();

	// no steps, new month will do this for us
	step_frequency = 0;
}



void
baum_t::saee_baum()
{
	// spawn a new tree in an area 5x5 tiles around
	// the area for normal new tree planting is slightly more restricted, square of 9x9 was too much
	const koord k = gib_pos().gib_2d() + koord(simrand(5)-2, simrand(5)-2);

	if(welt->lookup(k)) {
		grund_t *bd = welt->lookup(k)->gib_kartenboden();
		if(	bd!=NULL  &&
			besch->is_allowed_climate(welt->get_climate(bd->gib_hoehe()))  &&
			bd->ist_natur()  &&
			bd->obj_count()<welt->gib_max_no_of_trees_on_square())
		{
			bd->obj_add( new baum_t(welt, bd->gib_pos(), besch) );
		}
	}
}



/* we should be as fast as possible for this, because trees are nearly the most common object on a map */
bool
baum_t::check_season(long month)
{
	// take care of birth/death and seasons
	const long alter = (month - geburt);
	calc_bild(alter);
	if(alter==512) {
		// only in this month a tree can span new trees
		// only 1-3 trees will be planted....
		const uint8 c_plant_tree_max = 1+simrand(3);
		for(uint8 c_temp=0 ;  c_temp<c_plant_tree_max;  c_temp++ ) {
			saee_baum();
		}
		// we make the tree a month older to avoid second spawning
		geburt = geburt-1;
	}
	// tree will die after 704 month (i.e. 58 years 8 month)
	return alter<704;
}



void
baum_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	long alter = (welt->get_current_month() - geburt)<<18;
	file->rdwr_long(alter, "\n");

	// after loading, calculate new
	geburt = welt->get_current_month() - (alter>>18);

	if(file->is_saving()) {
		const char *s = besch->gib_name();
		file->rdwr_str(s, "N");
	}

	if(file->is_loading()) {
		char bname[128];
		file->rd_str_into(bname, "N");

		besch = (const baum_besch_t *)besch_names.get(bname);
		if(!besch) {
			// replace with random tree
			if(baum_typen.count()>0) {
				besch = baum_typen.at(simrand(baum_typen.count()));
			}
		}
	}
}



/**
 * Öffnet ein neues Beobachtungsfenster für das Objekt.
 * @author Hj. Malthaner
 */
void baum_t::zeige_info()
{
	if(umgebung_t::tree_info) {
		ding_t::zeige_info();
	}
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void baum_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);

	buf.append("\n");
	buf.append(translator::translate(besch->gib_name()));
	buf.append("\n");
	buf.append(welt->get_current_month() - geburt);
	buf.append(" ");
	buf.append(translator::translate("Monate alt"));
}



void
baum_t::entferne(spieler_t *sp)
{
	if(sp != NULL) {
		sp->buche(umgebung_t::cst_remove_tree, gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
}



void *
baum_t::operator new(size_t /*s*/)
{
	return (baum_t *)freelist_t::gimme_node(sizeof(baum_t));
}



void
baum_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(baum_t),p);
}
