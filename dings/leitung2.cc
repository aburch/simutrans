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


int
leitung_t::gimme_neighbours(leitung_t **conn)
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



fabrik_t *
leitung_t::suche_fab_4(const koord pos)
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
	set_net(NULL);
	ribi = ribi_t::keine;
	rdwr(file);
}


leitung_t::leitung_t(karte_t *welt, koord3d pos, spieler_t *sp) : ding_t(welt, pos)
{
	set_net(NULL);
	set_besitzer( sp );
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
		spieler_t::add_maintenance(get_besitzer(), -wegbauer_t::leitung_besch->get_wartung());
	}
}



void
leitung_t::entferne(spieler_t *sp)
{
	spieler_t::accounting(sp, -wegbauer_t::leitung_besch->get_preis()/2, get_pos().get_2d(), COST_CONSTRUCTION);
	mark_image_dirty( bild, 0 );
}



/**
 * called during map rotation
 * @author priss
 */
void leitung_t::rotate90()
{
	ding_t::rotate90();
	ribi_t::ribi old_ribi = ribi;
	ribi = ribi_t::rotate90( ribi );

	// determine new image
	// a little complex, since we cannot access the ground right now

	if(bild==IMG_LEER) {
		// most likely on a bridge
		return;
	}

	// first: test for slope
	if(old_ribi==ribi_t::nordsued) {
		if(bild==wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::nord, 0)) {
			bild = wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::ost, 0);
			return;
		}
		else if(bild==wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::sued, 0)) {
			bild = wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::west, 0);
			return;
		}
	}
	else {
		if(bild==wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::west, 0)) {
			bild = wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::nord, 0);
			return;
		}
		else if(bild==wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::ost, 0)) {
			bild = wegbauer_t::leitung_besch->get_hang_bild_nr(hang_t::sued, 0);
			return;
		}
	}

	if(bild != wegbauer_t::leitung_besch->get_bild_nr(old_ribi,0)) {
		// missing mast or crossing graphics are saved here
		if(ribi_t::ist_gerade_ns(old_ribi)) {
			if(bild==wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::nord|ribi_t::ost,0)) {
				// crossing
				bild = wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::sued|ribi_t::ost,0);
			}
			else {
				// missing mast
				bild = wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::sued|ribi_t::west,0);
			}
		}
		else {
			if(bild==wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::sued|ribi_t::ost,0)) {
				// crossing
				bild = wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::nord|ribi_t::ost,0);
			}
			else {
				// missing mast
				bild = wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::nord|ribi_t::west,0);
			}
		}
	}
	else {
		// or just a normal tile ...
		bild = wegbauer_t::leitung_besch->get_bild_nr(ribi,0);
	}
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
void leitung_t::recalc_bild()
{
	is_crossing = false;
	const koord pos = get_pos().get_2d();

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
		set_bild( wegbauer_t::leitung_besch->get_hang_bild_nr(hang, 0));
	}
	else {
		if(gr->hat_wege()) {
			// crossing with road or rail
			weg_t* way = gr->get_weg_nr(0);
			if(ribi_t::ist_gerade_ow(way->get_ribi())) {
				set_bild( wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::nord|ribi_t::ost,0));
			}
			else {
				set_bild( wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::sued|ribi_t::ost,0));
			}
			is_crossing = true;
		}
		else {
			if(ribi_t::ist_gerade(ribi)  &&  !ribi_t::ist_einfach(ribi)  &&  (pos.x+pos.y)&1) {
				// every second skip mast
				if(ribi_t::ist_gerade_ns(ribi)) {
					set_bild( wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::nord|ribi_t::west,0));
				}
				else {
					set_bild( wegbauer_t::leitung_besch->get_diagonal_bild_nr(ribi_t::sued|ribi_t::west,0));
				}
			}
			else {
				set_bild( wegbauer_t::leitung_besch->get_bild_nr(ribi,0));
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
				conn[i]->recalc_bild();
			}
		}
	}
	set_flag( ding_t::dirty );
	recalc_bild();
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
	spieler_t::add_maintenance(get_besitzer(), wegbauer_t::leitung_besch->get_wartung());
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
		file->rdwr_long(value, "\n");
	}
	else {
		file->rdwr_long(value, "\n");
		//      net = powernet_t::load_net((powernet_t *) value);
		set_net(NULL);
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



void pumpe_t::step(long delta_t )
{
	if(fab==NULL) {
		return;
	}

	if ( delta_t == 0 ) {
		return;
	}

	supply = fab->get_power();

	image_id new_bild;
	if ( supply > 0 ) {
		get_net()->add_supply( supply );
		if (skinverwaltung_t::pumpe->get_bild_anzahl() > 3  &&  get_pos().z >= welt->get_snowline()) {
			new_bild = skinverwaltung_t::pumpe->get_bild_nr(3);
		}
		else {
			new_bild = skinverwaltung_t::pumpe->get_bild_nr(1);
		}
	}
	else {
		if (skinverwaltung_t::pumpe->get_bild_anzahl() > 2  &&  get_pos().z >= welt->get_snowline()) {
			new_bild = skinverwaltung_t::pumpe->get_bild_nr(2);
		}
		else {
			new_bild = skinverwaltung_t::pumpe->get_bild_nr(0);
		}
	}
	if(bild!=new_bild) {
		set_flag(ding_t::dirty);
		set_bild( new_bild );
	}
}



void
pumpe_t::laden_abschliessen()
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


senke_t::senke_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
	fab = NULL;
	einkommen = 0;
	max_einkommen = 1;
	next_t = 0;
	delta_sum = 0;
	last_power_demand = 0;
	power_load = 0;
	sp->buche( welt->get_einstellungen()->cst_transformer, get_pos().get_2d(), COST_CONSTRUCTION);
}



senke_t::~senke_t()
{
	if(fab!=NULL) {
		fab->set_prodfaktor( 16 );
		fab->set_transformer_connected( false );
		senke_list.remove( this );
		welt->sync_remove( this );
		fab = NULL;
	}
	spieler_t::add_maintenance(get_besitzer(), welt->get_einstellungen()->cst_maintain_transformer);
}



void senke_t::step(long delta_t)
{
	if(fab==NULL) {
		return;
	}

	if ( delta_t == 0 ) {
		return;
	}

	uint32 power_demand = fab->get_power_demand();
	get_net()->add_demand(power_demand);

	uint32 net_demand = get_net()->get_demand();
	if(  net_demand > 0  ) {
		power_load = (
				last_power_demand 
				* ((get_net()->get_supply() << 5) / net_demand)
			) >> 5;
			// <<5 for max calculation precision fitting withing
			// uint32 with max supply capacity capped in
			// dataobj/powernet.cc max_capacity.
			// This should be fixed to be cleaner.  FIXME.
		if(  power_load > last_power_demand  ) {
			power_load = last_power_demand;
		}
		fab->add_power( power_load );
	}
	else {
		power_load = 0;
	}

	// this allows subsequently stepped senke to supply demand 
	// which this senke couldn't
	fab->add_power_demand( power_demand-power_load ); 

	max_einkommen += last_power_demand;
	einkommen += power_load;

	if(max_einkommen>(2000<<11)) {
		get_besitzer()->buche(einkommen >> 11, get_pos().get_2d(), COST_POWERLINES);
		get_besitzer()->buche(einkommen >> 11, get_pos().get_2d(), COST_INCOME);
		einkommen = 0;
		max_einkommen = 1;
	}

	last_power_demand = power_demand;
}


bool
senke_t::sync_step(long delta_t)
{
	if(fab==NULL) {
		return false;
	}
	bool snow = (skinverwaltung_t::senke->get_bild_anzahl() > 3  &&  get_pos().z >= welt->get_snowline());

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
				if (snow) {
					new_bild = skinverwaltung_t::senke->get_bild_nr(3);
				}
				else {
					new_bild = skinverwaltung_t::senke->get_bild_nr(1);
				}
			}
			else {
				if (snow) {
					new_bild = skinverwaltung_t::senke->get_bild_nr(2);
				}
				else {
					new_bild = skinverwaltung_t::senke->get_bild_nr(0);
				}
			}
		}
		else {
			if (snow) {
				new_bild = skinverwaltung_t::senke->get_bild_nr(2);
			}
			else {
				new_bild = skinverwaltung_t::senke->get_bild_nr(0);
			}
		}
		if(  bild != new_bild  ) {
			set_flag(ding_t::dirty);
			set_bild( new_bild );
		}
	}
	return true;
}



void
senke_t::laden_abschliessen()
{
	leitung_t::laden_abschliessen();
	spieler_t::add_maintenance(get_besitzer(), -welt->get_einstellungen()->cst_maintain_transformer);

	if(fab==NULL  &&  get_net()) {
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
}
