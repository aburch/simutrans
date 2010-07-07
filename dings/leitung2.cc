/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "leitung2.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../player/simplay.h"
#include "../simimg.h"
#include "../simfab.h"
#include "../simskin.h"

#include "../simgraph.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/powernet.h"
#include "../dataobj/umgebung.h"

#include "../boden/grund.h"
#include "../bauer/wegbauer.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/weighted_vector_tpl.h"

#define PROD 1000

/*
static const char * measures[] =
{
	"fail",
	"weak",
	"good",
	"strong",
};
*/


int leitung_t::gimme_neighbours(leitung_t **conn)
{
	int count = 0;
	grund_t *gr_base = welt->lookup(get_pos());
	for(int i=0; i<4; i++) {
		// get next connected tile (if there)
		grund_t *gr;
		conn[i] = NULL;
		if(  gr_base->get_neighbour( gr, invalid_wt, koord::nsow[i] ) ) {
			leitung_t *lt = gr->get_leitung();
			if(  lt  &&  spieler_t::check_owner(get_besitzer(), lt->get_besitzer())  ) {
				conn[i] = lt;
				count++;
			}
		}
	}
	return count;
}


fabrik_t *leitung_t::suche_fab_4(const koord pos)
{
	for(int k=0; k<4; k++) {
		fabrik_t *new_fab = fabrik_t::get_fab( welt, pos+koord::nsow[k] );
		if(new_fab) {
			return new_fab;
		}
	}
	return NULL;
}


leitung_t::leitung_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	city = NULL;
	set_net(NULL);
	ribi = ribi_t::keine;
	rdwr(file);
}


leitung_t::leitung_t(karte_t *welt, koord3d pos, spieler_t *sp) : ding_t(welt, pos)
{
	city = NULL;
	set_net(NULL);
	set_besitzer( sp );
	set_besch(wegbauer_t::leitung_besch);
}


leitung_t::~leitung_t()
{
	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		leitung_t *conn[4];
		int neighbours = gimme_neighbours(conn);
		gr->obj_remove(this);
		set_flag( ding_t::not_on_map );

		powernet_t *new_net = NULL;
		if(neighbours>1) {
			// only reconnect if two connections ...
			bool first = true;
			for(int i=0; i<4; i++) {
				if(conn[i]!=NULL) {
					if(!first) {
						// replace both nets
						new_net = new powernet_t();
						conn[i]->replace(new_net);
					}
					first = false;
				}
			}
		}

		// recalc images
		for(int i=0; i<4; i++) {
			if(conn[i]!=NULL) {
				conn[i]->calc_neighbourhood();
			}
		}

		if(neighbours==0) {
			// delete in last or crossing
//			if(welt->rem_powernet( net)) {
				// but there is still something wrong with the logic here ...
				// so we only delete, if still present in the world
				delete net;
//			}
//			else {
//				dbg->warning("~leitung()","net %p already deleted at (%i,%i)!",net,gr->get_pos().x,gr->get_pos().y);
//			}
		}
		spieler_t::add_maintenance(get_besitzer(), -besch->get_wartung());
	}
}


void leitung_t::entferne(spieler_t *sp) //"remove".
{
	spieler_t::accounting(sp, -besch->get_preis()/2, get_pos().get_2d(), COST_CONSTRUCTION);
	mark_image_dirty( bild, 0 );
}


/**
 * called during map rotation
 * @author prissi
 */
void leitung_t::rotate90()
{
	ding_t::rotate90();
	ribi = ribi_t::rotate90( ribi );
}


/* replace networks connection
 * non-trivial to handle transformers correctly
 * @author prissi
 */
void leitung_t::replace(powernet_t* new_net)
{
	if (get_net() != new_net) {
		// convert myself ...
//DBG_MESSAGE("leitung_t::replace()","My net %p by %p at (%i,%i)",new_net,current,base_pos.x,base_pos.y);
		set_net(new_net);
	}

	leitung_t * conn[4];
	if(gimme_neighbours(conn)>0) {
		for(int i=0; i<4; i++) {
			if(conn[i] && conn[i]->get_net()!=new_net) {
				conn[i]->replace(new_net);
			}
		}
	}
}


/**
 * Connect this piece of powerline to its neighbours
 * -> this can merge power networks
 * @author Hj. Malthaner
 */
void leitung_t::verbinde()
{
	// first get my own ...
	powernet_t *new_net = get_net();
//DBG_MESSAGE("leitung_t::verbinde()","Searching net at (%i,%i)",get_pos().x,get_pos().x);
	leitung_t * conn[4];
	if(gimme_neighbours(conn)>0) {
		for( uint8 i=0;  i<4 && new_net==NULL;  i++  ) {
			if(conn[i]) {
				new_net = conn[i]->get_net();
			}
		}
	}

//DBG_MESSAGE("leitung_t::verbinde()","Found net %p",new_net);

	// we are alone?
	if(get_net()==NULL) {
		if(new_net!=NULL) {
			replace(new_net);
		}
		else {
			// then we start a new net
			set_net(new powernet_t());
//DBG_MESSAGE("leitung_t::verbinde()","Creating new net %p",new_net);
		}
	}
	else if(new_net) {
		powernet_t *my_net = get_net();
		for( uint8 i=0;  i<4;  i++  ) {
			if(conn[i] && conn[i]->get_net()!=new_net) {
				conn[i]->replace(new_net);
			}
		}
		if(my_net && my_net!=new_net) {
			delete my_net;
		}
	}
}


/* extended by prissi */
void leitung_t::calc_bild()
{
	is_crossing = false;
	const koord pos = get_pos().get_2d();
	bool snow = get_pos().z >= welt->get_snowline();

	grund_t *gr = welt->lookup(get_pos());
	if(gr==NULL) {
		// no valid ground; usually happens during building ...
		return;
	}
	if(gr->ist_bruecke()) {
		// don't display on a bridge)
		set_bild(IMG_LEER);
		return;
	}

	hang_t::typ hang = gr->get_weg_hang();
	if(hang != hang_t::flach) {
		set_bild( besch->get_hang_bild_nr(hang, snow));
	}
	else {
		if(gr->hat_wege()) {
			// crossing with road or rail
			weg_t* way = gr->get_weg_nr(0);
			if(ribi_t::ist_gerade_ow(way->get_ribi())) {
				set_bild( besch->get_diagonal_bild_nr(ribi_t::nord|ribi_t::ost, snow));
			}
			else {
				set_bild( besch->get_diagonal_bild_nr(ribi_t::sued|ribi_t::ost, snow));
			}
			is_crossing = true;
		}
		else {
			if(ribi_t::ist_gerade(ribi)  &&  !ribi_t::ist_einfach(ribi)  &&  (pos.x+pos.y)&1) {
				// every second skip mast
				if(ribi_t::ist_gerade_ns(ribi)) {
					set_bild( besch->get_diagonal_bild_nr(ribi_t::nord|ribi_t::west, snow));
				}
				else {
					set_bild( besch->get_diagonal_bild_nr(ribi_t::sued|ribi_t::west, snow));
				}
			}
			else {
				set_bild( besch->get_bild_nr(ribi, snow));
			}
		}
	}
}


/**
 * Recalculates the images of all neighbouring
 * powerlines and the powerline itself
 *
 * @author Hj. Malthaner
 */
void leitung_t::calc_neighbourhood()
{
	leitung_t *conn[4];
	ribi = ribi_t::keine;
	if(gimme_neighbours(conn)>0) {
		for( uint8 i=0;  i<4 ;  i++  ) {
			if(conn[i]  &&  conn[i]->get_net()==get_net()) {
				ribi |= ribi_t::nsow[i];
				conn[i]->add_ribi(ribi_t::rueckwaerts(ribi_t::nsow[i]));
				conn[i]->calc_bild();
			}
		}
	}
	set_flag( ding_t::dirty );
	calc_bild();
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void leitung_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);

	uint32 supply = get_net()->get_supply();
	uint32 demand = get_net()->get_demand();
	uint32 load = demand>supply?supply:demand;

	buf.append(translator::translate("Net ID: "));
	buf.append((unsigned long)get_net());
	buf.append(translator::translate("\nCapacity: "));
	buf.append(get_net()->get_max_capacity()>>POWER_TO_MW);
	buf.append(translator::translate(" MW"));
	buf.append(translator::translate("\nDemand: "));
	buf.append(demand>>POWER_TO_MW);
	buf.append(translator::translate(" MW"));
	buf.append(translator::translate("\nGeneration: "));
	buf.append(supply>>POWER_TO_MW);
	buf.append(translator::translate(" MW"));
	buf.append(translator::translate("\nAct. Load: "));
	buf.append(load>>POWER_TO_MW);
	buf.append(translator::translate(" MW"));
	buf.append(translator::translate("\nGen. Usage: "));
	buf.append((100*load)/(supply>0?supply:1));
	buf.append("%");
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void leitung_t::laden_abschliessen()
{
	verbinde();
	calc_neighbourhood();
	grund_t *gr = welt->lookup(get_pos());
	assert(gr);
	spieler_t::add_maintenance(get_besitzer(), besch->get_wartung());
}


/**
 * Speichert den Zustand des Objekts.
 *
 * @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
 * soll.
 * @author Hj. Malthaner
 */
void leitung_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "leitung_t" );

	uint32 value;

	ding_t::rdwr(file);
	if(file->is_saving()) 
	{
		value = (unsigned long)get_net();
		file->rdwr_long(value, "\n");
		koord city_pos  = koord::invalid;
		if(city != NULL)
		{
			city_pos = city->get_pos();
		}
		city_pos.rdwr(file);
	}
	else 
	{
		file->rdwr_long(value, "\n");
		//      net = powernet_t::load_net((powernet_t *) value);
		set_net(NULL);
		if(file->get_experimental_version() >= 3)
		{
			koord city_pos = koord::invalid;
			city_pos.rdwr(file);
			city = welt->get_city(city_pos);
			if(city)
			{
				city->add_substation((senke_t*)this);
			}
		}
	}
	if(get_typ()==leitung) 
	{
		if(file->get_version() > 102002 && file->get_experimental_version() >= 8)
		{
			if(file->is_saving()) 
			{
				const char *s = besch->get_name();
				file->rdwr_str(s);
			}
			else 
			{
				char bname[128];
				file->rdwr_str(bname, lengthof(bname));
				if(bname[0] == '~')
				{
					set_besch(wegbauer_t::leitung_besch);					
					return;
				}

				const weg_besch_t *besch = wegbauer_t::get_besch(bname);
				if(besch==NULL) 
				{
					besch = wegbauer_t::get_besch(translator::compatibility_name(bname));
					if(besch==NULL) 
					{
						besch = wegbauer_t::leitung_besch;
					}
					dbg->warning("strasse_t::rdwr()", "Unknown powerline %s replaced by %s", bname, besch->get_name() );
				}
				set_besch(besch);
			}
		}
		else 
		{
			if (file->is_loading()) 
			{
				set_besch(wegbauer_t::leitung_besch);
			}
		}
	}
	else if(get_typ() == pumpe || get_typ() == senke)
	{
		// Must add dummy string here, or else the loading/saving will fail, 
		// since we do not know whether a leitung is a plain leitung, or a pumpe
		// or a senke on *loading*, whereas we do on saving.
		char dummy[2] = "~";
		file->rdwr_str(dummy, 2);
	}
}


/************************************ from here on pump (source) stuff ********************************************/

slist_tpl<pumpe_t *> pumpe_t::pumpe_list;


void pumpe_t::neue_karte()
{
	pumpe_list.clear();
}


void pumpe_t::step_all(long delta_t)
{
	slist_iterator_tpl<pumpe_t *> pumpe_iter( pumpe_list );
	while(  pumpe_iter.next()  ) {
		pumpe_iter.get_current()->step( delta_t );
	}
}


pumpe_t::pumpe_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
	fab = NULL;
	supply = 0;
}


pumpe_t::pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
	supply = 0;
	sp->buche( welt->get_einstellungen()->cst_transformer, get_pos().get_2d(), COST_CONSTRUCTION);
}


pumpe_t::~pumpe_t()
{
	if(fab) {
		fab->set_prodfaktor( max(16,fab->get_prodfaktor()/2) );
		fab->set_transformer_connected( false );
		pumpe_list.remove( this );
		fab = NULL;
	}
	spieler_t::add_maintenance(get_besitzer(), welt->get_einstellungen()->cst_maintain_transformer);
}


void pumpe_t::step(long delta_t)
{
	if(fab==NULL) {
		return;
	}

	if(  delta_t==0  ) {
		return;
	}

	supply = fab->get_power();

	image_id new_bild;
	int winter_offset = 0;
	if (skinverwaltung_t::senke->get_bild_anzahl() > 3  &&  get_pos().z >= welt->get_snowline()) {
		winter_offset = 2;
	}
	if(  supply > 0  ) {
		get_net()->add_supply( supply );
		new_bild = skinverwaltung_t::pumpe->get_bild_nr(1+winter_offset);
	}
	else {
		new_bild = skinverwaltung_t::pumpe->get_bild_nr(0+winter_offset);
	}
	if(bild!=new_bild) {
		set_flag(ding_t::dirty);
		set_bild( new_bild );
	}
}


void pumpe_t::laden_abschliessen()
{
	leitung_t::laden_abschliessen();
	spieler_t::add_maintenance(get_besitzer(), -welt->get_einstellungen()->cst_maintain_transformer);

	if(fab==NULL  &&  get_net()) {
		fab = leitung_t::suche_fab_4(get_pos().get_2d());
		fab->set_transformer_connected( true );
	}
	pumpe_list.insert( this );
}


void pumpe_t::info(cbuffer_t & buf) const
{
	ding_t::info( buf );

	buf.append(translator::translate("Net ID: "));
	buf.append((unsigned long)get_net());
	buf.append(translator::translate("\nGeneration: "));
	buf.append(supply>>POWER_TO_MW);
	buf.append(translator::translate(" MW"));
}


/************************************ From here on drain stuff ********************************************/

slist_tpl<senke_t *> senke_t::senke_list;


void senke_t::neue_karte()
{
	senke_list.clear();
}


void senke_t::step_all(long delta_t)
{
	slist_iterator_tpl<senke_t *> senke_iter( senke_list );
	while(  senke_iter.next()  ) {
		senke_iter.get_current()->step( delta_t );
	}

}


senke_t::senke_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
	fab = NULL;
	einkommen = 0;
	max_einkommen = 1;
	next_t = 0;
	delta_sum = 0;
	last_power_demand = 0;
	power_load = 0;
}


senke_t::senke_t(karte_t *welt, koord3d pos, spieler_t *sp, stadt_t* c) : leitung_t(welt, pos, sp)
{
	fab = NULL;
	einkommen = 0;
	max_einkommen = 1;
	city = c;
	if(city)
	{
		city->add_substation(this);
	}
	next_t = 0;
	delta_sum = 0;
	last_power_demand = 0;
	power_load = 0;
	sp->buche( welt->get_einstellungen()->cst_transformer, get_pos().get_2d(), COST_CONSTRUCTION);
}


senke_t::~senke_t()
{
	if(fab || city)
	{
		senke_list.remove( this );
		welt->sync_remove( this );
		if(fab)
		{
			fab->set_prodfaktor( 16 );
			fab->set_transformer_connected( false );
		}
		if(city)
		{
			city->remove_substation(this);
		}
	}
	spieler_t::add_maintenance(get_besitzer(), welt->get_einstellungen()->cst_maintain_transformer);
}


void senke_t::step(long delta_t)
{
	if(fab==NULL && city==NULL) {
		return;
	}

	if(delta_t==0) {
		return;
	}

	uint32 power_demand = 0;
	// 'fab' is the connected factory
	uint32 fab_power_demand = 0;
	uint32 fab_power_load = 0;
	// 'municipal' is city population plus factories in city
	uint32 municipal_power_demand = 0;
	uint32 municipal_power_load = 0;

	/* First add up all sources of demand and add the demand to the net. */
	if (fab != NULL) {
		fab_power_demand = fab->get_power_demand();
	}
	if(city != NULL)
	{
		const vector_tpl<fabrik_t*>& city_factories = city->get_city_factories();
		ITERATE(city_factories, i)
		{
			if (city_factories[i] != fab) { // don't double-count for the factory supplied above
				municipal_power_demand += city_factories[i]->get_power_demand();
			}
		}
		// Add the demand for the population
		municipal_power_demand += city->get_power_demand();
	}

	power_demand = fab_power_demand + municipal_power_demand;
	uint32 shared_power_demand = power_demand;
	double load_proportion = 1.0;

	bool supply_max = false;
	if(city)
	{
		// Check to see whether there are any *other* substations in the city that supply it with electricity,
		// and divide the demand between them.
		vector_tpl<senke_t*>* city_substations = city->get_substations();
		uint16 city_substations_number = city_substations->get_count();

		uint32 supply;
		vector_tpl<senke_t*> checked_substations;
		ITERATE_PTR(city_substations, i)
		{
			// Must use two passes here: first, check all those that don't have enough to supply 
			// an equal share, then check those that do.

			supply = city_substations->get_element(i)->get_power_load();

			if(supply < (shared_power_demand / (city_substations_number - checked_substations.get_count())))
			{
				if(city_substations->get_element(i) != this)
				{
					shared_power_demand -= supply;
				}
				else
				{
					supply_max = true;
				}
				checked_substations.append(city_substations->get_element(i));
			}
		}
		city_substations_number -= checked_substations.get_count();

		uint32 demand_distribution;
		uint8 count = 0;
		for(uint16 n = 0; n < city_substations->get_count(); n ++)
		{
			// Now check those that have more than enough power.

			if(city_substations->get_element(n) == this || checked_substations.is_contained(city_substations->get_element(n)))
			{
				continue;
			}

			supply = city_substations->get_element(n)->get_power_load();
			demand_distribution = shared_power_demand / (city_substations_number - count);
			if(supply < demand_distribution)
			{
				// Just in case the lack of power of the others means that this
				// one does not have enough power.
				shared_power_demand -= supply;
				count ++;
			}
			else
			{
				shared_power_demand -= demand_distribution;
				count ++;
			}
		}
	}

	if(supply_max)
	{
		shared_power_demand = get_power_load() ? get_power_load() : get_net()->get_supply();
	}

	// Add only this substation's share of the power. 
	get_net()->add_demand(shared_power_demand);
	
	if(city && city->get_substations()->get_count() > 1)
	{
		load_proportion = (double)shared_power_demand / (double)power_demand;
	}

	power_load = get_power_load();

	/* Now actually feed the power through to the factories and city */
	if (fab != NULL) {
		// the connected fab gets priority access to the power supply if there's a shortage
		// This should use 'min', but the current version of that in simtypes.h 
		// would cast to signed int (12.05.10)  FIXME.
		if (fab_power_demand < power_load) {
			fab_power_load = fab_power_demand;
		} else {
			fab_power_load = power_load;
		}
		fab->add_power( fab_power_load );
		if (fab_power_demand > fab_power_load) {
			// this allows subsequently stepped senke to supply demand
			// which this senke couldn't
			fab->add_power_demand( power_demand-fab_power_load );
		}
	}
	if (power_load > fab_power_load) {
		municipal_power_load = power_load - fab_power_load;
	}
	if(city != NULL)
	{
		// everyone else splits power on a proportional basis -- brownouts!
		const vector_tpl<fabrik_t*>& city_factories = city->get_city_factories();
		ITERATE(city_factories, i)
		{
			if (city_factories[i] != fab) { //don't produce twice for the factory supplied above
				const uint32 current_factory_demand = city_factories[i]->get_power_demand() * load_proportion;
				const uint32 current_factory_load = (
						current_factory_demand
						* (municipal_power_load << 5)
						/ municipal_power_demand
					) >> 5; // <<5 for same reasons as above, FIXME
				city_factories[i]->add_power( current_factory_load );
				if (current_factory_demand > current_factory_load) {
					// this allows subsequently stepped senke to supply demand
					// which this senke couldn't
					city_factories[i]->add_power_demand( current_factory_demand - current_factory_load );
				}

			}
		}
		// City gets growth credit for power for both citizens and city factories
		city->add_power((municipal_power_load>>POWER_TO_MW) * load_proportion);
		city->add_power_demand((municipal_power_demand>>POWER_TO_MW) * (load_proportion * load_proportion));
	}
	// Income
	max_einkommen += last_power_demand * delta_t / PRODUCTION_DELTA_T;
	einkommen += ((power_load  * delta_t / PRODUCTION_DELTA_T) * load_proportion);

	// Income rollover
	if(max_einkommen>(2000<<11)) {
		get_besitzer()->buche(einkommen >> 11, get_pos().get_2d(), COST_POWERLINES);
		get_besitzer()->buche(einkommen >> 11, get_pos().get_2d(), COST_INCOME);
		einkommen = 0;
		max_einkommen = 1;
	}

	last_power_demand = shared_power_demand;
}

uint32 senke_t::get_power_load() const
{
	uint32 pl = power_load;
	uint32 net_demand = get_net()->get_demand();
	if(  net_demand > 0  ) 
	{
		pl = (last_power_demand * ((get_net()->get_supply() << 5) / net_demand)) >>5 ; //  <<5 for max calculation precision fitting within uint32 with max supply capped in dataobj/powernet.cc max_capacity
		if(  pl > last_power_demand  ) 
		{
			pl = last_power_demand;
		}
	}
	else 
	{
		pl = 0;
	}
	return pl;
}

bool senke_t::sync_step(long delta_t)
{
	if( fab == NULL && city == NULL) {
		return false;
	}

	delta_sum += delta_t;
	if(  delta_sum > PRODUCTION_DELTA_T  ) {
		// sawtooth waveform resetting at PRODUCTION_DELTA_T => time period for image changing
		delta_sum -= delta_sum - delta_sum % PRODUCTION_DELTA_T;
	}

	next_t += delta_t;
	if(  next_t > PRODUCTION_DELTA_T / 16  ) {
		// sawtooth waveform resetting at PRODUCTION_DELTA_T / 16 => image changes at most this fast
		next_t -= next_t - next_t % (PRODUCTION_DELTA_T / 16);

		image_id new_bild;
		int winter_offset = 0;
		if (skinverwaltung_t::senke->get_bild_anzahl() > 3  &&  get_pos().z >= welt->get_snowline()) {
			winter_offset = 2;
		}
		if(  last_power_demand > 0 ) {
			uint32 load_factor = power_load * PRODUCTION_DELTA_T / last_power_demand;

			// allow load factor to be 0, 1/8 to 7/8, 1
			// ensures power on image shows for low loads and ensures power off image shows for high loads
			if(  load_factor > 0  &&  load_factor < PRODUCTION_DELTA_T / 8  ) {
				load_factor = PRODUCTION_DELTA_T / 8;
			}
			else {
				if(  load_factor > 7 * PRODUCTION_DELTA_T / 8  &&  load_factor < PRODUCTION_DELTA_T  ) {
					load_factor = 7 * PRODUCTION_DELTA_T / 8;
				}
			}

			if(  delta_sum <= (sint32)load_factor  ) {
				new_bild = skinverwaltung_t::senke->get_bild_nr(1+winter_offset);
			}
			else {
				new_bild = skinverwaltung_t::senke->get_bild_nr(0+winter_offset);
			}
		}
		else {
			new_bild = skinverwaltung_t::senke->get_bild_nr(0+winter_offset);
		}
		if(  bild != new_bild  ) {
			set_flag(ding_t::dirty);
			set_bild( new_bild );
		}
	}
	return true;
}


void senke_t::laden_abschliessen()
{
	leitung_t::laden_abschliessen();
	spieler_t::add_maintenance(get_besitzer(), -welt->get_einstellungen()->cst_maintain_transformer);

	if(fab == NULL && city == NULL && get_net()) 
	{
		fab = leitung_t::suche_fab_4(get_pos().get_2d());
		fab->set_transformer_connected( true );
	}
	senke_list.insert( this );
	welt->sync_add(this);
}


void senke_t::info(cbuffer_t & buf) const
{
	ding_t::info( buf );

	buf.append(translator::translate("Net ID: "));
	buf.append((unsigned long)get_net());
	buf.append(translator::translate("\nDemand: "));
	buf.append(last_power_demand>>POWER_TO_MW);
	buf.append(translator::translate(" MW"));
	buf.append(translator::translate("\nAct. Load: "));
	buf.append(power_load>>POWER_TO_MW);
	buf.append(translator::translate(" MW"));
	buf.append(translator::translate("\nSupplied: "));
	buf.append((100*power_load)/(last_power_demand>0?last_power_demand:1));
	buf.append("%");
	if(city)
	{
		buf.append(translator::translate("\nCity: "));
		buf.append(city->get_name());
	}
}
