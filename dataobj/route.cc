/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simintr.h"
#include "../simhalt.h"
#include "../boden/wege/weg.h"
#include "../boden/grund.h"
#include "../ifc/fahrer.h"
#include "loadsave.h"
#include "route.h"
#include "umgebung.h"


// if defined, print some profiling informations into the file
//#define DEBUG_ROUTES

// binary heap, the fastest
#include "../tpl/binary_heap_tpl.h"


#ifdef DEBUG_ROUTES
#include "../simsys.h"
#endif



void route_t::kopiere(const route_t *r)
{
	assert(r != NULL);
	const unsigned int hops = r->get_count()-1;
	route.clear();
	route.resize(hops + 1);
	for( unsigned int i=0;  i<=hops;  i++ ) {
		route.append(r->route[i]);
	}
}

void route_t::append(const route_t *r)
{
	assert(r != NULL);
	const uint32 hops = r->get_count()-1;
	route.resize(hops+1+route.get_count());

	while (!route.empty() && back() == r->front()) {
		// skip identical end tiles
		route.pop_back();
	}
	// then append
	for( unsigned int i=0;  i<=hops;  i++ ) {
		route.append(r->position_bei(i));
	}
}


void route_t::insert(koord3d k)
{
	route.insert_at(0,k);
}


void route_t::remove_koord_from(uint32 i) {
	while(  i+1 < get_count()  ) {
		route.pop_back();
	}
}



/**
 * Appends a straight line from the last koord3d in route to the desired target.
 * Will return false if failed
 * @author prissi
 */
bool route_t::append_straight_route(karte_t *welt, koord3d dest )
{
	if(  !welt->is_within_limits(dest.get_2d())  ) {
		return false;
	}

	// then try to calculate direct route
	koord pos = back().get_2d();
	const koord ziel=dest.get_2d();
	route.resize( route.get_count()+koord_distance(pos,ziel)+2 );
DBG_MESSAGE("route_t::append_straight_route()","start from (%i,%i) to (%i,%i)",pos.x,pos.y,dest.x,dest.y);
	while(pos!=ziel) {
		// shortest way
		if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
			pos.x += (pos.x>ziel.x) ? -1 : 1;
		}
		else {
			pos.y += (pos.y>ziel.y) ? -1 : 1;
		}
		if(!welt->is_within_limits(pos)) {
			break;
		}
		route.append(welt->lookup_kartenboden(pos)->get_pos());
	}
	DBG_MESSAGE("route_t::append_straight_route()","to (%i,%i) found.",ziel.x,ziel.y);

	return pos==ziel;
}


static bool is_in_list(vector_tpl<route_t::ANode*> const& list, grund_t const* const to)
{
	FOR(vector_tpl<route_t::ANode*>, const i, list) {
		if (i->gr == to) {
			return true;
		}
	}
	return false;
}


// node arrays
route_t::ANode* route_t::nodes=NULL;
uint32 route_t::MAX_STEP=0;
#ifdef DEBUG
bool route_t::node_in_use=false;
#endif

/* find the route to an unknow location
 * @author prissi
 */
bool route_t::find_route(karte_t *welt, const koord3d start, fahrer_t *fahr, const uint32 /*max_khm*/, uint8 start_dir, uint32 max_depth )
{
	bool ok = false;

	// check for existing koordinates
	const grund_t* g = welt->lookup(start);
	if (g == NULL) {
		return false;
	}

	// some thing for the search
	const waytype_t wegtyp = fahr->get_waytype();

	// memory in static list ...
	if(nodes==NULL) {
		MAX_STEP = welt->get_settings().get_max_route_steps();
		nodes = new ANode[MAX_STEP];
	}

	INT_CHECK("route 347");

	// arrays for A*
	static vector_tpl<ANode*> open;
	vector_tpl<ANode*> close;

	// nothing in lists
	open.clear();

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(  !fahr->ist_befahrbar(g)  ) {
		return false;
	}

	GET_NODE();

	uint32 step = 0;
	ANode* tmp = &nodes[step++];
	tmp->parent = NULL;
	tmp->gr = g;
	tmp->count = 0;

	// start in open
	open.append(tmp);

//DBG_MESSAGE("route_t::find_route()","calc route from %d,%d,%d",start.x, start.y, start.z);
	const grund_t* gr;
	do {
		// Hajo: this is too expensive to be called each step
		if((step & 127) == 0) {
			INT_CHECK("route 161");
		}

		tmp = open[0];
		open.remove_at( 0 );

		close.append(tmp);
		gr = tmp->gr;

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->get_pos().x,gr->get_pos().y,gr->get_pos().z,tmp->f);
		// already there
		if(  fahr->ist_ziel( gr, tmp->parent==NULL ? NULL : tmp->parent->gr )  ) {
			// we added a target to the closed list: check for length
			break;
		}

		// testing all four possible directions
		const ribi_t::ribi ribi =  fahr->get_ribi(gr);
		for(  int r=0;  r<4;  r++  ) {
			// a way goes here, and it is not marked (i.e. in the closed list)
			grund_t* to;
			if(  (ribi & ribi_t::nsow[r] & start_dir)!=0  // allowed dir (we can restrict the first step by start_dir)
				&& koord_distance(start.get_2d(),gr->get_pos().get_2d()+koord::nsow[r])<max_depth	// not too far away
				&& gr->get_neighbour(to, wegtyp, ribi_t::nsow[r])  // is connected
				&& fahr->ist_befahrbar(to)	// can be driven on
			) {
				// already in open list?
				if (is_in_list(open,  to)) {
					continue;
				}

				// already in closed list (i.e. all processed nodes)
				if (is_in_list(close, to)) {
					continue;
				}

				// not in there or taken out => add new
				ANode* k = &nodes[step++];

				k->parent = tmp;
				k->gr = to;
				k->count = tmp->count+1;

//DBG_DEBUG("insert to open","%i,%i,%i",to->get_pos().x,to->get_pos().y,to->get_pos().z);
				// insert here
				open.append(k);
			}
		}

		// ok, now no more restrains
		start_dir = ribi_t::alle;

	} while(  !open.empty()  &&  step < MAX_STEP  &&  open.get_count() < max_depth  );

	INT_CHECK("route 194");

//DBG_DEBUG("reached","");
	// target reached?
	if(!fahr->ist_ziel(gr,tmp->parent==NULL?NULL:tmp->parent->gr)  ||  step >= MAX_STEP) {
		if(  step >= MAX_STEP  ) {
			dbg->warning("route_t::find_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
		}
	}
	else {
		// reached => construct route
		route.store_at( tmp->count, tmp->gr->get_pos() );
		while(tmp != NULL) {
			route[ tmp->count ] = tmp->gr->get_pos();
			tmp = tmp->parent;
		}
		ok = !route.empty();
  }

	RELEASE_NODE();
	return ok;
}



ribi_t::ribi *get_next_dirs(const koord gr_pos, const koord ziel)
{
	static ribi_t::ribi next_ribi[4];
	if( abs(gr_pos.x-ziel.x)>abs(gr_pos.y-ziel.y) ) {
		next_ribi[0] = (ziel.x>gr_pos.x) ? ribi_t::ost : ribi_t::west;
		next_ribi[1] = (ziel.y>gr_pos.y) ? ribi_t::sued : ribi_t::nord;
	}
	else {
		next_ribi[0] = (ziel.y>gr_pos.y) ? ribi_t::sued : ribi_t::nord;
		next_ribi[1] = (ziel.x>gr_pos.x) ? ribi_t::ost : ribi_t::west;
	}
	next_ribi[2] = ribi_t::rueckwaerts( next_ribi[1] );
	next_ribi[3] = ribi_t::rueckwaerts( next_ribi[0] );
	return next_ribi;
}



bool route_t::intern_calc_route(karte_t *welt, const koord3d ziel, const koord3d start, fahrer_t *fahr, const sint32 max_speed, const uint32 max_cost)
{
	bool ok = false;

	// check for existing koordinates
	const grund_t *gr=welt->lookup(start);
	if(gr==NULL  ||  welt->lookup(ziel)==NULL) {
		return false;
	}

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(!fahr->ist_befahrbar(gr)) {
		return false;
	}

	// some thing for the search
	const waytype_t wegtyp = fahr->get_waytype();
	const bool is_airplane = fahr->get_waytype()==air_wt;
	grund_t *to;

	bool ziel_erreicht=false;

	// memory in static list ...
	if(nodes==NULL) {
		MAX_STEP = welt->get_settings().get_max_route_steps(); // may need very much memory => configurable
		nodes = new ANode[MAX_STEP + 4 + 2];
	}

	INT_CHECK("route 347");

	static binary_heap_tpl <ANode *> queue;

	GET_NODE();

	uint32 step = 0;
	ANode* tmp = &nodes[step];
	step ++;

	tmp->parent = NULL;
	tmp->gr = welt->lookup(start);
	tmp->f = calc_distance(start,ziel);
	tmp->g = 0;
	tmp->dir = 0;
	tmp->count = 0;
	tmp->ribi_from = ribi_t::alle;

	// nothing in lists
	welt->unmarkiere_alle();

	// clear the queue (should be empty anyhow)
	queue.clear();
	queue.insert(tmp);
	ANode* new_top = NULL;

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d,%d to %d,%d,%d",ziel.x, ziel.y, ziel.z, start.x, start.y, start.z);
	uint32 beat=1;
	do {
		// Hajo: this is too expensive to be called each step
		if((beat++ & 255) == 0) {
			INT_CHECK("route 161");
		}

		if (new_top) {
			// this is not in closed list, no check necessary
			tmp = new_top;
			new_top = NULL;
		}
		else {
			tmp = queue.pop();
			if(welt->ist_markiert(tmp->gr)) {
				// we were already here on a faster route, thus ignore this branch
				// (trading speed against memory consumption)
				continue;
			}
		}

		gr = tmp->gr;
		welt->markiere(gr);

		// we took the target pos out of the closed list
		if(ziel==gr->get_pos()) {
			ziel_erreicht = true;
			break;
		}

		uint32 topnode_g = !queue.empty() ? queue.front()->g : max_cost;

		// testing all four possible directions
		// mask direction we came from
		const ribi_t::ribi ribi =  fahr->get_ribi(gr)  &  ( ~ribi_t::rueckwaerts(tmp->ribi_from) );

		const ribi_t::ribi *next_ribi = get_next_dirs(gr->get_pos().get_2d(),ziel.get_2d());
		for(int r=0; r<4; r++) {

			// a way in our direction?
			if(  (ribi & next_ribi[r])==0  ) {
				continue;
			}

			to = NULL;
			if(is_airplane) {
				const planquadrat_t *pl=welt->lookup(gr->get_pos().get_2d()+koord(next_ribi[r]));
				if(pl) {
					to = pl->get_kartenboden();
				}
			}

			// a way goes here, and it is not marked (i.e. in the closed list)
			if((to  ||  gr->get_neighbour(to, wegtyp, next_ribi[r]))  &&  fahr->ist_befahrbar(to)  &&  !welt->ist_markiert(to)) {

				// Do not go on a tile, where a oneway sign forbids going.
				// This saves time and fixed the bug, that a oneway sign on the final tile was ignored.
				ribi_t::ribi last_dir=next_ribi[r];
				weg_t *w=to->get_weg(wegtyp);
				ribi_t::ribi go_dir = (w==NULL) ? 0 : w->get_ribi_maske();
				if((last_dir&go_dir)!=0) {
						continue;
				}

				// new values for cost g (without way it is either in the air or in water => no costs)
				uint32 new_g = tmp->g + (w ? fahr->get_kosten(to, max_speed, tmp->gr->get_pos().get_2d()) : 1);

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if(tmp->parent!=NULL) {
					current_dir = ribi_typ( tmp->parent->gr->get_pos().get_2d(), to->get_pos().get_2d() );
					if(tmp->dir!=current_dir) {
						new_g += 3;
						if(tmp->parent->dir!=tmp->dir  &&  tmp->parent->parent!=NULL) {
							// discourage 90° turns
							new_g += 10;
						}
						else if(ribi_t::ist_exakt_orthogonal(tmp->dir,current_dir)) {
							// discourage v turns heavily
							new_g += 25;
						}
					}

				}
				else {
					current_dir = ribi_typ( gr->get_pos().get_2d(), to->get_pos().get_2d() );
				}

				const uint32 new_f = new_g + calc_distance( to->get_pos(), ziel );

				// add new
				ANode* k = &nodes[step];
				step ++;

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_f;
				k->dir = current_dir;
				k->ribi_from = next_ribi[r];
				k->count = tmp->count+1;

				if (new_g <= topnode_g) {
					// do not put in queue if the new node is the best one
					topnode_g = new_g;
					if (new_top) {
						queue.insert(new_top);
					}
					new_top = k;
				}
				else {
					queue.insert( k );
				}
			}
		}

	} while (  (!queue.empty() ||  new_top) && step < MAX_STEP && tmp->g < max_cost);

#ifdef DEBUG_ROUTES
	// display marked route
	//reliefkarte_t::get_karte()->calc_map();
	DBG_DEBUG("route_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u (max %u)",step,MAX_STEP,queue.get_count(),tmp->g,max_cost);
#endif

	INT_CHECK("route 194");
	// target reached?
	if(!ziel_erreicht  || step >= MAX_STEP  ||  tmp->parent==NULL) {
		if(  step >= MAX_STEP  ) {
			dbg->warning("route_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
		}
	}
	else {
		// reached => construct route
		route.store_at( tmp->count, tmp->gr->get_pos() );
		while(tmp != NULL) {
			route[ tmp->count ] = tmp->gr->get_pos();
			tmp = tmp->parent;
		}
		ok = true;
	}

	RELEASE_NODE();

	return ok;
}



/* searches route, uses intern_calc_route() for distance between stations
 * handles only driving in stations by itself
 * corrected 12/2005 for station search
 * @author Hansjörg Malthaner, prissi
 */
route_t::route_result_t route_t::calc_route(karte_t *welt, const koord3d ziel, const koord3d start, fahrer_t *fahr, const sint32 max_khm, sint32 max_len )
{
	route.clear();

	INT_CHECK("route 336");

#ifdef DEBUG_ROUTES
	// profiling for routes ...
	long ms=dr_time();
#endif
	bool ok = intern_calc_route(welt, start, ziel, fahr, max_khm, 0xFFFFFFFFul );
#ifdef DEBUG_ROUTES
	if(fahr->get_waytype()==water_wt) {DBG_DEBUG("route_t::calc_route()","route from %d,%d to %d,%d with %i steps in %u ms found.",start.x, start.y, ziel.x, ziel.y, route.get_count()-1, dr_time()-ms );}
#endif

	INT_CHECK("route 343");

	if( !ok ) {
DBG_MESSAGE("route_t::calc_route()","No route from %d,%d to %d,%d found",start.x, start.y, ziel.x, ziel.y);
		// no route found
		route.resize(1);
		route.append(start); // just to be safe
		return no_route;
	}
	// advance so all convoi fits into a halt (only set for trains and cars)
	else if(  max_len>1  ) {

		// we need a halt of course ...
		halthandle_t halt = welt->lookup(start)->get_halt();
		if(  halt.is_bound()  ) {

			// first: find out how many tiles I am already in the station
			for(  size_t i = route.get_count();  i-- != 0  &&  max_len != 0  &&  halt == haltestelle_t::get_halt(welt, route[i], NULL);  --max_len) {
			}

			// and now go forward, if possible
			if(  max_len>0  ) {

				const uint32 max_n = route.get_count()-1;
				const koord zv = route[max_n].get_2d() - route[max_n - 1].get_2d();
				const int ribi = ribi_typ(zv);//fahr->get_ribi(welt->lookup(start));

				grund_t *gr = welt->lookup(start);
				const waytype_t wegtyp=fahr->get_waytype();

				while(  max_len>0  &&  gr->get_neighbour(gr,wegtyp,ribi)  &&  gr->get_halt()==halt  &&   fahr->ist_befahrbar(gr)   &&  (fahr->get_ribi(gr)&&ribi)!=0  ) {
					// Do not go on a tile, where a oneway sign forbids going.
					// This saves time and fixed the bug, that a oneway sign on the finaly tile was ignored.
					ribi_t::ribi go_dir=gr->get_weg(wegtyp)->get_ribi_maske();
					if(  (ribi&go_dir)!=0  ) {
						break;
					}
					route.append(gr->get_pos());
					max_len--;
				}
				// station too short => warning!
				if(  max_len>0  ) {
					return valid_route_halt_too_short;
				}
			}
		}
	}
	return valid_route;
}




void route_t::rdwr(loadsave_t *file)
{
	xml_tag_t r( file, "route_t" );
	sint32 max_n = route.get_count()-1;

	file->rdwr_long(max_n);
	if(file->is_loading()) {
		koord3d k;
		route.clear();
		route.resize(max_n+2);
		for(sint32 i=0;  i<=max_n;  i++ ) {
			k.rdwr(file);
			route.append(k);
		}
	}
	else {
		// writing
		for(sint32 i=0; i<=max_n; i++) {
			route[i].rdwr(file);
		}
	}
}
