/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>

#include "../world/simworld.h"
#include "../simintr.h"
#include "../simhalt.h"
#include "../obj/way/weg.h"
#include "../ground/grund.h"
#include "../ground/wasser.h"
#include "../dataobj/marker.h"
#include "../vehicle/simtestdriver.h"
#include "loadsave.h"
#include "route.h"
#include "environment.h"

// define USE_VALGRIND_MEMCHECK to make
// valgrind aware of the memory pool for A* nodes
#ifdef USE_VALGRIND_MEMCHECK
#include <valgrind/memcheck.h>
#endif

// if defined, print some profiling informations into the file
//#define DEBUG_ROUTES

// binary heap, the fastest
#include "../tpl/binary_heap_tpl.h"


#ifdef DEBUG_ROUTES
#include "../sys/simsys.h"
#endif


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
		route.append(r->at(i));
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
 */
bool route_t::append_straight_route(karte_t *welt, koord3d dest )
{
	const koord ziel=dest.get_2d();

	if(  !welt->is_within_limits(ziel)  ) {
		return false;
	}

	// then try to calculate direct route
	koord pos = back().get_2d();
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



// node arrays
route_t::ANode* route_t::nodes=NULL;
uint32 route_t::MAX_STEP=0;
#ifdef DEBUG
bool route_t::node_in_use=false;
#endif

/**
 * find the route to an unknown location
 */
bool route_t::find_route(karte_t *welt, const koord3d start, test_driver_t *tdriver, const uint32 max_khm, uint8 start_dir, uint32 max_depth )
{
	bool ok = false;

	// check for existing koordinates
	const grund_t* g = welt->lookup(start);
	if(  g == NULL  ) {
		return false;
	}

	// some thing for the search
	const waytype_t wegtyp = tdriver->get_waytype();

	// memory in static list ...
	if(  nodes == NULL  ) {
		MAX_STEP = welt->get_settings().get_max_route_steps();
		nodes = new ANode[MAX_STEP];
	}

	INT_CHECK("route 347");

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(  !tdriver->check_next_tile(g)  ) {
		return false;
	}

	static binary_heap_tpl <ANode *> queue;

	GET_NODE();
#ifdef USE_VALGRIND_MEMCHECK
	VALGRIND_MAKE_MEM_UNDEFINED(nodes, sizeof(ANode)*MAX_STEP);
#endif


	uint32 step = 0;
	ANode* tmp = &nodes[step++];
	tmp->parent = NULL;
	tmp->gr = g;
	tmp->count = 0;
	tmp->f = 0;
	tmp->g = 0;
	tmp->dir = 0;

	assert(start_dir == ribi_t::all  ||  ribi_t::is_single(start_dir) );
	// start_dir is direction to start with, adjust ribi_from such that bit mask boiles down to start_dir
	tmp->ribi_from = ribi_t::reverse_single(start_dir) ^ 0x0f;
	// assert that mask in first step is equal to start_dir
	assert( (uint8)(~ribi_t::reverse_single(tmp->ribi_from)& 0xf)  == start_dir);

	// nothing in lists
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y);

	queue.clear();
	queue.insert(tmp);

	bool target_reached = false;
	do {
		// this is too expensive to be called each step
		if((step & 4095) == 0) {
			INT_CHECK("route 161");
		}

		tmp = queue.pop();
		const grund_t* gr = tmp->gr;

		if(  marker.test_and_mark(gr)  ) {
			// we were already here on a faster route, thus ignore this branch
			// (trading speed against memory consumption)
			continue;
		}
		// tile is marked as visited


		// already there
		if(  tdriver->is_target( gr, tmp->parent==NULL ? NULL : tmp->parent->gr )  ) {
			// we added a target to the closed list: check for length
			target_reached = true;
			break;
		}

		// testing all four possible directions
		const ribi_t::ribi ribi =  tdriver->get_ribi(gr)  &  ( ~ribi_t::reverse_single(tmp->ribi_from) );
		for(  int r=0;  r<4;  r++  ) {
			// a way goes here, and it is not marked (i.e. in the closed list)
			grund_t* to = NULL;
			if(  (ribi & ribi_t::nesw[r] )!=0 // do not go backwards
			    && koord_distance(start, gr->get_pos() + koord::nesw[r])<max_depth // not too far away
			    && gr->get_neighbour(to, wegtyp, ribi_t::nesw[r])  // is connected
			    && !marker.is_marked(to) // not already tested
			    && tdriver->check_next_tile(to) // can be driven on
				&& step < MAX_STEP // do not overrun nodes[]
			) {
				// not in there or taken out => add new
				ANode* k = &nodes[step++];

				k->parent = tmp;
				k->gr = to;
				k->count = tmp->count+1;
				k->f = 0;
				k->g = tmp->g + tdriver->get_cost(to, to->get_weg(wegtyp), max_khm, ribi_t::nesw[r]);
				k->ribi_from = ribi_t::nesw[r];

				uint8 current_dir = ribi_t::nesw[r];
				if(tmp->parent!=NULL) {
					current_dir |= tmp->ribi_from;
					if(tmp->dir!=current_dir) {
						k->g += 3;
						if(tmp->parent->dir!=tmp->dir  &&  tmp->parent->parent!=NULL) {
							// discourage 90 degree turns
							k->g += 10;
						}
						else if(ribi_t::is_perpendicular(tmp->dir,current_dir)) {
							// discourage v turns heavily
							k->g += 25;
						}
					}
				}
				k->dir = current_dir;

				// insert here
				queue.insert(k);
			}
		}

	} while(  !queue.empty()  &&  step < MAX_STEP  &&  queue.get_count() < max_depth  );

	INT_CHECK("route 194");

	// target reached?
	if(!target_reached  ||  step >= MAX_STEP) {
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



ribi_t::ribi *get_next_dirs(const koord3d& gr_pos, const koord3d& ziel)
{
	static ribi_t::ribi next_ribi[4];
	if( abs(gr_pos.x-ziel.x)>abs(gr_pos.y-ziel.y) ) {
		next_ribi[0] = (ziel.x>gr_pos.x) ? ribi_t::east : ribi_t::west;
		next_ribi[1] = (ziel.y>gr_pos.y) ? ribi_t::south : ribi_t::north;
	}
	else {
		next_ribi[0] = (ziel.y>gr_pos.y) ? ribi_t::south : ribi_t::north;
		next_ribi[1] = (ziel.x>gr_pos.x) ? ribi_t::east : ribi_t::west;
	}
	next_ribi[2] = ribi_t::reverse_single( next_ribi[1] );
	next_ribi[3] = ribi_t::reverse_single( next_ribi[0] );
	return next_ribi;
}



bool route_t::intern_calc_route(karte_t *welt, const koord3d ziel, const koord3d start, test_driver_t *tdriver, const sint32 max_speed, const uint32 max_cost)
{
	bool ok = false;

	// check for existing koordinates
	const grund_t *gr=welt->lookup(start);
	if(  gr == NULL  ||  welt->lookup(ziel) == NULL) {
		return false;
	}

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(  !tdriver->check_next_tile(gr)  ) {
		return false;
	}

	// some thing for the search
	const waytype_t wegtyp = tdriver->get_waytype();
	const bool is_airplane = tdriver->get_waytype()==air_wt;
	const uint32 cost_upslope = tdriver->get_cost_upslope();

	/* On water we will try jump point search (jps):
	 * - If going straight do not turn, only if near an obstacle.
	 * - If going diagonally only proceed in the two directions defining the diagonal.
	 * Ideally, no water tile is visited twice.
	 * Needs postprocessing to eliminate unnecessary turns.
	 *
	 * Reference:
	 *  Harabor D. and Grastien A. 2011. Online Graph Pruning for Pathfinding on Grid Maps.
	 *  In Proceedings of the 25th National Conference on Artificial Intelligence (AAAI), San Francisco, USA.
	 *  https://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-aaai11.pdf
	 */
	const bool use_jps     = tdriver->get_waytype()==water_wt;

	bool ziel_erreicht=false;

	// memory in static list ...
	if(  nodes == NULL  ) {
		MAX_STEP = welt->get_settings().get_max_route_steps(); // may need very much memory => configurable
		nodes = new ANode[MAX_STEP + 4 + 2];
	}

	INT_CHECK("route 347");

	static binary_heap_tpl <ANode *> queue;

	GET_NODE();
#ifdef USE_VALGRIND_MEMCHECK
	VALGRIND_MAKE_MEM_UNDEFINED(nodes, sizeof(ANode)*MAX_STEP);
#endif

	uint32 step = 0;
	ANode* tmp = &nodes[step];
	step ++;

	tmp->parent = NULL;
	tmp->gr = gr;
	tmp->f = calc_distance(start,ziel);
	tmp->g = 0;
	tmp->dir = 0;
	tmp->count = 0;
	tmp->ribi_from = ribi_t::none;
	tmp->jps_ribi  = ribi_t::all;

	// nothing in lists
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y);

	// clear the queue (should be empty anyhow)
	queue.clear();
	queue.insert(tmp);
	ANode* new_top = NULL;

	uint32 beat=1;
	do {
		// this is too expensive to be called each step
		if((beat++ & 4095) == 0) {
			INT_CHECK("route 161");
		}

		if (new_top) {
			// this is not in closed list, no check necessary
			tmp = new_top;
			new_top = NULL;
			gr = tmp->gr;
			marker.mark(gr);
		}
		else {
			tmp = queue.pop();
			gr = tmp->gr;
			if(marker.test_and_mark(gr)) {
				// we were already here on a faster route, thus ignore this branch
				// (trading speed against memory consumption)
				continue;
			}
		}

		// we took the target pos out of the closed list
		if(  ziel == gr->get_pos()  ) {
			ziel_erreicht = true;
			break;
		}

		uint32 topnode_f = !queue.empty() ? queue.front()->f : max_cost;

		const ribi_t::ribi way_ribi =  tdriver->get_ribi(gr);
		// testing all four possible directions
		// mask direction we came from
		const ribi_t::ribi ribi =  way_ribi  &  ( ~ribi_t::reverse_single(tmp->ribi_from) )  &  tmp->jps_ribi;

		const ribi_t::ribi *next_ribi = get_next_dirs(gr->get_pos(), ziel);
		for(int r=0; r<4; r++) {

			// a way in our direction?
			if(  (ribi & next_ribi[r])==0  ) {
				continue;
			}

			grund_t* to = NULL;
			if(is_airplane) {
				to = welt->lookup_kartenboden(gr->get_pos().get_2d()+koord(next_ribi[r]));
			}

			// a way goes here, and it is not marked (i.e. in the closed list)
			if((to  ||  gr->get_neighbour(to, wegtyp, next_ribi[r]))  &&  tdriver->check_next_tile(to)  &&  !marker.is_marked(to)) {

				weg_t *w = to->get_weg(wegtyp);
				// Do not go on a tile, where a oneway sign forbids going.
				// This saves time and fixed the bug, that a oneway sign on the final tile was ignored.
				if (w  &&  w->get_ribi_maske()  &&  ribi_t::reverse_single(next_ribi[r]) == w->get_ribi()) {
					// there is a signal, and the only direction leaving the next tile
					// is back to our position
					continue;
				}

				// new values for cost g (without way it is either in the air or in water => no costs)
				uint32 new_g = tmp->g + (w ? tdriver->get_cost(to, w, max_speed, next_ribi[r]) : 1);

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if(tmp->parent!=NULL) {
					current_dir = next_ribi[r] | tmp->ribi_from;
					if(tmp->dir!=current_dir) {
						new_g += 3;
						if(tmp->parent->dir!=tmp->dir  &&  tmp->parent->parent!=NULL) {
							// discourage 90 degree turns
							new_g += 10;
						}
						else if(ribi_t::is_perpendicular(tmp->dir,current_dir)) {
							// discourage v turns heavily
							new_g += 25;
						}
					}

				}
				else {
					current_dir = next_ribi[r];
				}

				uint32 dist = calc_distance( to->get_pos(), ziel );

				// count how many 45 degree turns are necessary to get to target
				sint8 turns = 0;
				if (dist>1) {
					ribi_t::ribi to_target = ribi_type(to->get_pos(), ziel );

					if (to_target  &&  (to_target!=current_dir)) {
						if (ribi_t::is_single(current_dir) != ribi_t::is_single(to_target)) {
							to_target = ribi_t::rotate45(to_target);
							turns ++;
						}
						while(to_target!=current_dir) {
							to_target = ribi_t::rotate90(to_target);
							turns +=2;
						}
						if (turns>4) turns = 8-turns;
					}
				}
				// add 3*turns to the heuristic bound

				// take height difference into account when calculating distance
				uint32 costup = 0;
				if (cost_upslope) {
					costup = cost_upslope * max(ziel.z - to->get_vmove(next_ribi[r]), 0);
				}

				const uint32 new_f = new_g + dist + turns * 3 + costup;

				if (step >= MAX_STEP) {
					break; // Do not overrun nodes[]
				}

				// add new
				ANode* k = &nodes[step++];

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_f;
				k->dir = current_dir;
				k->ribi_from = next_ribi[r];
				k->count = tmp->count+1;
				k->jps_ribi = ribi_t::all;

				if (use_jps  &&  to->is_water()) {
					// only check previous direction plus directions not available on this tile
					// if going straight only check straight direction
					// if going diagonally check both directions that generate this diagonal
					// also enter all available canals and turn to get around canals
					if (tmp->parent!=NULL) {
						k->jps_ribi = ~way_ribi | current_dir |  ((wasser_t*)to)->get_canal_ribi();

						if (gr->is_water()) {
							// turn on next tile to enter possible neighbours of canal tiles
							k->jps_ribi |= ((const wasser_t*)gr)->get_canal_ribi();
						}
					}
				}

				if(  new_f <= topnode_f  ) {
					// do not put in queue if the new node is the best one
					topnode_f = new_f;
					if(  new_top  ) {
						queue.insert(new_top);
					}
					new_top = k;
				}
				else {
					queue.insert( k );
				}
			}
		}

	} while (  (!queue.empty() ||  new_top)  &&  step < MAX_STEP  &&  tmp->g < max_cost  );

#ifdef DEBUG_ROUTES
	// display marked route
	// minimap_t::get_instance()->calc_map();
	DBG_DEBUG("route_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u (max %u)",step,MAX_STEP,queue.get_count(),tmp->g,max_cost);
#endif

	INT_CHECK("route 194");
	// target reached?
	if(!ziel_erreicht  || step >= MAX_STEP  ||  tmp->g >= max_cost  ||  tmp->parent==NULL) {
		if(  step >= MAX_STEP  ) {
			dbg->warning("route_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
		}
	}
	else {
#ifdef DEBUG
		uint32 best = tmp->g;
#endif
		// reached => construct route
		route.store_at( tmp->count, tmp->gr->get_pos() );
		while(tmp != NULL) {
#ifdef DEBUG
			// debug heuristics
			if (tmp->f > best) {
				uint32 dist = calc_distance( tmp->gr->get_pos(), ziel);
				dbg->warning("route_t::intern_calc_route()", "Problem with heuristic:  from %s to %s at %s, best = %d, cost = %d, heur = %d, dist = %d, turns = %d",
					     start.get_str(), ziel.get_fullstr(), tmp->gr->get_pos().get_2d().get_str(), best, tmp->g, tmp->f, dist, tmp->f - tmp->g - dist);
			}
#endif
			route[ tmp->count ] = tmp->gr->get_pos();
			tmp = tmp->parent;
		}
		if (use_jps  &&  tdriver->get_waytype()==water_wt) {
			postprocess_water_route(welt);
		}
		ok = true;
	}

	RELEASE_NODE();

	return ok;
}


/*
 * Postprocess routes created by jump-point search.
 * These routes never turn when going straight.
 * So something like this can happen:
 *
 * >--+
 *    +--+
 *       +-->
 * This method tries to eliminate extra turns to make routes look more like
 *
 * >----+
 *      ++
 *       +-->
 */
void route_t::postprocess_water_route(karte_t *welt)
{
	if (route.get_count() < 5) return;

	// direction of last straight part (and last index of straight part)
	ribi_t::ribi straight_ribi = ribi_type(route[0], route[1]);
	uint32 straight_end = 0;

	// search for route parts:
	// straight - diagonal - straight (same direction as first straight part) - diagonal
	// phase 0       1           2 <- postprocess after next change to diagonal
	uint8 phase = 0;
	uint32 i = 1;
	while( i < route.get_count()-1 )
	{
		ribi_t::ribi ribi = ribi_type(route[i-1], route[i+1]);
		if (ribi_t::is_single(ribi)) {
			if (ribi == straight_ribi) {
				if (phase == 1) {
					// third part starts
					phase = 2;
				}
				else {
					if (phase == 0) {
						// still on first part
						straight_end = i;
					}
				}
			}
			else {
				// straight direction different than before - start anew
				phase = 0;
				straight_end = i;
				straight_ribi = ribi;
			}
		}
		else {
			if (phase < 1) {
				// second phase
				phase = 1;
			}
			else if (phase == 2) {
				// fourth phase
				// postprocess here
				bool ok = ribi_type(route[straight_end], route[i+1]) ==  ribi;
				// try to find straight route, which avoids one diagonal part
				koord3d_vector_t post;
				post.append( route[straight_end] );
				koord3d &end = route[i];
				for(uint32 j = straight_end; j < i  &&  ok; j++) {
					ribi_t::ribi next = 0;
					koord diff = (end - post.back()).get_2d();
					if (abs(diff.x)>=abs(diff.y)) {
						next = diff.x > 0 ? ribi_t::east : ribi_t::west;
						if (abs(diff.x)==abs(diff.y)  &&  next == straight_ribi) {
							next = diff.y > 0 ? ribi_t::south : ribi_t::north;
						}
					}
					else {
						next = diff.y > 0 ? ribi_t::south : ribi_t::north;
					}
					koord3d pos = post.back() + koord(next);
					ok = false;
					if (grund_t *gr = welt->lookup(pos)) {
						if (gr->is_water()) {
							ok = true;
							post.append(pos);
						}
					}
				}
				// now substitute the new route part into the route
				if (ok) {
					for(uint32 j = straight_end; j < i  &&  ok; j++) {
						route[j] = post[j-straight_end];
					}
					// start again with the first straight part
					i = straight_end;
				}
				else {
					// set second straight part to be the first
					straight_end = i-1;
				}
				// start new search
				phase = 0;
			}
		}
		i++;
	}
}



/**
 * searches route, uses intern_calc_route() for distance between stations
 * handles only driving in stations by itself
 */
route_t::route_result_t route_t::calc_route(karte_t *welt, const koord3d ziel, const koord3d start, test_driver_t *tdriver, const sint32 max_khm, sint32 max_len )
{
	route.clear();

	INT_CHECK("route 336");

#ifdef DEBUG_ROUTES
	const uint32 ms = dr_time();
#endif
	bool ok = intern_calc_route(welt, start, ziel, tdriver, max_khm, 0xFFFFFFFFul );
#ifdef DEBUG_ROUTES
	if(tdriver->get_waytype()==water_wt) {
		DBG_DEBUG("route_t::calc_route()", "route from %d,%d to %d,%d with %i steps in %u ms found.", start.x, start.y, ziel.x, ziel.y, route.get_count()-1, dr_time()-ms );
	}
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
			for(  size_t i = route.get_count();  i-- != 0  &&  max_len != 0  &&  halt == haltestelle_t::get_halt(route[i], NULL);  --max_len) {
			}

			// and now go forward, if possible
			if(  max_len>0  ) {

				const uint32 max_n = route.get_count()-1;
				const koord3d zv = route[max_n] - route[max_n - 1];
				const int ribi = ribi_type(zv);

				grund_t *gr = welt->lookup(start);
				const waytype_t wegtyp = tdriver->get_waytype();
				ribi_t::ribi way_ribi  = tdriver->get_ribi(gr);

				while(  max_len>0  &&  ((way_ribi & ribi) != 0)  &&  gr->get_neighbour(gr,wegtyp,ribi)  &&  gr->get_halt()==halt
					&&   tdriver->check_next_tile(gr)) {
					// Do not go on a tile, where a oneway sign forbids going.
					// This saves time and fixed the bug, that a oneway sign on the final tile was ignored.
					ribi_t::ribi go_dir=gr->get_weg(wegtyp)->get_ribi_maske();
					if(  (ribi&go_dir)!=0  ) {
						break;
					}
					route.append(gr->get_pos());
					max_len--;
					way_ribi = tdriver->get_ribi(gr);
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
