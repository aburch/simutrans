/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>

#include <limits.h>

#include "../simworld.h"
#include "../simcity.h"
#include "../simintr.h"
#include "../simhalt.h"
#include "../simfab.h"
#include "../boden/wege/weg.h"
#include "../boden/grund.h"
#include "../boden/wasser.h"
#include "../dataobj/marker.h"
#include "../ifc/simtestdriver.h"
#include "loadsave.h"
#include "route.h"
#include "../descriptor/bridge_desc.h"
#include "../boden/wege/strasse.h"
#include "../obj/gebaeude.h"
#include "../obj/roadsign.h"
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

bool route_t::suspend_private_car_routing = false;


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

void route_t::remove_koord_to(uint32 i)
{
	for(uint32 c = 0; c < i; c++)
	{
		route.remove_at(0);
	}
}

/**
 * Appends a straight line from the last koord3d in route to the desired target.
 * Will return false if failed
 * @author prissi
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
thread_local uint32 route_t::MAX_STEP=0;
thread_local uint32 route_t::max_used_steps=0;
thread_local route_t::ANode *route_t::_nodes[MAX_NODES_ARRAY];
thread_local bool route_t::_nodes_in_use[MAX_NODES_ARRAY]; // semaphores, since we only have few nodes arrays in memory

void route_t::INIT_NODES(uint32 max_route_steps, const koord &world_size)
{
	for (int i = 0; i < MAX_NODES_ARRAY; ++i)
	{
		_nodes[i] = NULL;
		_nodes_in_use[i] = false;
	}

	// may need very much memory => configurable
	const uint32 max_world_step_size = world_size == koord::invalid ? max_route_steps :  world_size.x * world_size.y * 2;
	MAX_STEP = min(max_route_steps, max_world_step_size);
	for (int i = 0; i < MAX_NODES_ARRAY; ++i)
	{
		_nodes[i] = new ANode[MAX_STEP + 4 + 2];
	}
}

void route_t::TERM_NODES(void *)
{
	if (MAX_STEP)
	{
		MAX_STEP = 0;
		for (int i = 0; i < MAX_NODES_ARRAY; ++i)
		{
			delete [] _nodes[i];
			_nodes[i] = NULL;
			_nodes_in_use[i] = false;
		}
	}
}

uint8 route_t::GET_NODES(ANode **nodes)
{
	for (int i = 0; i < MAX_NODES_ARRAY; ++i)
		if (!_nodes_in_use[i])
		{
			_nodes_in_use[i] = true;
			*nodes = _nodes[i];
			return i;
		}
	dbg->fatal("GET_NODE","called while list in use");
	return 0;
}

void route_t::RELEASE_NODES(uint8 nodes_index)
{
	if (!_nodes_in_use[nodes_index])
		dbg->fatal("RELEASE_NODE","called while list free");
	_nodes_in_use[nodes_index] = false;
}

/* find the route to an unknown location
 * @author prissi
 */
bool route_t::find_route(karte_t *welt, const koord3d start, test_driver_t *tdriver, const uint32 max_khm, uint8 start_dir, uint32 axle_load, sint32 max_tile_len, uint32 total_weight, uint32 max_depth, bool is_tall, find_route_flags flags)
{
	bool ok = false;

	// check for existing koordinates
	const grund_t* g = welt->lookup(start);
	if(  g == NULL  ) {
		return false;
	}

	const uint8 enforce_weight_limits = welt->get_settings().get_enforce_weight_limits();

	// some thing for the search
	const waytype_t wegtyp = tdriver->get_waytype();

	// memory in static list ...
	if(!MAX_STEP)
	{
		INIT_NODES(welt->get_settings().get_max_route_steps(), welt->get_size());
	}

	// nothing in lists
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y, karte_t::marker_index);

	binary_heap_tpl <ANode *> queue;

	// nothing in lists
	queue.clear();

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(!tdriver->check_next_tile(g))
	{
		return false;
	}

	ANode *nodes;
	uint8 ni = GET_NODES(&nodes);

#ifdef USE_VALGRIND_MEMCHECK
	VALGRIND_MAKE_MEM_UNDEFINED(nodes, sizeof(ANode)*MAX_STEP);
#endif

	uint32 step = 0;
	ANode* tmp = &nodes[step++];
	if (route_t::max_used_steps < step)
	{
		route_t::max_used_steps = step;
	}
	tmp->parent = NULL;
	tmp->gr = g;
	tmp->count = 0;
	tmp->f = 0;
	tmp->g = 0;
	tmp->dir = 0;

	// start in open
	queue.insert(tmp);

	const grund_t* gr = NULL;
	sint32 bridge_tile_count = 0;

	const fabrik_t* destination_industry;
	const gebaeude_t* destination_attraction;
	const stadt_t* destination_city;
	stadt_t* origin_city = NULL;
	bool reached_target = false;

	if (flags == private_car_checker)
	{
		origin_city = welt->access(start.get_2d())->get_city();
		if (origin_city)
		{
			origin_city->set_private_car_route_finding_in_progress(true);
		}
	}

	uint32 private_car_route_step_counter = 0;

	do
	{
		destination_industry = NULL;
		destination_attraction = NULL;
		destination_city = NULL;

		ANode *test_tmp = queue.pop();

		// already in open or closed (i.e. all processed nodes) list?
		if(marker.is_marked(test_tmp->gr))
		{
			// we were already here on a faster route, thus ignore this branch
			// (trading speed against memory consumption)
			continue;
		}

		tmp = test_tmp;
		gr = tmp->gr;
		marker.mark(gr);

		// already there
		if(tdriver->is_target(gr, tmp->parent == NULL ? NULL : tmp->parent->gr))
		{
			if(flags != private_car_checker)
			{
				// we added a target to the closed list: check for length
				break;
			}
			else
			{
				// Cost should be journey time per *straight line* tile, as the private car route
				// system needs to be able to approximate the total travelling time from the straight
				// line distance.
				reached_target = true;
				const koord3d k = gr->get_pos();
				destination_city = welt->access(k.get_2d())->get_city();
				if(destination_city && destination_city->get_townhall_road() == k.get_2d())
				{
					// This is a city destination.
					if(origin_city && start.get_2d() == k.get_2d())
					{
						// Very rare, but happens occasionally - two cities share a townhall road tile.
						// Must treat specially in order to avoid a division by zero error
#ifdef MULTI_THREAD
						int error = pthread_mutex_lock(&karte_t::private_car_route_mutex);
						assert(error == 0);
						(void)error;
#endif
						origin_city->add_road_connexion(10, destination_city);
#ifdef MULTI_THREAD
						error = pthread_mutex_unlock(&karte_t::private_car_route_mutex);
						assert(error == 0);
						(void)error;
#endif
					}
					else if(origin_city)
					{
						const uint16 straight_line_distance = shortest_distance(origin_city->get_townhall_road(), k.get_2d());
#ifdef MULTI_THREAD
						int error = pthread_mutex_lock(&karte_t::private_car_route_mutex);
						assert(error == 0);
						(void)error;
#endif
						origin_city->add_road_connexion(tmp->g / straight_line_distance, welt->access(k.get_2d())->get_city());
#ifdef MULTI_THREAD
						error = pthread_mutex_unlock(&karte_t::private_car_route_mutex);
						assert(error == 0);
						(void)error;
#endif
					}
				}
				else
				{
					// Do not register multiple routes to the destination city.
					destination_city = NULL;
				}

				weg_t* way = gr->get_weg(road_wt);

				if(way && way->connected_buildings.get_count() > 0)
				{
					FOR(minivec_tpl<gebaeude_t*>, const gb, way->connected_buildings)
					{
						if(!gb)
						{
							// Dud building
							// Is not thread-safe to remove this here.
							continue;
						}

						uint16 straight_line_distance;
						if (origin_city)
						{
							straight_line_distance = shortest_distance(origin_city->get_townhall_road(), k.get_2d());
						}
						else
						{
							straight_line_distance = shortest_distance(start.get_2d(), k.get_2d());
						}
						uint16 journey_time_per_tile;
						if(straight_line_distance == 0)
						{
							journey_time_per_tile = 10;
						}
						else
						{
							journey_time_per_tile = tmp->g / straight_line_distance;
						}
						destination_industry = gb->get_fabrik();
						if(destination_industry && origin_city)
						{
							// This is an industry
#ifdef MULTI_THREAD
							int error = pthread_mutex_lock(&karte_t::private_car_route_mutex);
							assert(error == 0);
							(void)error;
#endif
							origin_city->add_road_connexion(journey_time_per_tile, destination_industry);
#ifdef MULTI_THREAD
							error = pthread_mutex_unlock(&karte_t::private_car_route_mutex);
							assert(error == 0);
							(void)error;
#endif
#if 0
							if (destination_city)
							{
								// Only mark routes to country industries
								destination_industry = NULL;
							}
#endif
						}
						else if (origin_city && gb && gb->is_attraction())
						{
#ifdef MULTI_THREAD
							int error = pthread_mutex_lock(&karte_t::private_car_route_mutex);
							assert(error == 0);
							(void)error;
#endif
							origin_city->add_road_connexion(journey_time_per_tile, gb);
#ifdef MULTI_THREAD
							error = pthread_mutex_unlock(&karte_t::private_car_route_mutex);
							assert(error == 0);
							(void)error;
#endif
#if 0
							if (!destination_city)
							{
								// Only mark routes to country attractions
								destination_attraction = gb;
							}
#else
							destination_attraction = gb;
#endif
						}
					}
				}
			}
		}

		// Relax the route here if this is a private car route checker, as we may find many destinations.
		if (reached_target && flags == private_car_checker && (destination_attraction || destination_industry || destination_city))
		{
			route.clear();
			ANode* original_tmp = tmp;
			//route.resize(tmp->count + 16);
			const koord destination_pos = destination_industry ? destination_industry->get_pos().get_2d() : destination_attraction ? destination_attraction->get_first_tile()->get_pos().get_2d() : destination_city ? destination_city->get_townhall_road() : koord::invalid;
			koord3d previous = koord3d::invalid;
			weg_t* w;
			while (tmp != NULL)
			{
				private_car_route_step_counter++;
				w = tmp->gr->get_weg(road_wt);

				if (w)
				{
					// The route is added here in a different array index to the set of routes
					// that are currently being read.

					// Also, the route is iterated here *backwards*.
					w->add_private_car_route(destination_pos, previous);
				}

				// Old route storage - we probably no longer need this.
				//route.store_at(tmp->count, tmp->gr->get_pos());

				previous = tmp->gr->get_pos();
				tmp = tmp->parent;
			}

#ifdef MULTI_THREAD
			const uint32 max_steps = welt->get_settings().get_max_route_tiles_to_process_in_a_step();
			if (max_steps && !suspend_private_car_routing && private_car_route_step_counter >= max_steps)
			{
				// Halt this mid step if there are too many routes being calculated so as not to make the game unresponsive.
				// On a Ryzen 3900x, calculating all routes from one city on a 600 city map can take ~4 seconds.

				// It is intentional to have two barriers here.
				simthread_barrier_wait(&karte_t::private_car_barrier);
				if (!suspend_private_car_routing)
				{
					simthread_barrier_wait(&karte_t::private_car_barrier);
				}
				private_car_route_step_counter = 0;
			}
#endif
			tmp = original_tmp;

		}

		// testing all four possible directions
		ribi_t::ribi ribi;
		if(tdriver->get_waytype() == track_wt || tdriver->get_waytype() == tram_wt || tdriver->get_waytype() == monorail_wt || tdriver->get_waytype() == narrowgauge_wt || tdriver->get_waytype() == maglev_wt)
		{
			const weg_t* way = gr->get_weg(tdriver->get_waytype());
			if(way && way->has_signal())
			{
				ribi = way->get_ribi_unmasked();
			}
			else
			{
				ribi = tdriver->get_ribi(gr);
			}
		}
		else
		{
			ribi = tdriver->get_ribi(gr);
		}

		if(gr->ist_bruecke())
		{
			bridge_tile_count++;
		}
		else
		{
			bridge_tile_count = 0;
		}
		for(int r = 0; r < 4; r++)
		{
			// a way goes here, and it is not marked (i.e. in the closed list)
			grund_t* to;
			if((ribi & ribi_t::nsew[r] & start_dir) != 0  // allowed dir (we can restrict the first step by start_dir)
				&& koord_distance(start, gr->get_pos() + koord::nsew[r]) < max_depth	// not too far away
				&& gr->get_neighbour(to, wegtyp, ribi_t::nsew[r])  // is connected
				&& !marker.is_marked(to) // not already tested
				&& tdriver->check_next_tile(to)	// can be driven on
			) {

				weg_t* w = to->get_weg(tdriver->get_waytype());

				if (is_tall && to->is_height_restricted())
				{
					// Tall vehicles cannot pass under low bridges
					continue;
				}

				if(enforce_weight_limits > 1 && w != NULL)
				{
					// Bernd Gabriel, Mar 10, 2010: way limit info
					const uint32 way_max_axle_load = w->get_max_axle_load();
					const uint32 bridge_weight_limit = w->get_bridge_weight_limit();

					// This ensures that only that part of the convoy that is actually on the bridge counts.
					uint32 adjusted_convoy_weight = max_tile_len == 0 ? total_weight : (total_weight * max(bridge_tile_count - 2, 1)) / max_tile_len;

					if(axle_load > way_max_axle_load || adjusted_convoy_weight > bridge_weight_limit)
					{
						if(enforce_weight_limits == 2)
						{
							// Avoid routing over ways for which the convoy is overweight.
							continue;
						}
						else if((enforce_weight_limits == 3 && (way_max_axle_load == 0 || (axle_load * 100) / way_max_axle_load > 110)) || (bridge_weight_limit == 0 || (adjusted_convoy_weight * 100) / bridge_weight_limit > 110))
						{
							// Avoid routing over ways for which the convoy is more than 10% overweight or which have a zero weight limit.
							continue;
						}
					}
				}

				if(flags == choose_signal && w && w->has_sign())
				{
					// Do not traverse routes with end of choose signs
					 roadsign_t* rs = welt->lookup(w->get_pos())->find<roadsign_t>();
					 if(rs->get_desc()->is_end_choose_signal())
					 {
						 continue;
					 }
				}

				// not in there or taken out => add new
				ANode* k = &nodes[step++];
				if (route_t::max_used_steps < step)
				{
					route_t::max_used_steps = step;
				}

				k->parent = tmp;
				k->gr = to;
				k->count = tmp->count+1;
				k->f = 0;
				k->g = tmp->g + tdriver->get_cost(to, max_khm, gr->get_pos().get_2d());
				k->ribi_from = ribi_t::nsew[r];

				uint8 current_dir = ribi_t::nsew[r];
				if(tmp->parent!=NULL) {
					current_dir |= tmp->ribi_from;
					if(tmp->dir!=current_dir) {
						k->g += 3;
						if(ribi_t::is_perpendicular(tmp->dir,current_dir))
						{
							if(flags == choose_signal)
							{
								// In the case of a choose signal, this will in some situations allow trains to
								// route in very suboptimal ways if a route is part blocked: see here for an explanation:
								// http://forum.simutrans.com/index.php?topic=14839.msg146645#msg146645
								continue;
							}
							else
							{
								// discourage v turns heavily
								k->g += 25;
							}
						}
						else if(tmp->parent->dir!=tmp->dir  &&  tmp->parent->parent!=NULL)
						{
							// discourage 90� turns
							k->g += 10;
						}
					}
				}
				k->dir = current_dir;

				// insert here
				queue.insert(k);
			}
		}

		// ok, now no more restraints
		start_dir = ribi_t::all;

	} while(  !queue.empty()  &&  step < MAX_STEP  &&  queue.get_count() < max_depth  );

	// target reached?
	if(!tdriver->is_target(gr, tmp->parent == NULL ? NULL : tmp->parent->gr) || step >= MAX_STEP)
	{
		if(  step >= MAX_STEP  )
		{
			dbg->warning("route_t::find_route()","Too many steps (%i>=max %i) in route (too long/complex)", step, MAX_STEP);

		}
	}
	else
	{
		if(flags != private_car_checker)
		{
			// reached => construct route
			route.clear();
			route.resize(tmp->count+16);
			while(tmp != NULL)
			{
				route.store_at(tmp->count, tmp->gr->get_pos());
				tmp = tmp->parent;
			}
			ok = !route.empty();
		}
		else
		{
			ok = step < MAX_STEP;
		}
//		ok = !route.empty();
	}
	if (origin_city)
	{
		origin_city->set_private_car_route_finding_in_progress(false);
	}
	RELEASE_NODES(ni);
	return ok;
}



ribi_t::ribi *get_next_dirs(const koord3d& gr_pos, const koord3d& ziel)
{
	static thread_local ribi_t::ribi next_ribi[4];
	if( abs(gr_pos.x-ziel.x)>abs(gr_pos.y-ziel.y) ) {
		next_ribi[0] = (ziel.x>gr_pos.x) ? ribi_t::east : ribi_t::west;
		next_ribi[1] = (ziel.y>gr_pos.y) ? ribi_t::south : ribi_t::north;
	}
	else {
		next_ribi[0] = (ziel.y>gr_pos.y) ? ribi_t::south : ribi_t::north;
		next_ribi[1] = (ziel.x>gr_pos.x) ? ribi_t::east : ribi_t::west;
	}
	next_ribi[2] = ribi_t::reverse_single(next_ribi[1]);
	next_ribi[3] = ribi_t::reverse_single(next_ribi[0]);
	return next_ribi;
}


route_t::route_result_t route_t::intern_calc_route(karte_t *welt, const koord3d start, const koord3d ziel, test_driver_t* const tdriver, const sint32 max_speed, const sint64 max_cost, const uint32 axle_load, const uint32 convoy_weight, bool is_tall, const sint32 tile_length, koord3d avoid_tile, uint8 start_dir, find_route_flags flags)
{
	route_result_t ok = no_route;

	// check for existing koordinates
	const grund_t *gr=welt->lookup(start);
	if(  gr == NULL  ||  welt->lookup(ziel) == NULL) {
		return no_route;
	}

	// we clear it here probably twice: does not hurt ...
	route.clear();
	max_axle_load = MAXUINT32;
	max_convoy_weight = MAXUINT32;

	// first tile is not valid?!?
	if(  !tdriver->check_next_tile(gr)  ) {
		return no_route;
	}

	// some thing for the search
	const waytype_t wegtyp = tdriver->get_waytype();
	const bool is_airplane = tdriver->get_waytype()==air_wt;
	const uint32 cost_upslope = flags == simple_cost ? 0 : tdriver->get_cost_upslope();

	/* On water we will try jump point search (jps):
	 * - If going straight do not turn, only if near an obstacle.
	 * - If going diagonally only proceed in the two directions defining the diagonal.
	 * Ideally, no water tile is visited twice.
	 * Needs postprocessing to eliminate unnecessary turns.
	 *
	 * Reference:
	 *  Harabor D. and Grastien A. 2011. Online Graph Pruning for Pathfinding on Grid Maps.
	 *  In Proceedings of the 25th National Conference on Artificial Intelligence (AAAI), San Francisco, USA.
	 *  http://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-aaai11.pdf
	 */

	const bool use_jps     = tdriver->get_waytype()==water_wt;
	//const bool use_jps     = false;

	bool ziel_erreicht=false;

	// memory in static list ...
	if(!MAX_STEP)
	{
		INIT_NODES(welt->get_settings().get_max_route_steps(), welt->get_size());
	}

	binary_heap_tpl <ANode *> queue;

	ANode *nodes;
	uint8 ni = GET_NODES(&nodes);

#ifdef USE_VALGRIND_MEMCHECK
	VALGRIND_MAKE_MEM_UNDEFINED(nodes, sizeof(ANode)*MAX_STEP);
#endif

	uint32 step = 0;
	ANode* tmp = &nodes[step];
	step ++;
	if (route_t::max_used_steps < step)
		route_t::max_used_steps = step;

	tmp->parent = NULL;
	tmp->gr = gr;
	tmp->f = calc_distance(start, ziel) * 10;
	tmp->g = 0;
	tmp->dir = 0;
	tmp->count = 0;
	tmp->ribi_from = ribi_t::none;
	tmp->jps_ribi  = ribi_t::all;

	// nothing in lists
	marker_t& marker = marker_t::instance(welt->get_size().x, welt->get_size().y, karte_t::marker_index);

	const grund_t* avoid_ground = welt->lookup(avoid_tile);
	marker.mark(avoid_ground);

	// clear the queue (should be empty anyhow)
	queue.clear();
	queue.insert(tmp);
	ANode* new_top = NULL;

	const uint8 enforce_weight_limits = welt->get_settings().get_enforce_weight_limits();
#ifndef MULTI_THREAD
	uint32 beat=1;
#endif
	sint32 bridge_tile_count = 0;
	uint32 best_distance = 0xFFFF;

	do {
#ifndef MULTI_THREAD
		// If this is multi-threaded, we cannot have random
		// threads calling INT_CHECK.
		// Hajo: this is too expensive to be called each step
		if((beat++ & 1023) == 0)
		{
			INT_CHECK("route 161");
		}
#endif

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
		const weg_t* way = gr->get_weg(tdriver->get_waytype());

		const ribi_t::ribi way_ribi = way && way->has_signal() ? gr->get_weg_ribi_unmasked(tdriver->get_waytype()) : tdriver->get_ribi(gr);
		// testing all four possible directions
		// mask direction we came from
		const ribi_t::ribi ribi = way_ribi & ( ~ribi_t::reverse_single(tmp->ribi_from) ) & tmp->jps_ribi;

		const ribi_t::ribi *next_ribi = get_next_dirs(gr->get_pos(), ziel);
		for(int r=0; r<4; r++)
		{
			// a way in our direction?
			if(  (ribi & next_ribi[r])==0  )
			{
				continue;
			}

			if(start_dir != ribi_t::all && (ribi & ribi_t::nsew[r] & start_dir) == 0)  // allowed dir (we can restrict the first step by start_dir))
			{
				continue;
			}

			grund_t *to = NULL;
			if(is_airplane)
			{
				const planquadrat_t *pl=welt->access(gr->get_pos().get_2d()+koord(next_ribi[r]));
				if(pl)
				{
					to = pl->get_kartenboden();
				}
			}

			// a way goes here, and it is not marked (i.e. in the closed list)
			if ((to || gr->get_neighbour(to, wegtyp, next_ribi[r])) && tdriver->check_next_tile(to) && !marker.is_marked(to))
			{
				// Do not go on a tile where a one way sign forbids going.
				// This saves time and fixed the bug in which a oneway sign on the final tile was ignored.
				ribi_t::ribi last_dir = next_ribi[r];
				weg_t *w = to->get_weg(wegtyp);
				ribi_t::ribi go_dir = (w == NULL) ? 0 : w->get_ribi_maske();
				if ((last_dir&go_dir) != 0)
				{
					if (tdriver->get_waytype() == track_wt || tdriver->get_waytype() == narrowgauge_wt || tdriver->get_waytype() == maglev_wt || tdriver->get_waytype() == tram_wt || tdriver->get_waytype() == monorail_wt)
					{
						// Unidirectional signals allow routing in both directions but only act in one direction. Check whether this is one of those.
						if (!w->has_signal())
						{
							continue;
						}
					}
					else
					{
						continue;
					}
				}

				// Low bridges
				if (is_tall && to->is_height_restricted())
				{
					continue;
				}

				// Weight limits
				sint32 is_overweight = not_overweight;
				if (enforce_weight_limits > 0 && w != NULL)
				{
					// Bernd Gabriel, Mar 10, 2010: way limit info
					if (to->ist_bruecke() || w->get_desc()->get_styp() == type_elevated || w->get_waytype() == air_wt || w->get_waytype() == water_wt)
					{
						// Bridges care about convoy weight, whereas other types of way
						// care about axle weight.
						bridge_tile_count++;

						// This is actually maximum convoy weight: the name is odd because of the virtual method.
						uint32 way_max_convoy_weight;

						// Trams need to check the weight of the underlying bridge.

						if (w->get_desc()->get_styp() == type_tram)
						{
							const weg_t* underlying_bridge = welt->lookup(w->get_pos())->get_weg(road_wt);
							if (!underlying_bridge)
							{
								goto check_axle_load;
							}
							way_max_convoy_weight = underlying_bridge->get_bridge_weight_limit();

						}
						else
						{
							way_max_convoy_weight = w->get_bridge_weight_limit();
						}

						// This ensures that only that part of the convoy that is actually on the bridge counts.
						const sint32 proper_tile_length = tile_length > 8888 ? tile_length - 8888 : tile_length;
						uint32 adjusted_convoy_weight = tile_length == 0 ? convoy_weight : (convoy_weight * max(bridge_tile_count - 2, 1)) / proper_tile_length;
						const uint32 min_weight = min(adjusted_convoy_weight, convoy_weight);
						if (min_weight > way_max_convoy_weight)
						{
							switch (enforce_weight_limits)
							{
							case 1:
							default:

								is_overweight = slowly_only;
								break;

							case 2:

								is_overweight = cannot_route;
								break;

							case 3:

								is_overweight = way_max_convoy_weight == 0 || (min_weight * 100) / way_max_convoy_weight > 110 ? cannot_route : slowly_only;
								break;
							}
						}
						if (to->ist_bruecke())
						{
							// For a real bridge, also check the axle load of the underlying way.
							goto check_axle_load;
						}
					}
					else
					{
					check_axle_load:
						bridge_tile_count = 0;
						const uint32 way_max_axle_load = w->get_max_axle_load();
						max_axle_load = min(max_axle_load, way_max_axle_load);
						if (axle_load > way_max_axle_load)
						{
							switch (enforce_weight_limits)
							{
							case 1:
							default:

								is_overweight = slowly_only;
								break;

							case 2:

								is_overweight = cannot_route;
								break;

							case 3:

								is_overweight = way_max_axle_load == 0 || (axle_load * 100) / way_max_axle_load > 110 ? cannot_route : slowly_only;
								break;
							}
						}
					}

					if (is_overweight == cannot_route)
					{
						// Avoid routing over ways for which the convoy is overweight.
						continue;
					}

				}

				// new values for cost g (without way it is either in the air or in water => no costs)
				const int way_cost = flags == simple_cost ? 1 : tdriver->get_cost(to, max_speed, tmp->gr->get_pos().get_2d()) + (is_overweight == slowly_only ? 400 : 0);
				uint32 new_g = tmp->g + (w ? way_cost : flags == simple_cost ? 1 : 10);

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if (tmp->parent != NULL && flags != simple_cost) {
					current_dir = next_ribi[r] | tmp->ribi_from;
					if (tmp->dir != current_dir) {
						new_g += 30;
						if (tmp->parent->dir != tmp->dir  &&  tmp->parent->parent != NULL) {
							// discourage 90� turns
							new_g += 10;
						}
						else if (ribi_t::is_perpendicular(tmp->dir, current_dir))
						{
							// discourage v turns heavily
							new_g += 25;
						}
					}

				}
				else {
					current_dir = next_ribi[r];
				}

				uint32 dist = calc_distance(to->get_pos(), ziel);

				best_distance = (dist < best_distance) ? dist : best_distance;

				// count how many 45 degree turns are necessary to get to target
				sint8 turns = 0;
				if (dist > 1 && flags != simple_cost) {
					ribi_t::ribi to_target = ribi_type(to->get_pos(), ziel);

					if (to_target && (to_target != current_dir)) {
						if (ribi_t::is_single(current_dir) != ribi_t::is_single(to_target)) {
							to_target = ribi_t::rotate45(to_target);
							turns++;
						}
						while (to_target != current_dir /*&& turns < 126*/) {
							to_target = ribi_t::rotate90(to_target);
							turns += 2;
						}
						if (turns > 4) turns = 8 - turns;
					}
				}
				// add 3*turns to the heuristic bound

				// take height difference into account when calculating distance
				uint32 costup = 0;
				if (cost_upslope) {
					costup = cost_upslope * max(ziel.z - to->get_vmove(next_ribi[r]), 0);
				}

				const uint32 new_f = (new_g + dist + turns * 3 + costup) * 10;

				// add new
				ANode* k = &nodes[step];
				step++;
				if (route_t::max_used_steps < step)
					route_t::max_used_steps = step;

				k->parent = tmp;
				k->gr = to;
				k->g = new_g;
				k->f = new_f;
				k->dir = current_dir;
				k->ribi_from = next_ribi[r];
				k->count = tmp->count + 1;
				k->jps_ribi = ribi_t::all;

				if (use_jps  &&  to->is_water()) {
					// only check previous direction plus directions not available on this tile
					// if going straight only check straight direction
					// if going diagonally check both directions that generate this diagonal
					// also enter all available canals and turn to get around canals
					if (tmp->parent != NULL) {
						k->jps_ribi = ~way_ribi | current_dir | ((wasser_t*)to)->get_canal_ribi();

						if (gr->is_water()) {
							// turn on next tile to enter possible neighbours of canal tiles
							k->jps_ribi |= ((const wasser_t*)gr)->get_canal_ribi();
						}
					}
				}


				if (new_f <= topnode_f) {
					// do not put in queue if the new node is the best one
					topnode_f = new_f;
					if (new_top) {
						queue.insert(new_top);
					}
					new_top = k;
				}
				else {
					queue.insert(k);
				}
			}
		}

	} while (  (!queue.empty() ||  new_top)  &&  step < MAX_STEP  &&  tmp->g < max_cost  );

#ifdef DEBUG_ROUTES
	// display marked route
	//reliefkarte_t::get_karte()->calc_map();
	DBG_DEBUG("route_t::intern_calc_route()","steps=%i  (max %i) in route, open %i, cost %u (max %u)",step,MAX_STEP,queue.get_count(),tmp->g,max_cost);
#endif

	//INT_CHECK("route 194");
	// target reached?
	if(!ziel_erreicht  || step >= MAX_STEP  ||  tmp->g >= max_cost  ||  tmp->parent==NULL) {
		if(  step >= MAX_STEP  ) {
			dbg->warning("route_t::intern_calc_route()","Too many steps (%i>=max %i) in route (too long/complex)",step,MAX_STEP);
			ok = route_too_complex;
		}
	}
	else {
#ifdef DEBUG
		const uint32 best = tmp->g;
#endif
		// reached => construct route
		route.store_at( tmp->count, tmp->gr->get_pos() );
		while(tmp != NULL) {
#ifdef DEBUG
			// debug heuristics
			if (tmp->f > best) {
				uint32 dist = calc_distance( tmp->gr->get_pos(), ziel);
				dbg->warning("route_t::intern_calc_route()", "Problem with heuristic:  from %s to %s at %s, best = %d, cost = %d, heur = %d, dist = %d, turns = %d\n",
						    start.get_str(), ziel.get_fullstr(), tmp->gr->get_pos().get_2d().get_str(), best, tmp->g, tmp->f, dist, tmp->f - tmp->g - dist);
			}
#endif
			route[ tmp->count ] = tmp->gr->get_pos();
			tmp = tmp->parent;
		}
		if (use_jps  &&  tdriver->get_waytype()==water_wt) {
			postprocess_water_route(welt);
		}
		ok = valid_route;
	}

	RELEASE_NODES(ni);
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
				koord3d_vector_t mail;
				mail.append( route[straight_end] );
				koord3d &end = route[i];
				for(uint32 j = straight_end; j < i  &&  ok; j++) {
					ribi_t::ribi next = 0;
					koord diff = (end - mail.back()).get_2d();
					if (abs(diff.x)>=abs(diff.y)) {
						next = diff.x > 0 ? ribi_t::east : ribi_t::west;
						if (abs(diff.x)==abs(diff.y)  &&  next == straight_ribi) {
							next = diff.y > 0 ? ribi_t::south : ribi_t::north;
						}
					}
					else {
						next = diff.y > 0 ? ribi_t::south : ribi_t::north;
					}
					koord3d pos = mail.back() + koord(next);
					ok = false;
					if (grund_t *gr = welt->lookup(pos)) {
						if (gr->is_water()) {
							ok = true;
							mail.append(pos);
						}
					}
				}
				// now substitute the new route part into the route
				if (ok) {
					for(uint32 j = straight_end; j < i  &&  ok; j++) {
						route[j] = mail[j-straight_end];
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


/* searches route, uses intern_calc_route() for distance between stations
 * handles only driving in stations by itself
 * corrected 12/2005 for station search
 * @author Hansj�rg Malthaner, prissi
 */
 route_t::route_result_t route_t::calc_route(karte_t *welt, const koord3d start, const koord3d ziel, test_driver_t* const tdriver, const sint32 max_khm, const uint32 axle_load, bool is_tall, sint32 max_len, const sint64 max_cost, const uint32 convoy_weight, koord3d avoid_tile, uint8 direction, find_route_flags flags)
{
	route.clear();
	const uint32 distance = shortest_distance(start.get_2d(), ziel.get_2d()) * 600;
	if(tdriver->get_waytype() == water_wt && distance > (uint32)welt->get_settings().get_max_route_steps())
	{
		// Do not actually try to calculate the route if it is doomed to failure.
		// This ensures that the game does not become overloaded if a line
		// is altered so as to have many ships suddenly unable to find a route.
		route.append(start); // just to be safe
		return route_too_complex;
	}
//	INT_CHECK("route 336");

#ifdef DEBUG_ROUTES
	// profiling for routes ...
	long ms=dr_time();
#endif
	route_result_t ok = intern_calc_route(welt, start, ziel, tdriver, max_khm, max_cost, axle_load, convoy_weight, is_tall, max_len, avoid_tile, direction, flags);
#ifdef DEBUG_ROUTES
	if(tdriver->get_waytype()==water_wt) {DBG_DEBUG("route_t::calc_route()","route from %d,%d to %d,%d with %i steps in %u ms found.",start.x, start.y, ziel.x, ziel.y, route.get_count()-1, dr_time()-ms );}
#endif

//	INT_CHECK("route 343");

	if(ok != valid_route)
	{
		DBG_MESSAGE("route_t::calc_route()","No route from %d,%d to %d,%d found",start.x, start.y, ziel.x, ziel.y);
		// no route found
		route.resize(1);
		route.append(start); // just to be safe
		return ok;
	}

	// advance so all convoy fits into a halt (only set for trains and cars)
	bool move_to_end_of_station = max_len >= 8888;
	if(move_to_end_of_station)
	{
		max_len -= 8888;
	}
	if(max_len > 1 )
	{
		// we need a halt of course ...
		grund_t *gr = welt->lookup(ziel);
		halthandle_t halt = gr->get_halt();
		if(halt.is_bound())
		{
			sint32 platform_size = 0;
			// Count the station size
			for(sint32 i = route.get_count() - 1; i >= 0 && max_len > 0 && halt == haltestelle_t::get_halt(route[i], NULL); i--)
			{
				platform_size++;
 			}

			// Find the end of the station, and append these tiles to the route.
			const uint32 max_n = route.get_count() - 1;
			const koord zv = route[max_n].get_2d() - route[max_n - 1].get_2d();
			const int ribi = ribi_type(zv);

			const waytype_t wegtyp = tdriver->get_waytype();

			const bool is_rail_type = tdriver->get_waytype() == track_wt || tdriver->get_waytype() == narrowgauge_wt || tdriver->get_waytype() == maglev_wt || tdriver->get_waytype() == tram_wt || tdriver->get_waytype() == monorail_wt;
			bool first_run = true;

			bool ribi_check = (tdriver->get_ribi(gr) & ribi) != 0;
			bool has_signal = gr->get_weg(tdriver->get_waytype())->has_signal();

			while(gr->get_neighbour(gr, wegtyp, ribi) && gr->get_halt() == halt && tdriver->check_next_tile(gr) && ((ribi_check || (first_run && is_rail_type))))
			{
				first_run = false;
				has_signal = gr->get_weg(tdriver->get_waytype())->has_signal();
				// Do not go on a tile where a one way sign forbids going.
				// This saves time and fixed the bug that a one way sign on the final tile was ignored.
				weg_t* wg = gr->get_weg(wegtyp);
				ribi_t::ribi go_dir = wg ? wg->get_ribi_maske(): ribi_t::all;
				if((ribi & go_dir) != 0)
				{
					if(is_rail_type)
					{
						// Unidirectional signals allow routing in both directions but only act in one direction. Check whether this is one of those.
						if(!gr->get_weg(tdriver->get_waytype()) || !has_signal)
						{
							break;
						}
					}
					else
					{
						break;
					}
				}

				route.append(gr->get_pos());
				platform_size++;

				if (has_signal)
				{
					ribi_t::ribi ribi_unmasked = gr->get_weg_ribi_unmasked(wegtyp);
					ribi_check = (ribi_unmasked & ribi) != 0;
				}
				else
				{
					ribi_check = (tdriver->get_ribi(gr) & ribi) != 0;
				}

			}

			if(!move_to_end_of_station && platform_size > max_len)
			{
				// Do not go to the end, but stop part way along the platform.
				sint32 truncate_from_route = min(((platform_size - max_len) + 1) >> 1, get_count() - 1);
				while(truncate_from_route-- > 0)
				{
					route.pop_back();
				}
			}

			// station too short => warning!
			if(max_len > platform_size)
			{
				return valid_route_halt_too_short;
			}
		}
	}

	return ok;
}




void route_t::rdwr(loadsave_t *file)
{
	xml_tag_t r( file, "route_t" );
	sint32 max_n = route.get_count()-1;

	if(file->get_extended_version() >= 11 && file->get_version() >= 112003)
	{
		file->rdwr_long(max_axle_load);
		file->rdwr_long(max_convoy_weight);
	}
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

