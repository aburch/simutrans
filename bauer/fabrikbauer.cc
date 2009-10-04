/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
#include "../simdebug.h"
#include "fabrikbauer.h"
#include "hausbauer.h"
#include "../simworld.h"
#include "../simintr.h"
#include "../simfab.h"
#include "../simgraph.h"
#include "../simmesg.h"
#include "../simtools.h"
#include "../simcity.h"
#include "../simskin.h"
#include "../simhalt.h"
#include "../player/simplay.h"

#include "../boden/grund.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/array_tpl.h"
#include "../sucher/bauplatz_sucher.h"

#include "../gui/karte.h"	// to update map after construction of new industry

// radius for checking places for construction
#define DISTANCE 40


// all factories and their exclusion areas
static array_tpl<uint8> fab_map;
static sint32 fab_map_w=0;


// marks factories with exclusion region in the position map
void add_factory_to_fab_map( karte_t *welt, const fabrik_t *fab )
{
	sint16 start_y = max( 0, fab->get_pos().y-welt->get_einstellungen()->get_factory_spacing() );
	const sint16 end_y = min( welt->get_groesse_y()-1, fab->get_pos().y+fab->get_besch()->get_haus()->get_h(fab->get_rotate())+welt->get_einstellungen()->get_factory_spacing() );
	const sint16 start_x = max( 0, fab->get_pos().x-welt->get_einstellungen()->get_factory_spacing() );
	const sint16 end_x = min( welt->get_groesse_x()-1, fab->get_pos().x+fab->get_besch()->get_haus()->get_b(fab->get_rotate())+welt->get_einstellungen()->get_factory_spacing() );
	while(start_y<end_y) {
		for(  sint16 x=start_x;  x<end_x;  x++  ) {
			fab_map[fab_map_w*start_y+(x/8)] |= 1<<(x%8);
		}
		start_y ++;
	}
}



// create map with all factories and exclusion area
void init_fab_map( karte_t *welt )
{
	fab_map_w = ((welt->get_groesse_x()+7)/8);
	fab_map.resize( fab_map_w*welt->get_groesse_y() );
	for( int i=0;  i<fab_map_w*welt->get_groesse_y();  i++ ) {
		fab_map[i] = 0;
	}
	slist_iterator_tpl <fabrik_t *> iter(welt->get_fab_list());
	while(iter.next()) {
		add_factory_to_fab_map( welt, iter.get_current() );
	}
}



// true, if factory coordinate
inline bool is_factory_at( sint16 x, sint16 y)
{
	return (fab_map[(fab_map_w*y)+(x/8)]&(1<<(x%8)))!=0;
}


void fabrikbauer_t::neue_karte(karte_t *welt)
{
	init_fab_map( welt );
}



/**
 * bauplatz_mit_strasse_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class factory_bauplatz_mit_strasse_sucher_t: public bauplatz_sucher_t  {

public:
	factory_bauplatz_mit_strasse_sucher_t(karte_t *welt) : bauplatz_sucher_t(welt) {}

	bool strasse_bei(sint16 x, sint16 y) const {
		grund_t *bd = welt->lookup_kartenboden( koord(x,y) );
		return bd && bd->hat_weg(road_wt);
	}

	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const {
		if(bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)) {
			// try to built a little away from previous factory
			for(sint16 y=pos.y;  y<pos.y+h;  y++  ) {
				for(sint16 x=pos.x;  x<pos.x+b;  x++  ) {
					if( is_factory_at(x,y)  ) {
						return false;
					}
					grund_t *gr = welt->lookup_kartenboden(koord(x,y));
					if(gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1))!=NULL) {
						// something on top (monorail or powerlines)
						return false;
					}
				}
			}
			// check to not built on a road
			int i, j;
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
const fabrik_besch_t *fabrikbauer_t::get_random_consumer(bool electric, climate_bits cl, uint16 timeline )
{
	// get a random city factory
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);
	slist_tpl <const fabrik_besch_t *> consumer;
	int gewichtung=0;

	while(iter.next()) {
		const fabrik_besch_t *current=iter.get_current_value();
		// nur endverbraucher eintragen
		if(current->get_produkt(0)==NULL  &&  current->get_haus()->is_allowed_climate_bits(cl)  &&
			(electric ^ !current->is_electricity_producer())  &&
			(timeline==0  ||  (current->get_haus()->get_intro_year_month() <= timeline  &&  current->get_haus()->get_retire_year_month() > timeline))  ) {
			consumer.insert(current);
			gewichtung += current->get_gewichtung();
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
		if (next < fb->get_gewichtung()) {
			DBG_MESSAGE("fabrikbauer_t::get_random_consumer()", "consumer %s found.", fb->get_name());
			return fb;
		}
		next -= fb->get_gewichtung();
	}
	DBG_MESSAGE("fabrikbauer_t::get_random_consumer()", "consumer %s found.", consumer.front()->get_name());
	return consumer.front();
}



const fabrik_besch_t *fabrikbauer_t::get_fabesch(const char *fabtype)
{
	return table.get(fabtype);
}



void fabrikbauer_t::register_besch(fabrik_besch_t *besch)
{
	uint16 p=besch->get_produktivitaet();
	if(p&0x8000) {
		koord k=besch->get_haus()->get_groesse();
		// to be compatible with old factories, since new code only steps once per factory, not per tile
		besch->set_produktivitaet( (p&0x7FFF)*k.x*k.y );
DBG_DEBUG("fabrikbauer_t::register_besch()","Correction for old factory: Increase poduction from %i by %i",p&0x7FFF,k.x*k.y);
	}
	if(  table.remove(besch->get_name())  ) {
		dbg->warning( "fabrikbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	table.put(besch->get_name(), besch);
}



int
fabrikbauer_t::finde_anzahl_hersteller(const ware_besch_t *ware, uint16 timeline)
{
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);
	int anzahl=0;

	while(iter.next()) {
		const fabrik_besch_t *tmp = iter.get_current_value();

		for (uint i = 0; i < tmp->get_produkte(); i++) {
			const fabrik_produkt_besch_t *produkt = tmp->get_produkt(i);
			if(produkt->get_ware()==ware  &&  tmp->get_gewichtung()>0  &&  (timeline==0  ||  (tmp->get_haus()->get_intro_year_month() <= timeline  &&  tmp->get_haus()->get_retire_year_month() > timeline))  ) {
				anzahl ++;
			}
		}
	}
DBG_MESSAGE("fabrikbauer_t::finde_anzahl_hersteller()","%i producer for good '%s' fount.", anzahl, translator::translate(ware->get_name()));
	return anzahl;
}



/* finds a producer;
 * also water only producer are allowed
 */
const fabrik_besch_t *fabrikbauer_t::finde_hersteller(const ware_besch_t *ware, uint16 timeline )
{
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);
	slist_tpl <const fabrik_besch_t *> producer;
	int gewichtung=0;

	while(iter.next()) {
		const fabrik_besch_t *tmp = iter.get_current_value();

		for (uint i = 0; i < tmp->get_produkte(); i++) {
			const fabrik_produkt_besch_t *produkt = tmp->get_produkt(i);
			if(produkt->get_ware()==ware  &&  tmp->get_gewichtung()>0  &&  (timeline==0  ||  (tmp->get_haus()->get_intro_year_month() <= timeline  &&  tmp->get_haus()->get_retire_year_month() > timeline))  ) {
				producer.insert(tmp);
				gewichtung += tmp->get_gewichtung();
				break;
			}
		}
	}

	// no producer installed?
	if (producer.empty()) {
		dbg->error("fabrikbauer_t::finde_hersteller()","no producer for good '%s' was found", translator::translate(ware->get_name()));
		return NULL;
	}
	// now find a random one
	int next=simrand(gewichtung);
	const fabrik_besch_t *besch=NULL;
	for (slist_iterator_tpl<const fabrik_besch_t*> i(producer); i.next();) {
		besch = i.get_current();
		next -= besch->get_gewichtung();
		if(next<0) {
			break;
		}
	}
	DBG_MESSAGE("fabrikbauer_t::finde_hersteller()","producer for good '%s' was found %s", translator::translate(ware->get_name()),besch->get_name());
	return besch;
}




koord3d
fabrikbauer_t::finde_zufallsbauplatz(karte_t * welt, const koord3d pos, const int radius, koord groesse, bool wasser, const haus_besch_t *besch)
{
	static vector_tpl<koord3d> list(10000);
	koord k;
	bool is_fabrik = besch->get_utyp()==haus_besch_t::fabrik;

	list.clear();
	if(wasser) {
		groesse += koord(6,6);
	}

	// check no factory but otherwise good place
	for(k.y=pos.y-radius; k.y<=pos.y+radius; k.y++) {
		if(k.y<0) continue;
		if(k.y>=welt->get_groesse_y()) break;

		for(k.x=pos.x-radius; k.x<=pos.x+radius; k.x++) {
			if(k.x<0) continue;
			if(k.x>=welt->get_groesse_x()) break;
			// climate check
			if(is_fabrik  &&  is_factory_at(k.x,k.y)) {
				continue;
			}
			if(fabrik_t::ist_bauplatz(welt, k, groesse,wasser,besch->get_allowed_climate_bits())) {
				list.append(welt->lookup(k)->get_kartenboden()->get_pos());
				// nicht gleich daneben nochmal suchen
				k.x += 4;
				if(list.get_count()>=10000) {
					goto finish;
				}
			}
		}
	}
finish:
	// printf("Zufallsbauplatzindex %d\n", index);
	if(list.get_count()==0) {
		return koord3d(-1, -1, -1);
	}
	else {
		if(wasser) {
			// take care of offset
			return list[simrand(list.get_count())] + koord3d(3, 3, 0);
		}
		return list[simrand(list.get_count())];
	}
}


/* Create a certain numer of tourist attractions
 * @author prissi
 */
void fabrikbauer_t::verteile_tourist(karte_t* welt, int max_number)
{
	// current count
	int current_number=0;

	// select without timeline disappearing dates
	if(hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),true,temperate_climate)==NULL) {
		// nothing at all?
		return;
	}

	// very fast, so we do not bother updating progress bar
	printf("Distributing %i tourist attractions ...\n",max_number);fflush(NULL);

	int retrys = max_number*4;
	while(current_number<max_number  &&  retrys-->0) {
		koord3d	pos=koord3d(simrand(welt->get_groesse_x()),simrand(welt->get_groesse_y()),1);
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),true,(climate)simrand((int)arctic_climate+1));

		// no attractions for that climate or too new
		if(attraction==NULL  ||  (welt->use_timeline()  &&  attraction->get_intro_year_month()>welt->get_current_month()) ) {
			continue;
		}

		int	rotation=simrand(attraction->get_all_layouts()-1);
		pos = finde_zufallsbauplatz(welt, pos, 20, attraction->get_groesse(rotation),false,attraction);	// so far -> land only
		if(welt->lookup(pos)) {
			// Platz gefunden ...
			hausbauer_t::baue(welt, welt->get_spieler(1), pos, rotation, attraction);
			current_number ++;
			retrys = max_number*4;
		}

	}
	// update an open map
	reliefkarte_t::get_karte()->calc_map_groesse();
}



/**
 * baue fabrik nach Angaben in info
 * @author Hj.Malthaner
 */
fabrik_t* fabrikbauer_t::baue_fabrik(karte_t* welt, koord3d* parent, const fabrik_besch_t* info, int rotate, koord3d pos, spieler_t* spieler)
{
	fabrik_t * fab = new fabrik_t(pos, spieler, info);

	if(parent) {
		fab->add_lieferziel(parent->get_2d());
	}

	// now build factory
	fab->baue(rotate);
	welt->add_fab(fab);
	add_factory_to_fab_map( welt, fab );

	// make all water station
	if(info->get_platzierung() == fabrik_besch_t::Wasser) {
		const haus_besch_t *besch = info->get_haus();
		koord dim = besch->get_groesse(rotate);

		koord k;
		halthandle_t halt = welt->get_spieler(1)->halt_add(pos.get_2d());
		if(halt.is_bound()) {

			for(k.x=pos.x; k.x<pos.x+dim.x; k.x++) {
				for(k.y=pos.y; k.y<pos.y+dim.y; k.y++) {
					if(welt->ist_in_kartengrenzen(k)) {
						// add all water to station
						grund_t *gr = welt->lookup(k)->get_kartenboden();
						// build only on gb, otherwise can't remove it
						// also savegame restore only halt on gb
						// this needs for bad fish swarm
						if(gr->ist_wasser()  &&  gr->hat_weg(water_wt) == 0  &&  gr->find<gebaeude_t>()) {
							halt->add_grund( gr );
						}
					}
				}
			}
			halt->set_name( translator::translate(info->get_name()) );
			halt->recalc_station_type();
		}
	}
	else {
		// connenct factory to stations
		// search for near stations and connect factory to them
		koord dim = info->get_haus()->get_groesse(rotate);
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
	if(info->get_pax_level()>0) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = staedte.begin(), end = staedte.end(); i != end; ++i) {
			(*i)->add_factory_arbeiterziel(fab);
		}
	}
	return fab;
}


// check, if we have to rotate the factories before building this tree
bool fabrikbauer_t::can_factory_tree_rotate( const fabrik_besch_t *besch )
{
	// we are finished: we cannont rotate
	if(!besch->get_haus()->can_rotate()) {
		return false;
	}

	// now check for all products (should be changed later for the root)
	for( int i=0;  i<besch->get_lieferanten();  i++  ) {

		const ware_besch_t *ware = besch->get_lieferant(i)->get_ware();
		stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(table);

		// infortunately, for every for iteration, we have to check all factories ...
		while(iter.next()) {
			const fabrik_besch_t *tmp = iter.get_current_value();

			// now check, if we produce this ...
			for (uint i = 0; i < tmp->get_produkte(); i++) {
				if(tmp->get_produkt(i)->get_ware()==ware  &&  tmp->get_gewichtung()>0) {

					if(!can_factory_tree_rotate( tmp )) {
						return false;
					}
					// check next factory
					break;
				}
			}
		}
	}
	// all good -> true;
	return true;
}


/**
 * vorbedingung: pos ist für fabrikbau geeignet
 */
int fabrikbauer_t::baue_hierarchie(koord3d* parent, const fabrik_besch_t* info, int rotate, koord3d* pos, spieler_t* sp, int number_of_chains )
{
	karte_t* welt = sp->get_welt();
	int n = 1;
	int org_rotation = -1;

	if(info==NULL) {
		// no industry found
		return 0;
	}

	// no cities at all?
	if (info->get_platzierung() == fabrik_besch_t::Stadt  &&  welt->get_staedte().empty()) {
		return 0;
	}

	// rotate until we can save it, if one of the factory is non-rotateable ...
	if(welt->cannot_save()  &&  parent==NULL  &&  !can_factory_tree_rotate(info)  ) {
		org_rotation = welt->get_einstellungen()->get_rotation();
		for(  int i=0;  i<3  &&  welt->cannot_save();  i++  ) {
			pos->rotate90( welt->get_groesse_y()-info->get_haus()->get_h(rotate) );
			welt->rotate90();
		}
		assert( !welt->cannot_save() );
	}

	// intown needs different place search
	if (info->get_platzierung() == fabrik_besch_t::Stadt) {
		stadt_fabrik_t sf;
		koord k=pos->get_2d();

		koord size=info->get_haus()->get_groesse(0);

		// built consumer (factory) intown
		sf.stadt = welt->suche_naechste_stadt(k);

		//
		// Drei Varianten:
		// A:
		// Ein Bauplatz, möglichst nah am Rathaus mit einer Strasse daneben.
		// Das könnte ein Zeitproblem geben, wenn eine Stadt keine solchen Bauplatz
		// hat und die Suche bis zur nächsten Stadt weiterläuft
		// Ansonsten erscheint mir das am realistischtsten..
		bool	is_rotate=info->get_haus()->get_all_layouts()>1;
		k = factory_bauplatz_mit_strasse_sucher_t(welt).suche_platz(sf.stadt->get_pos(), size.x, size.y, info->get_haus()->get_allowed_climate_bits(), &is_rotate);
		rotate = is_rotate?1:0;

		INT_CHECK( "fabrikbauer 588" );

		// B:
		// Gefällt mir auch. Die Endfabriken stehen eventuell etwas außerhalb der Stadt
		// aber nicht weit weg.
		// (does not obey climates though!)
		// k = finde_zufallsbauplatz(welt, welt->lookup(sf.stadt->get_pos())->get_boden()->get_pos(), 3, land_bau.dim).get_2d();

		// C:
		// Ein Bauplatz, möglichst nah am Rathaus.
		// Wenn mehrere Endfabriken bei einer Stadt landen, sind die oft hinter
		// einer Reihe Häuser "versteckt", von Strassen abgeschnitten.
		//k = bauplatz_sucher_t(welt).suche_platz(sf.stadt->get_pos(), land_bau.dim.x, land_bau.dim.y, info->get_haus()->get_allowed_climate_bits(), &is_rotate);

		if(k != koord::invalid) {
			*pos = welt->lookup(k)->get_kartenboden()->get_pos();
		}
		else {
			return 0;
		}
	}

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Construction of %s at (%i,%i).",info->get_name(),pos->x,pos->y);
	INT_CHECK("fabrikbauer 594");

	const fabrik_t *our_fab=baue_fabrik(welt, parent, info, rotate, *pos, sp);

	INT_CHECK("fabrikbauer 596");

	// now built supply chains for all products
	for(int i=0; i<info->get_lieferanten()  &&  i<number_of_chains; i++) {
		n += baue_link_hierarchie( our_fab, info, i, sp);
	}

	// finally
	if(parent==NULL) {
		DBG_MESSAGE("fabrikbauer_t::baue_hierarchie()","update karte");

		// update the map if needed
		reliefkarte_t::get_karte()->calc_map_groesse();

		INT_CHECK( "fabrikbauer 730" );

		// must rotate back?
		if(org_rotation>=0) {
			for(  int i=0;  i<4  &&  welt->get_einstellungen()->get_rotation()!=org_rotation;  i++  ) {
				pos->rotate90( welt->get_groesse_y()-1 );
				welt->rotate90();
			}
			welt->update_map();
		}
	}

	return n;
}



int fabrikbauer_t::baue_link_hierarchie(const fabrik_t* our_fab, const fabrik_besch_t* info, int lieferant_nr, spieler_t* sp)
{
	int n = 0;	// number of additional factories
	karte_t* welt = sp->get_welt();
	/* first we try to connect to existing factories and will do some
	 * crossconnect (if wanted)
	 * We must take care, to add capacity for the crossconnected ones!
	 */
	const fabrik_lieferant_besch_t *lieferant = info->get_lieferant(lieferant_nr);
	const ware_besch_t *ware = lieferant->get_ware();
	const int anzahl_hersteller=finde_anzahl_hersteller( ware, welt->get_timeline_year_month() );

	if(anzahl_hersteller==0) {
		dbg->error("fabrikbauer_t::baue_hierarchie()","No producer for %s found, chain uncomplete!",ware->get_name() );
		return 0;
	}

	// how much we need?
	sint32 verbrauch = our_fab->get_base_production()*lieferant->get_verbrauch();

	slist_tpl<fabs_to_crossconnect_t> factories_to_correct;
	slist_tpl<fabrik_t *> new_factories;	// since the crosscorrection must be done later
	slist_tpl<fabrik_t *> crossconnected_supplier;	// also done after the construction of new chains

	int lcount = lieferant->get_anzahl();
	int lfound = 0;	// number of found producers

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","lieferanten %i, lcount %i (need %i of %s)",info->get_lieferanten(),lcount,verbrauch,ware->get_name());

	// Hajo: search if there already is one or two (crossconnect everything if possible)
	const slist_tpl<fabrik_t *> & list = welt->get_fab_list();
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
			const unsigned distance = koord_distance(fab->get_pos(),our_fab->get_pos());

			if(distance>6) {//  &&  distance < simrand(welt->get_groesse_x()+welt->get_groesse_y())) {
				// ok, this would match
				// but can she supply enough?

				// now guess, how much this factory can supply
				const fabrik_besch_t* const fb = fab->get_besch();
				for (uint gg = 0; gg < fb->get_produkte(); gg++) {
					if (fb->get_produkt(gg)->get_ware() == ware && fab->get_lieferziele().get_count() < 10) { // does not make sense to split into more ...
						sint32 production_left = fab->get_base_production() * fb->get_produkt(gg)->get_faktor();
						const vector_tpl <koord> & lieferziele = fab->get_lieferziele();
						for( uint32 ziel=0;  ziel<lieferziele.get_count()  &&  production_left>0;  ziel++  ) {
							fabrik_t *zfab=fabrik_t::get_fab(welt,lieferziele[ziel]);
							for(int zz=0;  zz<zfab->get_besch()->get_lieferanten();  zz++) {
								if(zfab->get_besch()->get_lieferant(zz)->get_ware()==ware) {
									production_left -= zfab->get_base_production()*zfab->get_besch()->get_lieferant(zz)->get_verbrauch();
									break;
								}
							}
						}
						// here is actually capacity left (or sometimes just connect anyway)!
						if(production_left>0  ||  simrand(100)<(uint32)welt->get_einstellungen()->get_crossconnect_factor()) {
							found = true;
							if(production_left>0) {
								verbrauch -= production_left;
								fab->add_lieferziel(our_fab->get_pos().get_2d());
								DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","supplier %s can supply approx %i of %s to us", fb->get_name(), production_left, ware->get_name());
							}
							else {
								/* we steal something; however the total capacity
								 * needed is the same. Therefore, we just keep book
								 * from whose factories from how many we stole */
								crossconnected_supplier.append(fab);
								for( unsigned ziel=0;  ziel<lieferziele.get_count();  ziel++  ) {
									fabrik_t *zfab=fabrik_t::get_fab(welt,lieferziele[ziel]);
									slist_tpl<fabs_to_crossconnect_t>::iterator i = std::find(factories_to_correct.begin(), factories_to_correct.end(), fabs_to_crossconnect_t(zfab, 0));
									if (i == factories_to_correct.end()) {
										factories_to_correct.append(fabs_to_crossconnect_t(zfab, 1));
									}
									else {
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
			hersteller = finde_hersteller( ware, welt->get_timeline_year_month() );
			// no one at all
			if(hersteller==NULL) {
				if(welt->use_timeline()) {
					// can happen with timeline
					if(info->get_produkte()!=0) {
						dbg->error( "fabrikbauer_t::baue_hierarchie()", "no produder for %s yet!", ware->get_name() );
						return 0;
					}
					else {
						// only consumer: Will do with partly covered chains
						dbg->warning( "fabrikbauer_t::baue_hierarchie()", "no produder for %s yet!", ware->get_name() );
						break;
					}
				}
				else {
					// must not happen else
					dbg->fatal( "fabrikbauer_t::baue_hierarchie()", "no produder for %s yet!", ware->get_name() );
				}
			}
			if(info==hersteller) {
				// loop: we must stop here!
				dbg->fatal("fabrikbauer_t::baue_hierarchie()","found myself! (pak corrupted?)");
			}
			retry = 0;
		}

		int rotate = simrand(hersteller->get_haus()->get_all_layouts()-1);
		koord3d parent_pos = our_fab->get_pos();
		koord3d k = finde_zufallsbauplatz(welt, our_fab->get_pos()+(retry_koord[retry%25]*DISTANCE*2), DISTANCE, hersteller->get_haus()->get_groesse(rotate),hersteller->get_platzierung()==fabrik_besch_t::Wasser,hersteller->get_haus());

		INT_CHECK("fabrikbauer 697");

		if(welt->lookup(k)) {
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Try to built lieferant %s at (%i,%i) r=%i for %s.",hersteller->get_name(),k.x,k.y,rotate,info->get_name());
			n += baue_hierarchie(&parent_pos, hersteller, rotate, &k, sp, 10000 );
			lfound ++;

			INT_CHECK( "fabrikbauer 702" );

			// now substract current supplier
			fabrik_t *fab = fabrik_t::get_fab(welt, k.get_2d() );
			if(fab==NULL) {
				continue;
			}
			new_factories.append(fab);

			// connect new supplier to us
			const fabrik_besch_t* const fb = fab->get_besch();
			for (uint gg = 0; gg < fab->get_besch()->get_produkte(); gg++) {
				if (fb->get_produkt(gg)->get_ware() == ware) {
					sint32 produktion = fab->get_base_production() * fb->get_produkt(gg)->get_faktor();
					// the take care of how much this factorycould supply
					verbrauch -= produktion;
					DBG_MESSAGE("fabrikbauer_t::baue_hierarchie", "new supplier %s can supply approx %i of %s to us", fb->get_name(), produktion, ware->get_name());
					break;
				}
			}
			retry = 0;
		}
		else {
			k = our_fab->get_pos()+(retry_koord[retry%25]*DISTANCE*2);
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","failed to built lieferant %s around (%i,%i) r=%i for %s.",hersteller->get_name(),k.x,k.y,rotate,info->get_name());
			retry ++;
		}
	}

	/* now we add us to all crossconnected factories */
	for (slist_tpl<fabrik_t *>::iterator fab = crossconnected_supplier.begin(), fab_end = crossconnected_supplier.end(); fab != fab_end;  ++fab) {
		(*fab)->add_lieferziel( our_fab->get_pos().get_2d() );
	}

	/* now the crossconnect part:
	 * connect also the factories we stole from before ... */
	for (slist_tpl<fabrik_t *>::iterator fab = new_factories.begin(), fab_end = new_factories.end(); fab != fab_end;  ++fab) {
		for (slist_tpl<fabs_to_crossconnect_t>::iterator i = factories_to_correct.begin(), end = factories_to_correct.end(); i != end;) {
			i->demand -= 1;
			(*fab)->add_lieferziel( i->fab->get_pos().get_2d() );
			(*i).fab->add_supplier( (*fab)->get_pos().get_2d() );
			if (i->demand < 0) {
				i = factories_to_correct.erase(i);
			}
			else {
				++i;
			}
		}
	}

	INT_CHECK( "fabrikbauer 723" );

	return n;
}



/* this function is called, whenever it is time for industry growth
 * If there is still a pending consumer, it will first complete another chain for it
 * If not, it will decide to either built a power station (if power is needed)
 * or built a new consumer near the indicated position
 * @return: number of factories built
 */
int fabrikbauer_t::increase_industry_density( karte_t *welt, bool tell_me )
{
	int nr = 0;
	fabrik_t *last_built_consumer = NULL;
	int last_built_consumer_ware = 0;

	// find last consumer
	if(!welt->get_fab_list().empty()) {
		slist_iterator_tpl<fabrik_t*> iter (welt->get_fab_list());
		while(iter.next()) {
			fabrik_t *fab = iter.get_current();
			if(fab->get_besch()->get_produkte()==0) {
				last_built_consumer = fab;
				break;
			}
		}
		// ok, found consumer
		if(  last_built_consumer  ) {
			for(  int i=0;  i < last_built_consumer->get_besch()->get_lieferanten();  i++  ) {
				uint8 w_idx = last_built_consumer->get_besch()->get_lieferant(i)->get_ware()->get_index();
				for(  uint32 j=0;  j<last_built_consumer->get_suppliers().get_count();  j++  ) {
					fabrik_t *sup = fabrik_t::get_fab( welt, last_built_consumer->get_suppliers()[j] );
					const fabrik_besch_t* const fb = sup->get_besch();
					for (uint32 k = 0; k < fb->get_produkte(); k++) {
						if (fb->get_produkt(k)->get_ware()->get_index() == w_idx) {
							last_built_consumer_ware = i+1;
							goto next_ware_check;
						}
					}
				}
next_ware_check:
				// ok, found something, text next
				;
			}
		}
	}

	// first: do we have to continue unfinished buissness?
	if(last_built_consumer  &&  last_built_consumer_ware < last_built_consumer->get_besch()->get_lieferanten()) {
		uint32 last_suppliers = last_built_consumer->get_suppliers().get_count();
		do {
			nr += baue_link_hierarchie( last_built_consumer, last_built_consumer->get_besch(), last_built_consumer_ware, welt->get_spieler(1) );
			last_built_consumer_ware ++;
		} while(  last_built_consumer_ware < last_built_consumer->get_besch()->get_lieferanten()  &&  last_built_consumer->get_suppliers().get_count()==last_suppliers  );

		// only return, if successfull
		if(  last_built_consumer->get_suppliers().get_count() > last_suppliers  ) {
			DBG_MESSAGE( "fabrikbauer_t::increase_industry_density()", "added ware %i to factory %s", last_built_consumer_ware, last_built_consumer->get_name() );
			// tell the player
			if(tell_me) {
				stadt_t *s = welt->suche_naechste_stadt( last_built_consumer->get_pos().get_2d() );
				const char *stadt_name = s ? s->get_name() : "simcity";
				char buf[256];
				sprintf(buf, translator::translate("Factory chain extended\nfor %s near\n%s built with\n%i factories."), translator::translate(last_built_consumer->get_name()), stadt_name, nr );
				welt->get_message()->add_message(buf, last_built_consumer->get_pos().get_2d(), message_t::industry, CITY_KI, last_built_consumer->get_besch()->get_haus()->get_tile(0)->get_hintergrund(0, 0, 0));
			}
			reliefkarte_t::get_karte()->calc_map();
			return nr;
		}
	}

	// ok, nothing to built, thus we must start new
	last_built_consumer = NULL;
	last_built_consumer_ware = 0;

	// first decide, whether a new powerplant is needed or not
	uint32 total_produktivity = 1;
	uint32 electric_productivity = 0;

	slist_iterator_tpl<fabrik_t*> iter (welt->get_fab_list());
	while(iter.next()) {
		fabrik_t * fab = iter.get_current();
		if(fab->get_besch()->is_electricity_producer()) {
			electric_productivity += fab->get_base_production();
		}
		else {
			total_produktivity += fab->get_base_production();
		}
	}

	// now decide producer of electricity or normal ...
	sint32 promille = (electric_productivity*4000l)/total_produktivity;
	int no_electric = promille > welt->get_einstellungen()->get_electric_promille();
	DBG_MESSAGE( "fabrikbauer_t::increase_industry_density()", "production of electricity/total production is %i/%i (%i o/oo)", electric_productivity, total_produktivity, promille );

	while(  no_electric<2  ) {
		for(int retrys=20;  retrys>0;  retrys--  ) {
			const fabrik_besch_t *fab=get_random_consumer( no_electric==0, ALL_CLIMATES, welt->get_timeline_year_month() );
			if(fab) {
				const bool in_city = fab->get_platzierung() == fabrik_besch_t::Stadt;
				if(in_city  &&  welt->get_staedte().get_count()==0) {
					// we cannot built this factory here
					continue;
				}
				koord3d	pos = in_city ?
					welt->lookup_kartenboden( welt->get_staedte().at_weight( simrand( welt->get_staedte().get_sum_weight() ) )->get_pos() )->get_pos() :
					koord3d(simrand(welt->get_groesse_x()),simrand(welt->get_groesse_y()),1);

				int	rotation=simrand(fab->get_haus()->get_all_layouts()-1);
				if(!in_city) {
					pos = finde_zufallsbauplatz(welt, pos, 20, fab->get_haus()->get_groesse(rotation),fab->get_platzierung()==fabrik_besch_t::Wasser,fab->get_haus());
				}
				if(welt->lookup(pos)) {
					// Platz gefunden ...
					nr += baue_hierarchie(NULL, fab, rotation, &pos, welt->get_spieler(1), 1 );
					if(nr>0) {
						fabrik_t *our_fab = fabrik_t::get_fab( welt, pos.get_2d() );
						if(in_city) {
							last_built_consumer = our_fab;
							last_built_consumer_ware = 1;
						}
						reliefkarte_t::get_karte()->calc_map_groesse();
						// tell the player
						if(tell_me) {
							stadt_t *s = welt->suche_naechste_stadt( pos.get_2d() );
							const char *stadt_name = s ? s->get_name() : "simcity";
							char buf[256];
							sprintf(buf, translator::translate("New factory chain\nfor %s near\n%s built with\n%i factories."), translator::translate(our_fab->get_name()), stadt_name, nr );
							welt->get_message()->add_message(buf, pos.get_2d(), message_t::industry, CITY_KI, our_fab->get_besch()->get_haus()->get_tile(0)->get_hintergrund(0, 0, 0));
						}
						return nr;
					}
				}
			}
		}
		// if electricity not sucess => try next
		no_electric ++;
	}

	// we should not reach here, because that means neither land nor city industries exist ...
	dbg->warning( "fabrikbauer_t::increase_industry_density()", "No suitable city industry found => pak missing something?" );
	return 0;
}
