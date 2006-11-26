/*
 * route.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simintr.h"
#include "../simtime.h"
#include "../simmem.h"
#include "../simhalt.h"
#include "../boden/wege/weg.h"
#include "../boden/grund.h"
#include "loadsave.h"
#include "route.h"
#include "umgebung.h"


// if defined, print some profiling informations into the file
//#define DEBUG_ROUTES

// this is WAY slower than the sorted_heap! (also not always finding best routes, but was the original algorithms)
//#include "../tpl/prioqueue_tpl.h" // ~100-250% slower

// HOT queue with the pocket size equal to the distance
// still slower than a binary heap
//#include "../tpl/HOT_queue_tpl.h"	// ~10% slower
//#include "../tpl/HOT_queue2_tpl.h" // ~5% slower

// sorted heap, since we only need insert and pop
//#include "../tpl/sorted_heap_tpl.h" // ~10% slower

// sorted heap, since we only need insert and pop
#include "../tpl/binary_heap_tpl.h" // fastest



void
route_t::kopiere(const route_t *r)
{
	assert(r != NULL);
	const unsigned int hops = r->gib_max_n();
	route.resize(hops+1);
	route.clear();
	for( unsigned int i=0;  i<=hops;  i++ ) {
		route.append(r->route[i]);
	}
}

void
route_t::append(const route_t *r)
{
	assert(r != NULL);
	const unsigned int hops = r->gib_max_n();
	route.resize(hops+1+route.get_count());

DBG_MESSAGE("route_t::append()","last=%i,%i,%i   first=%i,%i,%i",
	route[gib_max_n()].x, route[gib_max_n()].y, route[gib_max_n()].z,
	r->position_bei(0).x, r->position_bei(0).y, r->position_bei(0).z );

	// must be identical
//	assert(gib_max_n() <= 0 || r->position_bei(0) == route[gib_max_n()]);

	while (gib_max_n() >= 0 && route[gib_max_n()] == r->position_bei(0)) {
		// skip identical end tiles
		route.remove_at(gib_max_n());
	}
	// then append
	for( unsigned int i=0;  i<=hops;  i++ ) {
		route.append(r->position_bei(i),16);
	}
}

void
route_t::insert(koord3d k)
{
	if(route.get_size()>=route.get_count()-1) {
		route.resize(route.get_size()+16);
	}
//DBG_MESSAGE("route_t::insert()","insert %d,%d",k.x, k.y);
	route.insert_at(0,k);
}

void
route_t::append(koord3d k)
{
	while (route.get_count() > 1 && route[route.get_count() - 1] == route[route.get_count() - 2]) {
		route.remove_at(route.get_count()-1);
	}
	if (route.get_count() == 0 || k != route[route.get_count() - 1]) {
		route.append(k,16);
	}
	route.append(k,16);	// the last is always double
}


void
route_t::remove_koord_from(int i) {
	while(i<gib_max_n()) {
		route.remove_at(gib_max_n());
	}
	route.append(route[gib_max_n()]); // the last is always double
}



/**
 * Appends a straight line from the last koord3d in route to the desired target.
 * Will return false if failed
 * @author prissi
 */
bool
route_t::append_straight_route(karte_t *welt, koord3d dest )
{
	if(route.get_count()<=1  ||  !welt->ist_in_kartengrenzen(dest.gib_2d())) {
		return false;
	}

	while (route.get_count() > 1 && route[route.get_count() - 2] == route[route.get_count() - 1]) {
		route.remove_at(route.get_count()-1);
	}

	// then try to calculate direct route
	koord pos = route[gib_max_n()].gib_2d();
	const koord ziel=dest.gib_2d();
	route.resize( route.get_count()+abs_distance(pos,ziel)+2 );
DBG_MESSAGE("route_t::append_straight_route()","start from (%i,%i) to (%i,%i)",pos.x,pos.y,dest.x,dest.y);
	while(pos!=ziel) {
		// shortest way
		if(abs(pos.x-ziel.x)>=abs(pos.y-ziel.y)) {
			pos.x += (pos.x>ziel.x) ? -1 : 1;
		}
		else {
			pos.y += (pos.y>ziel.y) ? -1 : 1;
		}
		if(!welt->ist_in_kartengrenzen(pos)) {
			break;
		}
		route.append( welt->lookup(pos)->gib_kartenboden()->gib_pos(),16 );
	}
	route.append(route[route.get_count() - 1], 16);
	DBG_MESSAGE("route_t::append_straight_route()","to (%i,%i) found.",ziel.x,ziel.y);

	return pos==ziel;
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
bool
route_t::find_route(karte_t *welt,
                    const koord3d start,
                    fahrer_t *fahr, const uint32 /*max_khm*/, uint8 start_dir, uint32 max_depth )
{
	bool ok = false;

	// check for existing koordinates
	if(welt->lookup(start)==NULL) {
		return false;
	}

	// some thing for the search
	const waytype_t wegtyp = fahr->gib_waytype();

	// memory in static list ...
	if(nodes==NULL) {
		MAX_STEP = umgebung_t::max_route_steps;
		nodes = new ANode[MAX_STEP];
	}

	INT_CHECK("route 347");

	// arrays for A*
	static vector_tpl<ANode*> open(0);
	vector_tpl<ANode*> close(0);

	// nothing in lists
	open.clear();

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(!fahr->ist_befahrbar(welt->lookup(start))) {
		return false;
	}

	GET_NODE();

	uint32 step = 0;
	ANode* tmp = &nodes[step++];
	tmp->parent = NULL;
	tmp->gr = welt->lookup(start);
	tmp->count = 0;

	// start in open
	open.append(tmp,256);

//DBG_MESSAGE("route_t::find_route()","calc route from %d,%d,%d",start.x, start.y, start.z);
	const grund_t* gr;
	do {
		// Hajo: this is too expensive to be called each step
		if((step & 127) == 0) {
			INT_CHECK("route 161");
		}

		tmp = open[0];
		open.remove_at( 0 );

		close.append(tmp,16);
		gr = tmp->gr;

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,tmp->f);
		// already there
		if(fahr->ist_ziel(gr,tmp->parent==NULL?NULL:tmp->parent->gr)) {
			// we added a target to the closed list: we are finished
			break;
		}

		// testing all four possible directions
		const ribi_t::ribi ribi =  fahr->gib_ribi(gr);
		for(int r=0; r<4; r++) {
			// a way goes here, and it is not marked (i.e. in the closed list)
			grund_t* to;
			if(  (ribi & ribi_t::nsow[r] & start_dir)!=0  // allowed dir (we can restrict the first step by start_dir)
				&& abs_distance(start.gib_2d(),gr->gib_pos().gib_2d()+koord::nsow[r])<max_depth	// not too far away
				&& gr->get_neighbour(to, wegtyp, koord::nsow[r])  // is connected
				&& fahr->ist_befahrbar(to)	// can be driven on
			) {
				unsigned index;

				// already in open list?
				for(  index=0;  index<open.get_count();  index++  ) {
					if (open[index]->gr == to) {
						break;
					}
				}
				// in open list => ignore this
				if(index<open.get_count()) {
					continue;
				}


				// already in closed list (i.e. all processed nodes)
				for( index=0;  index<close.get_count();  index++  ) {
					if (close[index]->gr == to) {
						break;
					}
				}
				// in close list => ignore this
				if(index<close.get_count()) {
					continue;
				}

				// not in there or taken out => add new
				ANode* k = &nodes[step++];

				k->parent = tmp;
				k->gr = to;
				k->count = tmp->count+1;

//DBG_DEBUG("insert to open","%i,%i,%i",to->gib_pos().x,to->gib_pos().y,to->gib_pos().z);
				// insert here
				open.append(k,16);

			}
		}

		// ok, now no more restrains
		start_dir = ribi_t::alle;

	} while(open.get_count()>0  &&  step<MAX_STEP  &&  open.get_count()<max_depth);

	INT_CHECK("route 194");

//DBG_DEBUG("reached","");
	// target reached?
	if(!fahr->ist_ziel(gr,tmp->parent==NULL?NULL:tmp->parent->gr)  ||  step >= MAX_STEP) {
		dbg->warning("route_t::find_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
	}
	else {
		// reached => construct route
		route.clear();
		route.resize(tmp->count+16);
		while(tmp != NULL) {
			route.store_at( tmp->count, tmp->gr->gib_pos() );
//DBG_DEBUG("add","%i,%i",tmp->pos.x,tmp->pos.y);
			tmp = tmp->parent;
		}
		ok = route.get_count()>0;
  }

	RELEASE_NODE();
	return ok;
}



ribi_t::ribi *
get_next_dirs(const koord gr_pos, const koord ziel)
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



bool
route_t::intern_calc_route(karte_t *welt, const koord3d ziel, const koord3d start, fahrer_t *fahr, const uint32 max_speed, const uint32 max_cost)
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
	const waytype_t wegtyp = fahr->gib_waytype();
	const bool is_airplane = fahr->gib_waytype()==air_wt;
	grund_t *to;

	bool ziel_erreicht=false;

	// memory in static list ...
	if(nodes==NULL) {
		MAX_STEP = umgebung_t::max_route_steps;	// may need very much memory => configurable
		nodes = new ANode[MAX_STEP + 4 + 2];
	}

	INT_CHECK("route 347");

	// there are several variant for mantaining the open list
	// however, only binary heap and HOT queue with binary heap are worth considering
#if defined(tpl_HOT_queue_tpl_h)
    static HOT_queue_tpl <ANode *> queue;
#elif defined(tpl_binary_heap_tpl_h)
    static binary_heap_tpl <ANode *> queue;
#elif defined(tpl_sorted_heap_tpl_h)
    static sorted_heap_tpl <ANode *> queue;
#else
    static prioqueue_tpl <ANode *> queue;
#endif

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

	// nothing in lists
	welt->unmarkiere_alle();

	// clear the queue (should be empty anyhow)
	queue.clear();
	queue.insert(tmp);

//DBG_MESSAGE("route_t::itern_calc_route()","calc route from %d,%d,%d to %d,%d,%d",ziel.x, ziel.y, ziel.z, start.x, start.y, start.z);
	uint32 beat=1;
	do {
		// Hajo: this is too expensive to be called each step
		if((beat++ & 255) == 0) {
			INT_CHECK("route 161");
		}

		ANode *test_tmp = queue.pop();

		if(welt->ist_markiert(test_tmp->gr)) {
			// we were already here on a faster route, thus ignore this branch
			// (trading speed against memory consumption)
			continue;
		}

		tmp = test_tmp;
		gr = tmp->gr;
		welt->markiere(gr);

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,tmp->f);

		// we took the target pos out of the closed list
		if(ziel==gr->gib_pos()) {
			ziel_erreicht = true;
			break;
		}

		// testing all four possible directions
		const ribi_t::ribi ribi =  fahr->gib_ribi(gr);
		const ribi_t::ribi *next_ribi = get_next_dirs(gr->gib_pos().gib_2d(),ziel.gib_2d());
		for(int r=0; r<4; r++) {

			// a way in our direction?
			if(  (ribi & next_ribi[r])==0  ) {
				continue;
			}

			to = NULL;
			if(is_airplane) {
				const planquadrat_t *pl=welt->lookup(gr->gib_pos().gib_2d()+koord(next_ribi[r]));
				if(pl) {
					to = pl->gib_kartenboden();
				}
			}

			// a way goes here, and it is not marked (i.e. in the closed list)
			if((to  ||  gr->get_neighbour(to, wegtyp, koord(next_ribi[r]) ))  &&  fahr->ist_befahrbar(to)  &&  !welt->ist_markiert(to)) {

				// Do not go on a tile, where a oneway sign forbids going.
				// This saves time and fixed the bug, that a oneway sign on the final tile was ignored.
				ribi_t::ribi last_dir=next_ribi[r];
				weg_t *w=to->gib_weg(wegtyp);
				ribi_t::ribi go_dir = (w==NULL) ? 0 : w->gib_ribi_maske();
				if((last_dir&go_dir)!=0) {
						continue;
				}

				// new values for cost g
				uint32 new_g = tmp->g + fahr->gib_kosten(to,max_speed);

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if(tmp->parent!=NULL) {
					current_dir = ribi_typ( tmp->parent->gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
					if(tmp->dir!=current_dir) {
						new_g += 3;
						if(tmp->parent->dir!=tmp->dir) {
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
					 current_dir = ribi_typ( gr->gib_pos().gib_2d(), to->gib_pos().gib_2d() );
				}

				const uint32 new_f = new_g + calc_distance( to->gib_pos(), ziel );

				// add new
				ANode* k = &nodes[step];
				step ++;

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_f;
				k->dir = current_dir;
				k->count = tmp->count+1;

				queue.insert( k );
			}
		}

	} while (!queue.empty() && !ziel_erreicht && step < MAX_STEP && tmp->g < max_cost);

#ifdef DEBUG_ROUTES
	// display marked route
	reliefkarte_t::gib_karte()->calc_map();
#endif

	INT_CHECK("route 194");
#ifdef DEBUG_ROUTES
DBG_DEBUG("route_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u (max %u)",step,MAX_STEP,queue.count(),tmp->g,max_cost);
#endif
	// target reached?
	if(!ziel_erreicht  || step >= MAX_STEP  ||  tmp->parent==NULL) {
		dbg->warning("route_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
	}
	else {
		// reached => construct route
		route.clear();
		route.resize(tmp->count+16);
		while(tmp != NULL) {
			route.store_at( tmp->count, tmp->gr->gib_pos() );
//DBG_DEBUG("add","%i,%i at pos %i",tmp->gr->gib_pos().x,tmp->gr->gib_pos().y,tmp->count);
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
bool
route_t::calc_route(karte_t *welt,
                    const koord3d ziel, const koord3d start,
                    fahrer_t *fahr, const uint32 max_khm, const uint32 max_cost)
{
	route.clear();

	INT_CHECK("route 336");

#ifdef DEBUG_ROUTES
	// profiling for routes ...
	long ms=get_current_time_millis();
#endif
	bool ok = intern_calc_route(welt, start, ziel, fahr, max_khm,max_cost);
#ifdef DEBUG_ROUTES
	if(fahr->gib_waytype()==water_wt) {DBG_DEBUG("route_t::calc_route()","route from %d,%d to %d,%d with %i steps in %u ms found.",start.x, start.y, ziel.x, ziel.y, route.get_count()-2, get_current_time_millis()-ms );}
#endif

	INT_CHECK("route 343");

	if( !ok ) {
DBG_MESSAGE("route_t::calc_route()","No route from %d,%d to %d,%d found",start.x, start.y, ziel.x, ziel.y);
		// no route found
		route.resize(1);
		route.append(start);	// just to be save
		return false;

	}
	else {
		// drive to the end in a station
		halthandle_t halt = welt->lookup(start)->gib_halt();

		// only needed for stations: go to the very end
		if(halt.is_bound()) {

			// does only make sence for trains
			if(fahr->gib_waytype()==track_wt  ||  fahr->gib_waytype()==monorail_wt  ||  fahr->gib_waytype()==tram_wt) {

				int max_n = route.get_count()-1;

				const koord zv = route[max_n].gib_2d() - route[max_n - 1].gib_2d();
//DBG_DEBUG("route_t::calc_route()","zv=%i,%i",zv.x,zv.y);

				const int ribi = ribi_typ(zv);//fahr->gib_ribi(welt->lookup(start));
				grund_t *gr=welt->lookup(start);
				const waytype_t wegtyp=fahr->gib_waytype();

				while(gr->get_neighbour(gr,wegtyp,zv)  &&  gr->gib_halt() == halt  &&   fahr->ist_befahrbar(gr)   &&  (fahr->gib_ribi(gr)&&ribi)!=0) {
					// stop at end of track! (prissi)

					// Do not go on a tile, where a oneway sign forbids going.
					// This saves time and fixed the bug, that a oneway sign on the finaly tile was ignored.
					ribi_t::ribi go_dir=gr->gib_weg(wegtyp)->gib_ribi_maske();
					if((ribi&go_dir)!=0) {
						break;
					}
//DBG_DEBUG("route_t::calc_route()","add station at %i,%i",gr->gib_pos().x,gr->gib_pos().y);
					route.append(gr->gib_pos(),12);
				}

			}
		}
	}
	return true;
}




void
route_t::rdwr(loadsave_t *file)
{
	int max_n = route.get_count()-1;
	int i;

	file->rdwr_long(max_n, "\n");
	if(file->is_loading()) {
		koord3d k;
		route.clear();
		route.resize(max_n+2);
		for(i=0;  i<=max_n;  i++ ) {
			k.rdwr(file);
			route.append( k );
		}
	}
	else {
		// writing
		for(i=0; i<=max_n; i++) {
			route[i].rdwr(file);
		}
	}
}
