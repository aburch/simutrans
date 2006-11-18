/*
 * fabrikbauer.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simdebug.h"
#include "fabrikbauer.h"
#include "../simworld.h"
#include "../simintr.h"
#include "../simfab.h"
#include "../simdisplay.h"
#include "../simgraph.h"
#include "../simtools.h"
#include "../simcity.h"
#include "../simskin.h"
#include "../simhalt.h"
#include "../simplay.h"

#include "../dings/leitung2.h"

#include "../boden/grund.h"
//#include "../boden/wege/dock.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/translator.h"

#include "../sucher/bauplatz_sucher.h"

#include "../gui/karte.h"	// to update map after construction of new industry

// Hajo: average industry distance
#define DISTANCE 50


/**
 * bauplatz_mit_strasse_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class factory_bauplatz_mit_strasse_sucher_t: public bauplatz_sucher_t  {

public:
	factory_bauplatz_mit_strasse_sucher_t(karte_t *welt) : bauplatz_sucher_t (welt) {}

	// get distance to next factory
	int find_dist_next_factory(koord pos) const {
		const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
		slist_iterator_tpl <fabrik_t *> iter (list);
		int dist = welt->gib_groesse_x()+welt->gib_groesse_y();
		while(iter.next()) {
			fabrik_t * fab = iter.get_current();
			int d = koord_distance(fab->gib_pos(),pos);
			if(d<dist) {
				dist = d;
			}
		}
//DBG_DEBUG("bauplatz_mit_strasse_sucher_t::find_dist_next_factory()","returns %i",dist);
		return dist;
	}

	bool strasse_bei(sint16 x, sint16 y) const {
		grund_t *bd = welt->lookup(koord(x, y))->gib_kartenboden();
		return bd && bd->hat_weg(road_wt);
	}

	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const {
		if(bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)) {
			// try to built a little away from previous factory
			if(find_dist_next_factory(pos)<3+b+h) {
				return false;
			}
			int i, j;
			// check to not built on a road
			for(j=pos.x; j<pos.x+b; j++) {
				for(i=pos.y; i<pos.y+h; i++) {
					if(strasse_bei(j,i)) {
						return false;
					}
				}
			}
			// now check for road connection
			for(i = pos.y; i < pos.y + h; i++) {
				if(strasse_bei(pos.x - 1, i) ||  strasse_bei(pos.x + b, i)) {
					return true;
				}
			}
			for(i = pos.x; i < pos.x + b; i++) {
				if(strasse_bei(i, pos.y - 1) ||  strasse_bei(i, pos.y + h)) {
					return true;
				}
			}
		}
		return false;
	}
};


stringhashtable_tpl<const fabrik_besch_t *> fabrikbauer_t::table;

slist_tpl <fabrikbauer_t::stadt_fabrik_t> fabrikbauer_t::gebaut;


void fabrikbauer_t::bau_info_t::random(slist_tpl <const fabrik_besch_t *> &fab)
{
	slist_iterator_tpl<const fabrik_besch_t *> iter(fab);
	int gewichtung = 0;
	int next;

	while(iter.next()) {
		gewichtung += iter.get_current()->gib_gewichtung();
	}
	if(gewichtung > 0) {
		next = simrand(gewichtung);
		iter.begin();
		while(iter.next()) {
			if(next < iter.get_current()->gib_gewichtung()) {
				info = iter.get_current();
				besch = hausbauer_t::finde_fabrik(info->gib_name());

				if(besch == 0) {
dbg->fatal("fabrikbauer_t::bau_info_t::random","No description found for '%s'",info->gib_name());
				}
				break;
			}
			next -= iter.get_current()->gib_gewichtung();
		}
		rotate = simrand(besch->gib_all_layouts()-1);
		dim = besch->gib_groesse(rotate);
	}
	else {
		besch = NULL;
		info = NULL;
		dim = koord(1,1);
		rotate = 0;
	}
}



/* returns a random consumer
 * @author prissi
 */
const fabrik_besch_t *fabrikbauer_t::get_random_consumer(bool in_city,climate_bits cl)
{
	// get a random city factory
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);
	slist_tpl <const fabrik_besch_t *> consumer;
	int gewichtung=0;

	while(iter.next()) {
		const fabrik_besch_t *current=iter.get_current_value();
		// nur endverbraucher eintragen
		if(current->gib_produkt(0)==NULL  &&  current->gib_haus()->is_allowed_climate_bits(cl)  &&
			((in_city  &&  current->gib_platzierung() == fabrik_besch_t::Stadt)
			||  (!in_city  &&  current->gib_platzierung() == fabrik_besch_t::Land)  )  ) {
			consumer.insert(current);
			gewichtung += current->gib_gewichtung();
		}
	}
	// no consumer installed?
	if(consumer.count()==0) {
DBG_MESSAGE("fabrikbauer_t::get_random_consumer()","No suitable consumer found");
		return NULL;
	}
	// now find a random one
	int next=simrand(gewichtung);
	for( unsigned i=1;  i<consumer.count();  i++  ) {
		if(next < consumer.at(i)->gib_gewichtung()) {
DBG_MESSAGE("fabrikbauer_t::get_random_consumer()","consumer %s found.",consumer.at(i)->gib_name());
			return consumer.at(i);
		}
		next -= consumer.at(i)->gib_gewichtung();
	}
DBG_MESSAGE("fabrikbauer_t::get_random_consumer()","consumer %s found.",consumer.at(0)->gib_name());
	return consumer.at(0);
}



const fabrik_besch_t *fabrikbauer_t::gib_fabesch(const char *fabtype)
{
    return table.get(fabtype);
}



void fabrikbauer_t::register_besch(fabrik_besch_t *besch)
{
 	uint16 p=besch->gib_produktivitaet();
 	if(p&0x8000) {
 		koord k=besch->gib_haus()->gib_groesse();
 		// to be compatible with old factories, since new code only steps once per factory, not per tile
 		besch->setze_produktivitaet( (p&0x7FFF)*k.x*k.y );
DBG_DEBUG("fabrikbauer_t::register_besch()","Correction for old factory: Increase poduction from %i by %i",p&0x7FFF,k.x*k.y);
 	}
	table.put(besch->gib_name(), besch);
}



int
fabrikbauer_t::finde_anzahl_hersteller(const ware_besch_t *ware)
{
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);
	int anzahl=0;

	while(iter.next()) {
		const fabrik_besch_t *tmp = iter.get_current_value();

		for(int i = 0; i<tmp->gib_produkte(); i++) {
			const fabrik_produkt_besch_t *produkt = tmp->gib_produkt(i);
			if(produkt->gib_ware()==ware  &&  tmp->gib_gewichtung()>0) {
				anzahl ++;
			}
		}
	}
DBG_MESSAGE("fabrikbauer_t::finde_anzahl_hersteller()","%i producer for good '%s' fount.", anzahl, translator::translate(ware->gib_name()));
	return anzahl;
}



/* finds a producer;
 * also water only producer are allowed
 */
const fabrik_besch_t *
fabrikbauer_t::finde_hersteller(const ware_besch_t *ware,int /*nr*/)
{
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);
	slist_tpl <const fabrik_besch_t *> producer;
	int gewichtung=0;

	while(iter.next()) {
		const fabrik_besch_t *tmp = iter.get_current_value();

		for(int i = 0; i<tmp->gib_produkte(); i++) {
			const fabrik_produkt_besch_t *produkt = tmp->gib_produkt(i);
			if(produkt->gib_ware()==ware  &&  tmp->gib_gewichtung()>0) {
				producer.insert(tmp);
				gewichtung += tmp->gib_gewichtung();
				break;
			}
		}
	}

	// no producer installed?
	if(producer.count()==0) {
		dbg->error("fabrikbauer_t::finde_hersteller()","no producer for good '%s' was found", translator::translate(ware->gib_name()));
		return NULL;
	}
	// now find a random one
	int next=simrand(gewichtung);
	const fabrik_besch_t *besch=NULL;
DBG_MESSAGE("fabrikbauer_t::finde_hersteller()","good '%s' weight=% of total %i", translator::translate(ware->gib_name()),next,gewichtung);
	for( unsigned i=0;  i<producer.count();  i++  ) {
		besch = producer.at(i);
		next -= besch->gib_gewichtung();
DBG_MESSAGE("fabrikbauer_t::finde_hersteller()","%i) %s (weight %i)", i, besch->gib_name(),besch->gib_gewichtung());
		if(next<0) {
			break;
		}
	}
DBG_MESSAGE("fabrikbauer_t::finde_hersteller()","producer for good '%s' was found %s", translator::translate(ware->gib_name()),besch->gib_name());
	return besch;
}




koord3d
fabrikbauer_t::finde_zufallsbauplatz(karte_t * welt, const koord3d pos, const int radius, koord groesse,bool wasser,const haus_besch_t *besch)
{
	array_tpl<koord3d> list(4*radius*radius);
	int index = 0;
	koord k;

	if(wasser) {
		groesse += koord(6,6);
	}
	// assert(radius <= 32);

	for(k.y=pos.y-radius; k.y<=pos.y+radius; k.y++) {
		for(k.x=pos.x-radius; k.x<=pos.x+radius; k.x++) {
			// climate check
			if(fabrik_t::ist_bauplatz(welt, k, groesse,wasser,besch->get_allowed_climate_bits())) {
				list.at(index ++) = welt->lookup(k)->gib_kartenboden()->gib_pos();
				// nicht gleich daneben nochmal suchen
				k.x += 4;
			}
		}
	}

	// printf("Zufallsbauplatzindex %d\n", index);
	if(index == 0) {
		return koord3d(-1, -1, -1);
	}
	else {
		if(wasser) {
			// take care of offset
			return list.at(simrand(index))+koord3d(3,3,0);
		}
		return list.at(simrand(index));
	}
}



/* Create a certain numer of tourist attractions
 * @author prissi
 */
void
fabrikbauer_t::verteile_tourist(karte_t * welt, spieler_t *, int max_number)
{
	// current count
	int current_number=0;

	// select without timeline (otherwise no old attractions appear)
	if(hausbauer_t::waehle_sehenswuerdigkeit(0,temperate_climate)==NULL) {
		// nothing at all?
		return;
	}

	// very fast, so we do not bother updating progress bar
	print("Distributing %i tourist attractions ...\n",max_number);fflush(NULL);

	int retrys = max_number*4;
	while(current_number<max_number  &&  retrys-->0) {
		koord3d	pos=koord3d(simrand(welt->gib_groesse_x()),simrand(welt->gib_groesse_y()),1);
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit(0,(climate)simrand((int)arctic_climate+1));

		if(attraction==NULL) {
			continue;
		}

		int	rotation=simrand(attraction->gib_all_layouts()-1);
		pos = finde_zufallsbauplatz(welt, pos, 20, attraction->gib_groesse(rotation),false,attraction);	// so far -> land only
		if(welt->lookup(pos)) {
			// Platz gefunden ...
			hausbauer_t::baue(welt, welt->gib_spieler(1), pos, rotation, attraction);
			current_number ++;
			retrys = max_number*4;
		}

	}
	// update an open map
	reliefkarte_t::gib_karte()->calc_map();
}



/* Create a certain numer of industries
 * @author prissi
 */
void
fabrikbauer_t::verteile_industrie(karte_t * welt, spieler_t *, int max_number_of_factories,bool in_city)
{
	// stuff for the progress bar
	const int display_offset = 16 + welt->gib_einstellungen()->gib_anzahl_staedte()*4 + (in_city? welt->gib_einstellungen()->gib_land_industry_chains() : 0);
	const int display_total = 16 + welt->gib_einstellungen()->gib_anzahl_staedte()*4 + welt->gib_einstellungen()->gib_land_industry_chains() + welt->gib_einstellungen()->gib_city_industry_chains();
	// current count
	int factory_number=0;
	int current_number=0;

	// no consumer at all?
	if(get_random_consumer(in_city,ALL_CLIMATES)==NULL) {
		return;
	}

//	max_number_of_factories = (max_number_of_factories*welt->gib_groesse()*welt->gib_groesse())/(1024*1024);
	print("Distributing about %i industries ...\n",max_number_of_factories);fflush(NULL);

	int retrys = max_number_of_factories*4;
	while(current_number<max_number_of_factories  &&  retrys-->0) {
		koord3d	pos=koord3d(simrand(welt->gib_groesse_x()),simrand(welt->gib_groesse_y()),1);
		const fabrik_besch_t *fab=get_random_consumer(in_city,(climate_bits)(1<<welt->get_climate(welt->lookup(pos.gib_2d())->gib_kartenboden()->gib_hoehe())));
		if(fab) {
			int	rotation=simrand(fab->gib_haus()->gib_all_layouts()-1);

			pos = finde_zufallsbauplatz(welt, pos, 20, fab->gib_haus()->gib_groesse(rotation),fab->gib_platzierung()==fabrik_besch_t::Wasser,fab->gib_haus());
			if(welt->lookup(pos)) {
				// Platz gefunden ...
				factory_number += baue_hierarchie(welt, NULL, fab, rotation, &pos, welt->gib_spieler(1));
				current_number ++;
				retrys = max_number_of_factories*4;
			}

		      if(is_display_init()) {
			    display_progress(display_offset + current_number, display_total);
			    display_flush(IMG_LEER,0, 0, 0, "", "", 0, 0);
			}
		}
	}
	print("Constructed %i industries ...\n",factory_number);
	// update an open map
	reliefkarte_t::gib_karte()->calc_map();
}



fabrik_t *
fabrikbauer_t::baue_fabrik(karte_t * welt, koord3d *parent, const fabrik_besch_t *info, int rotate, koord3d pos, spieler_t *spieler)
{
	fabrik_t * fab = new fabrik_t(welt, pos, spieler, info);
	int i;

	vector_tpl<ware_t> *eingang = new vector_tpl<ware_t> (info->gib_lieferanten());
	vector_tpl<ware_t> *ausgang = new vector_tpl<ware_t> (info->gib_produkte());

	// create producer information
	for(i=0; i < info->gib_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = info->gib_lieferant(i);
		ware_t ware(lieferant->gib_ware());
		ware.max = lieferant->gib_kapazitaet() << fabrik_t::precision_bits;
		eingang->append(ware);
	}
	fab->set_eingang( eingang );

	// create consumer information
	for(i=0; i < info->gib_produkte(); i++) {
		const fabrik_produkt_besch_t *produkt = info->gib_produkt(i);
		ware_t ware(produkt->gib_ware());

		ware.max = produkt->gib_kapazitaet() << fabrik_t::precision_bits;
		if(info->gib_lieferanten()==0) {
			// if source then start with full storage (thus AI will built immeadiately lines)
			// @author prissi
			ware.menge = ware.max-(16<<fabrik_t::precision_bits);
		}
		ausgang->append(ware);
	}
	fab->set_ausgang( ausgang );

	// set production
	fab->set_prodfaktor( 16 );
	fab->set_prodbase( info->gib_produktivitaet() +  simrand(info->gib_bereich()) );
	if(parent) {
		fab->add_lieferziel(parent->gib_2d());
	}

	// now built factory
	fab->baue(rotate, true);
	welt->add_fab(fab);

	// make all water a station
	if(info->gib_platzierung() == fabrik_besch_t::Wasser) {
		const haus_besch_t *besch = hausbauer_t::finde_fabrik(info->gib_name());
		koord dim = besch->gib_groesse(rotate);

		koord k;
		halthandle_t halt = welt->gib_spieler(1)->halt_add(pos.gib_2d());

		if(halt.is_bound()) {

			welt->lookup(pos)->setze_text( translator::translate(info->gib_name()) );

			for(k.x=pos.x; k.x<pos.x+dim.x; k.x++) {
				for(k.y=pos.y; k.y<pos.y+dim.y; k.y++) {
					if(welt->ist_in_kartengrenzen(k)) {
						// add all water to station
						grund_t *gr = welt->lookup(k)->gib_kartenboden();
						if(gr->ist_wasser() && gr->hat_weg(water_wt) == 0) {
	//						gr->neuen_weg_bauen(new dock_t(welt), ribi_t::alle, welt->gib_spieler(0));
							halt->add_grund( gr );
						}
					}
				}
			}
			// is in water
			halt->verbinde_fabriken();
			halt->recalc_station_type();
		}
	}
	else {
		// connenct factory to stations
		// search for near stations and connect factory to them
		koord dim = info->gib_haus()->gib_groesse(rotate);
		koord k;

		for(k.x=pos.x; k.x<=pos.x+dim.x; k.x++) {
			for(k.y=pos.y; k.y<=pos.y+dim.y; k.y++) {
				const minivec_tpl<halthandle_t> &halt_list = welt->access(k)->get_haltlist();
				for( unsigned i=0;  i<halt_list.get_count();  i++  ) {
					halt_list[i]->verbinde_fabriken();
				}
			}
		}
	}

	// add passenger to pax>0, (so no sucide diver at the fish swarm)
	if(info->gib_pax_level()>0) {
		const weighted_vector_tpl<stadt_t *> *staedte=welt->gib_staedte();
		for(  unsigned i=0;  i<staedte->get_count();  i++  ) {
			staedte->get(i)->add_factory_arbeiterziel(fab);
		}
	}
	return fab;
}



/**
 * vorbedingung: pos ist für fabrikbau geeignet
 */
int
fabrikbauer_t::baue_hierarchie(karte_t * welt, koord3d *parent, const fabrik_besch_t *info, int rotate, koord3d *pos, spieler_t *sp)
{
	int n = 1;

	if(info==NULL) {
		// no industry found
		return 0;
	}

	if(info->gib_platzierung() == fabrik_besch_t::Stadt  &&  welt->gib_staedte()->get_count()>0) {
		// built consumer (factory) intown:
		stadt_fabrik_t sf;
		koord k=pos->gib_2d();

		koord size=info->gib_haus()->gib_groesse(0);
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Search place for city factory (%i,%i) size.",size.x,size.y);

		sf.stadt = welt->suche_naechste_stadt(k);
		if(sf.stadt==NULL) {
			return 0;
		}

		INT_CHECK( "fabrikbauer 574" );

		//
		// Drei Varianten:
		// A:
		// Ein Bauplatz, möglichst nah am Rathaus mit einer Strasse daneben.
		// Das könnte ein Zeitproblem geben, wenn eine Stadt keine solchen Bauplatz
		// hat und die Suche bis zur nächsten Stadt weiterläuft
		// Ansonsten erscheint mir das am realistischtsten..
		bool	is_rotate=info->gib_haus()->gib_all_layouts()>1;
		k = factory_bauplatz_mit_strasse_sucher_t(welt).suche_platz(sf.stadt->gib_pos(), size.x, size.y,info->gib_haus()->get_allowed_climate_bits(), &is_rotate);
		rotate = is_rotate?1:0;
		DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Construction at (%i,%i).",k.x,k.y);

		INT_CHECK( "fabrikbauer 588" );

		// B:
		// Gefällt mir auch. Die Endfabriken stehen eventuell etwas außerhalb der Stadt
		// aber nicht weit weg.
		//k = finde_zufallsbauplatz(welt, welt->lookup(sf.stadt->gib_pos())->gib_boden()->gib_pos(), 3, land_bau.dim).gib_2d();
		// C:
		// Ein Bauplatz, möglichst nah am Rathaus.
		// Wenn mehrere Endfabriken bei einer Stadt landen, sind die oft hinter
		// einer Reihe Häuser "versteckt", von Strassen abgeschnitten.
		//k = bauplatz_sucher_t(welt).suche_platz(sf.stadt->gib_pos(), land_bau.dim.x, land_bau.dim.y);

		if(k != koord::invalid) {
			*pos = welt->lookup(k)->gib_kartenboden()->gib_pos();
		}
		else
			return 0;
	}

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Construction of %s at (%i,%i).",info->gib_name(),pos->x,pos->y);
	INT_CHECK("fabrikbauer 594");

	const fabrik_t *our_fab=baue_fabrik(welt, parent, info, rotate, *pos, sp);

	INT_CHECK("fabrikbauer 596");

	for(int i=0; i < info->gib_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = info->gib_lieferant(i);
		const ware_besch_t *ware = lieferant->gib_ware();
		const int anzahl_hersteller=finde_anzahl_hersteller(ware);

		// we assume, we need two times the available supply
		int verbrauch=(our_fab->max_produktion()*lieferant->gib_verbrauch()*100)/256;

		int lcount = lieferant->gib_anzahl();
		int lfound = 0;	// number of found producers

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","lieferanten %i, lcount %i (need %i of %s)",info->gib_lieferanten(),lcount,verbrauch,ware->gib_name());

		// Hajo: search if there already is one or two (crossconnect everything if possible)
		const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
		slist_iterator_tpl <fabrik_t *> iter (list);
		bool found = false;

		while( iter.next() &&
				// try to find matching factories for this consumption
				( (lcount==0  &&  lfound<3  &&  verbrauch>0) ||
				// otherwise dont find more than two times new number
				  (lcount>=lfound+1) )
				)
		{
			fabrik_t * fab = iter.get_current();

			// connect to an existing one, if this is an producer
			if(fab->vorrat_an(ware) > -1) {

				// for sources (oil fields, forests ... ) prefer thoses with a smaller distance
				const unsigned distance = koord_distance(fab->gib_pos(),*pos);
				const bool ok = fab->gib_eingang()->get_count()>0  ||  distance < DISTANCE || distance < simrand((welt->gib_groesse_x()*3)/4);

				if(ok  &&  distance>6) {
					found = true;
					fab->add_lieferziel(pos->gib_2d());

					// now guess, how much this factory can supply
					for(int gg=0;gg<fab->gib_besch()->gib_produkte();gg++) {
						if(fab->gib_besch()->gib_produkt(gg)->gib_ware()==ware  &&  fab->gib_lieferziele().get_count()<10) {	// does not make sense to split into more ...
							int produktion=(fab->max_produktion()*fab->gib_besch()->gib_produkt(gg)->gib_faktor())/(fab->gib_lieferziele().get_count()*2);
							// search consumer
							const vector_tpl <koord> & lieferziele = fab->gib_lieferziele();
							const int lieferziel_anzahl=lieferziele.get_count();
							// assume free capacity is just total capacity divided by the number of demander
							verbrauch -= produktion/(lieferziel_anzahl+1);

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","supplier %s can supply approx %i of %s to %i companies",fab->gib_besch()->gib_name(),produktion,ware->gib_name(),lieferziel_anzahl);
							break;
						}
					}
					lfound ++;
				}
			}
		}

		INT_CHECK( "fabrikbauer 670" );

		// this is a source?
		if(lcount!=0) {
			// at least three producers please
			lcount = max(0,lcount-(lfound/2));
		}
		else {
			// Hajo: if none exist, build one
			lcount = (lfound>0)?0:1;
		}

		// try to add all types of factories until demand is satisfied
		for(int j=0;j<20 &&  lcount>0  &&  verbrauch>0;j++) {
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","find lieferant for %s.",info->gib_name());
			const fabrik_besch_t *hersteller = finde_hersteller(ware,j%anzahl_hersteller);
			if(info==hersteller) {
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie()","found myself!");
				return n;
			}

			INT_CHECK( "fabrikbauer 692" );

			int rotate = simrand(hersteller->gib_haus()->gib_all_layouts()-1);
			koord3d k = finde_zufallsbauplatz(welt, *pos, DISTANCE, hersteller->gib_haus()->gib_groesse(rotate),hersteller->gib_platzierung()==fabrik_besch_t::Wasser,hersteller->gib_haus());

			INT_CHECK("fabrikbauer 697");

			if(welt->lookup(k)) {
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Try to built lieferant %s at (%i,%i) r=%i for %s.",hersteller->gib_name(),k.x,k.y,rotate,info->gib_name());
				n += baue_hierarchie(welt, pos, hersteller, rotate, &k, sp);
				lcount --;

				INT_CHECK( "fabrikbauer 702" );

				// now substract current supplier
				const ding_t * dt = welt->lookup(k.gib_2d())->gib_kartenboden()->obj_bei(0);
				if(dt) {
					const fabrik_t *fab = dt->get_fabrik();
					// find our product
					for(int gg=0;gg<fab->gib_besch()->gib_produkte();gg++) {
						if(fab->gib_besch()->gib_produkt(gg)->gib_ware()==ware) {
							int produktion = (fab->max_produktion()*fab->gib_besch()->gib_produkt(gg)->gib_faktor()*100)/256;
							verbrauch -= produktion;
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","new supplier %s can supply approx %i of %s to us",fab->gib_besch()->gib_name(),produktion,ware->gib_name());
							break;
						}
					}
				}
			}
		}
	}
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie()","update karte");

	INT_CHECK( "fabrikbauer 723" );

	// finally
	if(parent==NULL) {
		// update an open map
		reliefkarte_t::gib_karte()->calc_map();

		INT_CHECK( "fabrikbauer 730" );
	}


	return n;
}
