/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#if MULTI_THREAD>1
#include <pthread.h>
static pthread_mutex_t verbinde_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t calc_bild_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pumpe_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t senke_list_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

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


/**
 * returns possible directions for powerline on this tile
 */
ribi_t::ribi get_powerline_ribi(grund_t *gr)
{
	hang_t::typ slope = gr->get_weg_hang();
	ribi_t::ribi ribi = (ribi_t::ribi)ribi_t::alle;
	if (slope == hang_t::flach) {
		// respect possible directions for bridge and tunnel starts
		if (gr->ist_karten_boden()  &&  (gr->ist_tunnel()  ||  gr->ist_bruecke())) {
			ribi = ribi_t::doppelt( ribi_typ( gr->get_grund_hang() ) );
		}
	}
	else {
		ribi = ribi_t::doppelt( ribi_typ(slope) );
	}
	return ribi;
}

int leitung_t::gimme_neighbours(leitung_t **conn)
{
	int count = 0;
	grund_t *gr_base = welt->lookup(get_pos());
	ribi_t::ribi ribi = get_powerline_ribi(gr_base);
	for(int i=0; i<4; i++) {
		// get next connected tile (if there)
		grund_t *gr;
		conn[i] = NULL;
		if(  (ribi & ribi_t::nsow[i])  &&  gr_base->get_neighbour( gr, invalid_wt, ribi_t::nsow[i] ) ) {
			leitung_t *lt = gr->get_leitung();
			// check that we can connect to the other tile: correct slope,
			// both ground or both tunnel or both not tunnel
			bool const ok = (gr->ist_karten_boden()  &&  gr_base->ist_karten_boden())  ||  (gr->ist_tunnel()==gr_base->ist_tunnel());
			if(  lt  &&  (ribi_t::rueckwaerts(ribi_t::nsow[i]) & get_powerline_ribi(gr))  &&  ok  ) {
				const spieler_t *owner = get_besitzer();
				const spieler_t *other = lt->get_besitzer();
				const spieler_t *super = welt->get_spieler(1);
				if (owner==other  ||  owner==super  ||  other==super) {
					conn[i] = lt;
					count++;
				}
			}
		}
	}
	return count;
}


fabrik_t *leitung_t::suche_fab_4(const koord pos)
{
	for(int k=0; k<4; k++) {
		fabrik_t *fab = fabrik_t::get_fab( welt, pos+koord::nsow[k] );
		if(fab) {
			return fab;
		}
	}
	return NULL;
}


leitung_t::leitung_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	bild = IMG_LEER;
	set_net(NULL);
	ribi = ribi_t::keine;
	rdwr(file);
}


leitung_t::leitung_t(karte_t *welt, koord3d pos, spieler_t *sp) : ding_t(welt, pos)
{
	bild = IMG_LEER;
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
			delete net;
		}
		if(!gr->ist_tunnel()) {
			spieler_t::add_maintenance(get_besitzer(), -besch->get_wartung(), powerline_wt);
		}
	}
}


void leitung_t::entferne(spieler_t *sp)
{
	spieler_t::book_construction_costs(sp, -besch->get_preis()/2, get_pos().get_2d(), powerline_wt);
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
	bool snow = get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate;

	grund_t *gr = welt->lookup(get_pos());
	if(gr==NULL) {
		// no valid ground; usually happens during building ...
		return;
	}
	if(gr->ist_bruecke() || (gr->get_typ()==grund_t::tunnelboden && gr->ist_karten_boden())) {
		// don't display on a bridge or in a tunnel)
		set_bild(IMG_LEER);
		return;
	}

	image_id old_image = get_bild();
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
	if (old_image != get_bild()) {
		mark_image_dirty(old_image,0);
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

	const uint64 supply = get_net()->get_supply();
	const uint64 demand = get_net()->get_demand();
	const uint64 load = demand>supply ? supply:demand;

	buf.printf( translator::translate("Net ID: %lu\n"), (unsigned long)get_net() );
//	buf.printf( translator::translate("Capacity: %u MW\n"), (uint32)(get_net()->get_max_capacity()>>POWER_TO_MW) );
	buf.printf( translator::translate("Demand: %u MW\n"), (uint32)(demand>>POWER_TO_MW) );
	buf.printf( translator::translate("Generation: %u MW\n"), (uint32)(supply>>POWER_TO_MW) );
	buf.printf( translator::translate("Act. load: %u MW\n"), (uint32)(load>>POWER_TO_MW) );
	buf.printf( translator::translate("Usage: %u %%"), (uint32)((100ull*load)/(supply>0?supply:1ull)) );
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void leitung_t::laden_abschliessen()
{
#if MULTI_THREAD>1
	pthread_mutex_lock( &verbinde_mutex );
#endif
	verbinde();
#if MULTI_THREAD>1
	pthread_mutex_unlock( &verbinde_mutex );
#endif
#if MULTI_THREAD>1
	pthread_mutex_lock( &calc_bild_mutex );
#endif
	calc_neighbourhood();
#if MULTI_THREAD>1
	pthread_mutex_unlock( &calc_bild_mutex );
#endif
	grund_t *gr = welt->lookup(get_pos());
	assert(gr);

	spieler_t::add_maintenance(get_besitzer(), besch->get_wartung(), powerline_wt);
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
	if(file->is_saving()) {
		value = (unsigned long)get_net();
		file->rdwr_long(value);
	}
	else {
		file->rdwr_long(value);
		set_net(NULL);
	}

	if(get_typ()==leitung) {
		/* ATTENTION: during loading thus MUST not be called from the constructor!!!
		 * (Otherwise it will be always true!
		 */
		if(file->get_version() > 102002) {
			if(file->is_saving()) {
				const char *s = besch->get_name();
				file->rdwr_str(s);
			}
			else {
				char bname[128];
				file->rdwr_str(bname, lengthof(bname));

				const weg_besch_t *besch = wegbauer_t::get_besch(bname);
				if(besch==NULL) {
					besch = wegbauer_t::get_besch(translator::compatibility_name(bname));
					if(besch==NULL) {
						welt->add_missing_paks( bname, karte_t::MISSING_WAY );
						besch = wegbauer_t::leitung_besch;
					}
					dbg->warning("leitung_t::rdwr()", "Unknown powerline %s replaced by %s", bname, besch->get_name() );
				}
				set_besch(besch);
			}
		}
		else {
			if (file->is_loading()) {
				set_besch(wegbauer_t::leitung_besch);
			}
		}
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
	FOR(slist_tpl<pumpe_t*>, const p, pumpe_list) {
		p->step(delta_t);
	}
}


pumpe_t::pumpe_t(karte_t *welt, loadsave_t *file ) : leitung_t( welt, koord3d::invalid, NULL )
{
	fab = NULL;
	supply = 0;
	rdwr( file );
}


pumpe_t::pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
	supply = 0;
	spieler_t::book_construction_costs(sp, welt->get_settings().cst_transformer, get_pos().get_2d(), powerline_wt);
}


pumpe_t::~pumpe_t()
{
	if(fab) {
		fab->set_transformer_connected( false );
		fab = NULL;
	}
	pumpe_list.remove( this );
	spieler_t::add_maintenance(get_besitzer(), (sint32)welt->get_settings().cst_maintain_transformer, powerline_wt);
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
	if(  skinverwaltung_t::senke->get_bild_anzahl() > 3  &&  (get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate)  ) {
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
	spieler_t::add_maintenance(get_besitzer(), -welt->get_settings().cst_maintain_transformer, powerline_wt);

	assert(get_net());

	if(  fab==NULL  ) {
		if(welt->lookup(get_pos())->ist_karten_boden()) {
			// on surface, check around
			fab = leitung_t::suche_fab_4(get_pos().get_2d());
		}
		else {
			// underground, check directly above
			fab = fabrik_t::get_fab(welt, get_pos().get_2d());
		}
		if(  fab  ) {
			// only add when factory there
			fab->set_transformer_connected( true );
		}
	}
#if MULTI_THREAD>1
	pthread_mutex_lock( &pumpe_list_mutex );
#endif
	pumpe_list.insert( this );
#if MULTI_THREAD>1
	pthread_mutex_unlock( &pumpe_list_mutex );
#endif
#if MULTI_THREAD>1
	pthread_mutex_lock( &calc_bild_mutex );
#endif
	set_bild(skinverwaltung_t::pumpe->get_bild_nr(0));
	is_crossing = false;
#if MULTI_THREAD>1
	pthread_mutex_unlock( &calc_bild_mutex );
#endif
}


void pumpe_t::info(cbuffer_t & buf) const
{
	ding_t::info( buf );

	buf.printf( translator::translate("Net ID: %lu\n"), (unsigned long)get_net() );
	buf.printf( translator::translate("Generation: %u MW\n"), supply>>POWER_TO_MW );
	buf.printf("\n\n"); // pad for consistent dialog size
}


/************************************ From here on drain stuff ********************************************/

slist_tpl<senke_t *> senke_t::senke_list;


void senke_t::neue_karte()
{
	senke_list.clear();
}


void senke_t::step_all(long delta_t)
{
	FOR(slist_tpl<senke_t*>, const s, senke_list) {
		s->step(delta_t);
	}
}


senke_t::senke_t(karte_t *welt, loadsave_t *file) : leitung_t( welt, koord3d::invalid, NULL )
{
	fab = NULL;
	einkommen = 0;
	max_einkommen = 1;
	next_t = 0;
	delta_sum = 0;
	last_power_demand = 0;
	power_load = 0;
	rdwr( file );
	welt->sync_add(this);
}


senke_t::senke_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
	einkommen = 0;
	max_einkommen = 1;
	next_t = 0;
	delta_sum = 0;
	last_power_demand = 0;
	power_load = 0;
	spieler_t::book_construction_costs(sp, welt->get_settings().cst_transformer, get_pos().get_2d(), powerline_wt);
	welt->sync_add(this);
}


senke_t::~senke_t()
{
	welt->sync_remove( this );
	if(fab!=NULL) {
		fab->set_transformer_connected( false );
		fab = NULL;
	}
	senke_list.remove( this );
	spieler_t::add_maintenance(get_besitzer(), welt->get_settings().cst_maintain_transformer, powerline_wt);
}


void senke_t::step(long delta_t)
{
	if(fab==NULL) {
		return;
	}
	if(delta_t==0) {
		return;
	}

	const uint32 power_demand = fab->get_power_demand();
	get_net()->add_demand( power_demand );

	const uint64 net_demand = get_net()->get_demand();
	if(  net_demand > 0  ) {
		power_load = (uint32)((((uint64)last_power_demand) * ((get_net()->get_supply() << 5) / net_demand)) >> 5);
		if(  power_load > last_power_demand  ) {
			power_load = last_power_demand;
		}
		fab->add_power( power_load );
	}
	else {
		power_load = 0;
	}
	fab->add_power_demand( power_demand-power_load ); // allows subsequently stepped senke to supply demand this senke couldn't

	if(  fab->get_besch()->get_electric_amount() == 65535  ){
		// demand not specified in pak, use old fixed demands
		max_einkommen += last_power_demand * delta_t / PRODUCTION_DELTA_T;
		einkommen += power_load  * delta_t / PRODUCTION_DELTA_T;
	}
	else if(  welt->ticks_per_world_month_shift >= 18  ) {
		max_einkommen += (last_power_demand * delta_t / PRODUCTION_DELTA_T) >> (welt->ticks_per_world_month_shift-18);
		einkommen += (power_load  * delta_t / PRODUCTION_DELTA_T) >> (welt->ticks_per_world_month_shift-18);
	}
	else {
		max_einkommen += (last_power_demand * delta_t / PRODUCTION_DELTA_T) << (18-welt->ticks_per_world_month_shift);
		einkommen += (power_load  * delta_t / PRODUCTION_DELTA_T) << (18-welt->ticks_per_world_month_shift);
	}

	if(max_einkommen>(2000<<11)) {
		get_besitzer()->book_revenue(einkommen >> 11, get_pos().get_2d(), powerline_wt);
		einkommen = 0;
		max_einkommen = 1;
	}

	last_power_demand = power_demand;
}


bool senke_t::sync_step(long delta_t)
{
	if(fab==NULL) {
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
		if(  skinverwaltung_t::senke->get_bild_anzahl() > 3  &&  (get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate)  ) {
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
	spieler_t::add_maintenance(get_besitzer(), -welt->get_settings().cst_maintain_transformer, powerline_wt);

	assert(get_net());

	if(  fab==NULL  ) {
		if(welt->lookup(get_pos())->ist_karten_boden()) {
			// on surface, check around
			fab = leitung_t::suche_fab_4(get_pos().get_2d());
		}
		else {
			// underground, check directly above
			fab = fabrik_t::get_fab(welt, get_pos().get_2d());
		}
		if(  fab  ) {
			fab->set_transformer_connected( true );
		}
	}
#if MULTI_THREAD>1
	pthread_mutex_lock( &senke_list_mutex );
#endif
	senke_list.insert( this );
#if MULTI_THREAD>1
	pthread_mutex_unlock( &senke_list_mutex );
	pthread_mutex_lock( &calc_bild_mutex );
#endif
	set_bild(skinverwaltung_t::senke->get_bild_nr(0));
	is_crossing = false;
#if MULTI_THREAD>1
	pthread_mutex_unlock( &calc_bild_mutex );
#endif
}


void senke_t::info(cbuffer_t & buf) const
{
	ding_t::info( buf );

	buf.printf( translator::translate("Net ID: %lu\n"), (unsigned long)get_net() );
	buf.printf( translator::translate("Demand: %u MW\n"), last_power_demand>>POWER_TO_MW );
	buf.printf( translator::translate("Act. load: %u MW\n"), power_load>>POWER_TO_MW );
	buf.printf( translator::translate("Supplied: %u %%"), (100*power_load)/(last_power_demand>0?last_power_demand:1) );
	buf.printf("\n\n"); // pad for consistent dialog size
}
