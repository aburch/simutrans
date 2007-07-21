/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <algorithm>
#include "../simdebug.h"
#include "fabrikbauer.h"
#include "hausbauer.h"
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
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

#include "../sucher/bauplatz_sucher.h"

#include "../gui/karte.h"	// to update map after construction of new industry

// radius for checking places for construction
#define DISTANCE 40


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
	if (consumer.empty()) {
DBG_MESSAGE("fabrikbauer_t::get_random_consumer()","No suitable consumer found");
		return NULL;
	}
	// now find a random one
	int next=simrand(gewichtung);
	for (slist_iterator_tpl<const fabrik_besch_t*> i(consumer); i.next();) {
		const fabrik_besch_t* fb = i.get_current();
		if (next < fb->gib_gewichtung()) {
			DBG_MESSAGE("fabrikbauer_t::get_random_consumer()", "consumer %s found.", fb->gib_name());
			return fb;
		}
		next -= fb->gib_gewichtung();
	}
	DBG_MESSAGE("fabrikbauer_t::get_random_consumer()", "consumer %s found.", consumer.front()->gib_name());
	return consumer.front();
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
	if (producer.empty()) {
		dbg->error("fabrikbauer_t::finde_hersteller()","no producer for good '%s' was found", translator::translate(ware->gib_name()));
		return NULL;
	}
	// now find a random one
	int next=simrand(gewichtung);
	const fabrik_besch_t *besch=NULL;
DBG_MESSAGE("fabrikbauer_t::finde_hersteller()","good '%s' weight=% of total %i", translator::translate(ware->gib_name()),next,gewichtung);
	for (slist_iterator_tpl<const fabrik_besch_t*> i(producer); i.next();) {
		besch = i.get_current();
		next -= besch->gib_gewichtung();
		DBG_MESSAGE("fabrikbauer_t::finde_hersteller()", "%s (weight %i)", besch->gib_name(), besch->gib_gewichtung());
		if(next<0) {
			break;
		}
	}
DBG_MESSAGE("fabrikbauer_t::finde_hersteller()","producer for good '%s' was found %s", translator::translate(ware->gib_name()),besch->gib_name());
	return besch;
}




koord3d
fabrikbauer_t::finde_zufallsbauplatz(karte_t * welt, const koord3d pos, const int radius, koord groesse, bool wasser, const haus_besch_t *besch)
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
				list[index++] = welt->lookup(k)->gib_kartenboden()->gib_pos();
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
			return list[simrand(index)] + koord3d(3, 3, 0);
		}
		return list[simrand(index)];
	}
}


/* Create a certain numer of tourist attractions
 * @author prissi
 */
void fabrikbauer_t::verteile_tourist(karte_t* welt, int max_number)
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
void fabrikbauer_t::verteile_industrie(karte_t* welt, int max_number_of_factories, bool in_city)
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
		const fabrik_besch_t *fab=get_random_consumer(in_city,(climate_bits)(1<<welt->get_climate(welt->lookup(pos.gib_2d())->gib_kartenboden()->gib_pos().z)));
		if(fab) {
			int	rotation=simrand(fab->gib_haus()->gib_all_layouts()-1);

			pos = finde_zufallsbauplatz(welt, pos, 20, fab->gib_haus()->gib_groesse(rotation),fab->gib_platzierung()==fabrik_besch_t::Wasser,fab->gib_haus());
			if(welt->lookup(pos)) {
				// Platz gefunden ...
				factory_number += baue_hierarchie(NULL, fab, rotation, &pos, welt->gib_spieler(1));
				current_number ++;
				retrys = max_number_of_factories*4;
			}

		      if(is_display_init()) {
			    display_progress(display_offset + current_number, display_total);
				display_flush(IMG_LEER, 0, "", "", 0, 0);
			}
		}
	}
	print("Constructed %i industries ...\n",factory_number);
	// update an open map
	reliefkarte_t::gib_karte()->calc_map();
}


/**
 * baue fabrik nach Angaben in info
 * @author Hj.Malthaner
 */
static fabrik_t* baue_fabrik(karte_t* welt, koord3d* parent, const fabrik_besch_t* info, int rotate, koord3d pos, spieler_t* spieler)
{
	fabrik_t * fab = new fabrik_t(pos, spieler, info);

	if(parent) {
		fab->add_lieferziel(parent->gib_2d());
	}

	// now built factory
	fab->baue(rotate, true);
	welt->add_fab(fab);

	// make all water station
	if(info->gib_platzierung() == fabrik_besch_t::Wasser) {
		const haus_besch_t *besch = info->gib_haus();
		koord dim = besch->gib_groesse(rotate);

		koord k;
		halthandle_t halt = welt->gib_spieler(1)->halt_add(pos.gib_2d());
		if(halt.is_bound()) {

			for(k.x=pos.x; k.x<pos.x+dim.x; k.x++) {
				for(k.y=pos.y; k.y<pos.y+dim.y; k.y++) {
					if(welt->ist_in_kartengrenzen(k)) {
						// add all water to station
						grund_t *gr = welt->lookup(k)->gib_kartenboden();
						if(gr->ist_wasser() && gr->hat_weg(water_wt) == 0) {
							halt->add_grund( gr );
						}
					}
				}
			}
			halt->setze_name( translator::translate(info->gib_name()) );
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
				const planquadrat_t *plan = welt->lookup(k);
				const halthandle_t *halt_list = plan->get_haltlist();
				for(  unsigned h=0;  h<plan->get_haltlist_count();  h++ ) {
					halt_list[h]->verbinde_fabriken();
				}
			}
		}
	}

	// add passenger to pax>0, (so no sucide diver at the fish swarm)
	if(info->gib_pax_level()>0) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->gib_staedte();
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = staedte.begin(), end = staedte.end(); i != end; ++i) {
			(*i)->add_factory_arbeiterziel(fab);
		}
	}
	return fab;
}


/**
 * vorbedingung: pos ist für fabrikbau geeignet
 */
int fabrikbauer_t::baue_hierarchie(koord3d* parent, const fabrik_besch_t* info, int rotate, koord3d* pos, spieler_t* sp)
{
	karte_t* welt = sp->get_welt();
	int n = 1;

	if(info==NULL) {
		// no industry found
		return 0;
	}

	if (info->gib_platzierung() == fabrik_besch_t::Stadt && !welt->gib_staedte().empty()) {
		// built consumer (factory) intown:
		stadt_fabrik_t sf;
		koord k=pos->gib_2d();

		koord size=info->gib_haus()->gib_groesse(0);
//DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Search place for city factory (%i,%i) size.",size.x,size.y);

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
		k = factory_bauplatz_mit_strasse_sucher_t(welt).suche_platz(sf.stadt->gib_pos(), size.x, size.y, info->gib_haus()->get_allowed_climate_bits(), &is_rotate);
		rotate = is_rotate?1:0;

		INT_CHECK( "fabrikbauer 588" );

		// B:
		// Gefällt mir auch. Die Endfabriken stehen eventuell etwas außerhalb der Stadt
		// aber nicht weit weg.
		// (does not obey climates though!)
		// k = finde_zufallsbauplatz(welt, welt->lookup(sf.stadt->gib_pos())->gib_boden()->gib_pos(), 3, land_bau.dim).gib_2d();

		// C:
		// Ein Bauplatz, möglichst nah am Rathaus.
		// Wenn mehrere Endfabriken bei einer Stadt landen, sind die oft hinter
		// einer Reihe Häuser "versteckt", von Strassen abgeschnitten.
		//k = bauplatz_sucher_t(welt).suche_platz(sf.stadt->gib_pos(), land_bau.dim.x, land_bau.dim.y, info->gib_haus()->get_allowed_climate_bits(), &is_rotate);

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

	/* first we try to connect to existing factories and will do some
	 * vrossconnect (if wanted)
	 * We must ake care, to add capacity for the crossconnected ones!
	 */
	for(int i=0; i<info->gib_lieferanten(); i++) {
		const fabrik_lieferant_besch_t *lieferant = info->gib_lieferant(i);
		const ware_besch_t *ware = lieferant->gib_ware();
		const int anzahl_hersteller=finde_anzahl_hersteller(ware);

		if(anzahl_hersteller==0) {
			dbg->fatal("fabrikbauer_t::baue_hierarchie()","No producer for %s found!",ware->gib_name() );
		}

		// how much we need?
		sint32 verbrauch = our_fab->get_base_production()*lieferant->gib_verbrauch();

		slist_tpl<fabs_to_crossconnect_t> factories_to_correct;
		slist_tpl<fabrik_t *> new_factories;	// since the crosscorrection must be done later
		slist_tpl<fabrik_t *> crossconnected_supplier;	// also done after the construction of new chains

		int lcount = lieferant->gib_anzahl();
		int lfound = 0;	// number of found producers

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","lieferanten %i, lcount %i (need %i of %s)",info->gib_lieferanten(),lcount,verbrauch,ware->gib_name());

		// Hajo: search if there already is one or two (crossconnect everything if possible)
		const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
		slist_iterator_tpl <fabrik_t *> iter (list);
		bool found = false;

		while( iter.next() &&
				// try to find matching factories for this consumption
				( (lcount==0  &&  verbrauch>0) ||
				// but don't find more than two times number of factories requested
				  (lcount>=lfound+1) )
				)
		{
			fabrik_t * fab = iter.get_current();

			// connect to an existing one, if this is an producer
			if(fab->vorrat_an(ware) > -1) {

				// for sources (oil fields, forests ... ) prefer thoses with a smaller distance
				const unsigned distance = koord_distance(fab->gib_pos(),*pos);

				if(distance>6) {//  &&  distance < simrand(welt->gib_groesse_x()+welt->gib_groesse_y())) {
					// ok, this would match
					// but can she supply enough?

					// now guess, how much this factory can supply
					for(int gg=0;gg<fab->gib_besch()->gib_produkte();gg++) {
						if(fab->gib_besch()->gib_produkt(gg)->gib_ware()==ware  &&  fab->gib_lieferziele().get_count()<10) {	// does not make sense to split into more ...
							sint32 production_left = fab->get_base_production()*fab->gib_besch()->gib_produkt(gg)->gib_faktor();
							const vector_tpl <koord> & lieferziele = fab->gib_lieferziele();
							for( unsigned ziel=0;  ziel<lieferziele.get_count()  &&  production_left>0;  ziel++  ) {
								fabrik_t *zfab=fabrik_t::gib_fab(welt,lieferziele[ziel]);
								for(int zz=0;  zz<zfab->gib_besch()->gib_lieferanten();  zz++) {
									if(zfab->gib_besch()->gib_lieferant(zz)->gib_ware()==ware) {
										production_left -= zfab->get_base_production()*zfab->gib_besch()->gib_lieferant(zz)->gib_verbrauch();
										break;
									}
								}
							}
							// here is actually capacity left (or sometimes just connect anyway)!
							if(production_left>0  ||  simrand(100)<umgebung_t::crossconnect_factor) {
								found = true;
								if(production_left>0) {
									verbrauch -= production_left;
									fab->add_lieferziel(pos->gib_2d());
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","supplier %s can supply approx %i of %s to us",fab->gib_besch()->gib_name(),production_left,ware->gib_name());
								}
								else {
									/* we steal something; however the total capacity
									 * needed is the same. Therefore, we just keep book
									 * from whose factories from how many we stole */
									crossconnected_supplier.append(fab);
									for( unsigned ziel=0;  ziel<lieferziele.get_count();  ziel++  ) {
										fabrik_t *zfab=fabrik_t::gib_fab(welt,lieferziele[ziel]);
										slist_tpl<fabs_to_crossconnect_t>::iterator i = std::find(factories_to_correct.begin(), factories_to_correct.end(), fabs_to_crossconnect_t(zfab, 0));
										if (i == factories_to_correct.end()) {
											factories_to_correct.append(fabs_to_crossconnect_t(zfab, 1));
										} else {
											i->demand += 1;
										}
									}
									// the needed produktion to be built does not change!
								}
								lfound ++;
							}
							break; // since we found the product ...
						}
					}
				}
			}
		}

		INT_CHECK( "fabrikbauer 670" );

		if(lcount!=0) {
			/* if a certain number of producer is requested,
			 * we will built at least some new factories
			 * crossconnected ones only count half for this
			 */
			found = lfound/2;
		}

		/* try to add all types of factories until demand is satisfied
		 * or give up after 50 tries
		 */
		int retry = 0;
		const fabrik_besch_t *hersteller = NULL;
		static koord retry_koord[1+8+16]={
			koord(0,0), // center
			koord(-1,-1), koord(0,-1), koord(1,-1), koord(1,0), koord(1,1), koord(0,1), koord(-1,1), koord(-1,0), // nearest neighbour
			koord(-2,-2), koord(-1,-2), koord(0,-2), koord(1,-2), koord(2,-2), koord(2,-1), koord(2,0), koord(2,1), koord(2,2), koord(1,2), koord(0,2), koord(-1,2), koord(-2,2), koord(-2,1), koord(-2,0), koord(-2,1) // second nearest neighbour
		};

		for(int j=0;  j<50  &&  (lcount>lfound  ||  lcount==0)  &&  verbrauch>0;  j++  ) {

			if(retry==0  ||  retry>25) {
				hersteller = finde_hersteller(ware,j%anzahl_hersteller);
				if(info==hersteller) {
					dbg->error("fabrikbauer_t::baue_hierarchie()","found myself! (pak corrupted?)");
					return n;
				}
				retry = 0;
			}

			int rotate = simrand(hersteller->gib_haus()->gib_all_layouts()-1);
			koord3d k = finde_zufallsbauplatz(welt, *pos+(retry_koord[retry]*DISTANCE*2), DISTANCE, hersteller->gib_haus()->gib_groesse(rotate),hersteller->gib_platzierung()==fabrik_besch_t::Wasser,hersteller->gib_haus());

			INT_CHECK("fabrikbauer 697");

			if(welt->lookup(k)) {
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Try to built lieferant %s at (%i,%i) r=%i for %s.",hersteller->gib_name(),k.x,k.y,rotate,info->gib_name());
				n += baue_hierarchie(pos, hersteller, rotate, &k, sp);
				lfound ++;

				INT_CHECK( "fabrikbauer 702" );

				// now substract current supplier
				const ding_t * dt = welt->lookup_kartenboden(k.gib_2d())->obj_bei(0);
				if(dt  &&  dt->get_fabrik()) {
					fabrik_t *fab = dt->get_fabrik();
					if(fab==NULL) {
						continue;
					}
					new_factories.append(fab);

					// connect new supplier to us
					for(int gg=0;gg<fab->gib_besch()->gib_produkte();gg++) {
						if(fab->gib_besch()->gib_produkt(gg)->gib_ware()==ware) {
							sint32 produktion = fab->get_base_production()*fab->gib_besch()->gib_produkt(gg)->gib_faktor();
							// the take care of how much this factorycould supply
							verbrauch -= produktion;
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","new supplier %s can supply approx %i of %s to us",fab->gib_besch()->gib_name(),produktion,ware->gib_name());
							break;
						}
					}
				}
				retry = 0;
			}
			else {
				k = *pos+(retry_koord[retry]*DISTANCE*2);
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","failed to built lieferant %s around (%i,%i) r=%i for %s.",hersteller->gib_name(),k.x,k.y,rotate,info->gib_name());
				retry ++;
			}
		}

		/* now we add us to all crossconnected factories */
		for (slist_tpl<fabrik_t *>::iterator fab = crossconnected_supplier.begin(), fab_end = crossconnected_supplier.end(); fab != fab_end;  ++fab) {
			(*fab)->add_lieferziel( pos->gib_2d() );
		}

		/* now the crossconnect part:
		 * connect also the factories we stole from before ... */
		for (slist_tpl<fabrik_t *>::iterator fab = new_factories.begin(), fab_end = new_factories.end(); fab != fab_end;  ++fab) {
			for (slist_tpl<fabs_to_crossconnect_t>::iterator i = factories_to_correct.begin(), end = factories_to_correct.end(); i != end;) {
				i->demand -= 1;
				(*fab)->add_lieferziel( i->fab->gib_pos().gib_2d() );
				(*i).fab->add_supplier( (*fab)->gib_pos().gib_2d() );
				if (i->demand < 0) {
					i = factories_to_correct.erase(i);
				} else {
					++i;
				}
			}
		}

		// next ware ...
	}

	INT_CHECK( "fabrikbauer 723" );

	// finally
	if(parent==NULL) {
		DBG_MESSAGE("fabrikbauer_t::baue_hierarchie()","update karte");

		// update an open map
		reliefkarte_t::gib_karte()->calc_map();

		INT_CHECK( "fabrikbauer 730" );
	}

	return n;
}
