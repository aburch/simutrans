/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string.h>

#include <limits.h>
#ifdef UINT_MAX
#define MAXUINT32 UINT_MAX
#endif

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

// this is WAY slower than the sorted_heap! (also not always finding best routes, but was the original algorithms)
//#include "../tpl/prioqueue_tpl.h" // ~100-250% slower

// HOT queue with the pocket size equal to the distance
// still slower than a binary heap
//#include "../tpl/HOT_queue_tpl.h"	// ~10% slower
//#include "../tpl/HOT_queue2_tpl.h" // ~5% slower

// sorted heap, since we only need insert and pop
//#include "../tpl/sorted_heap_tpl.h" // ~10% slower

// binary heap, the fastest
#include "../tpl/binary_heap_tpl.h" // fastest


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


void route_t::truncate_from(uint16 index) {
	for( uint16 i=get_count()-1; i>index; i-- ) {
		route.remove_at(i);
	}
}



/**
 * Appends a straight line from the last koord3d in route to the desired target.
 * Will return false if failed
 * @author prissi
 */
bool route_t::append_straight_route(karte_t *welt, koord3d dest )
{
	if(  !welt->ist_in_kartengrenzen(dest.get_2d())  ) {
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
		if(!welt->ist_in_kartengrenzen(pos)) {
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
uint32 route_t::MAX_STEP=0;
uint32 route_t::max_used_steps=0;
route_t::ANode *route_t::_nodes[MAX_NODES_ARRAY];
bool route_t::_nodes_in_use[MAX_NODES_ARRAY]; // semaphores, since we only have few nodes arrays in memory

void route_t::INIT_NODES(uint32 max_route_steps, uint32 world_width, uint32 world_height)
{
	for (int i = 0; i < MAX_NODES_ARRAY; ++i)
	{
		_nodes[i] = NULL;
		_nodes_in_use[i] = false;
	}

	// may need very much memory => configurable
	MAX_STEP = min(max_route_steps, world_width * world_height); 
	for (int i = 0; i < MAX_NODES_ARRAY; ++i)
	{
		_nodes[i] = new ANode[MAX_STEP + 4 + 2];
	}
}

void route_t::TERM_NODES()
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


/* find the route to an unknow location
 * @author prissi
 */
bool route_t::find_route(karte_t *welt, const koord3d start, fahrer_t *fahr, const uint32 /*max_khm*/, uint8 start_dir, uint32 weight, uint32 max_depth )
{
	bool ok = false;

	// check for existing koordinates
	const grund_t* g = welt->lookup(start);
	if (g == NULL) {
		return false;
	}

	const uint8 enforce_weight_limits =welt->get_settings().get_enforce_weight_limits();

	// some thing for the search
	const waytype_t wegtyp = fahr->get_waytype();

	// memory in static list ...
	if(!MAX_STEP)
	{
		INIT_NODES(welt->get_settings().get_max_route_steps(), welt->get_groesse_x(), welt->get_groesse_y());
	}

	INT_CHECK("route 347");

	// arrays for A*
	//static 
	vector_tpl<ANode*> open;
	vector_tpl<ANode*> close;

	// nothing in lists
	open.clear();

	// we clear it here probably twice: does not hurt ...
	route.clear();

	// first tile is not valid?!?
	if(  !fahr->ist_befahrbar(g)  ) {
		return false;
	}

	ANode *nodes;
	uint8 ni = GET_NODES(&nodes);

	uint32 step = 0;
	ANode* tmp = &nodes[step++];
	if (route_t::max_used_steps < step)
		route_t::max_used_steps = step;
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
				if (is_in_list(open,  to)) continue;
				// already in closed list (i.e. all processed nodes)
				if (is_in_list(close, to)) continue;

				weg_t* w = to->get_weg(fahr->get_waytype());
				
				if (enforce_weight_limits && w != NULL)
				{
					// Bernd Gabriel, Mar 10, 2010: way limit info
					const uint32 way_max_weight = w->get_max_weight();
					max_weight = min(max_weight, way_max_weight);

					if(enforce_weight_limits == 2 && weight > way_max_weight)
					{
						// Avoid routing over ways for which the convoy is overweight.
						continue;
					}
				}

				// not in there or taken out => add new
				ANode* k = &nodes[step++];
				if (route_t::max_used_steps < step)
					route_t::max_used_steps = step;

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
		route.clear();
		route.resize(tmp->count+16);
		while(tmp != NULL) {
			route.store_at( tmp->count, tmp->gr->get_pos() );
//DBG_DEBUG("add","%i,%i",tmp->pos.x,tmp->pos.y);
			tmp = tmp->parent;
		}
		ok = !route.empty();
	}

	RELEASE_NODES(ni);
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


void route_t::concatenate_routes(route_t* tail_route)
{
	route.resize(route.get_count() + tail_route->route.get_count());
	ITERATE_PTR(tail_route, i)
	{
		if(i == 0)
		{ 
			// This is necessary, as otherwise the first tile of the new route
			// will be the same as the last tile of the old route, causing
			// disrupted convoy movement. 
			continue;
		}
		route.append(tail_route->route.get_element(i));
	}
}


bool route_t::intern_calc_route(karte_t *welt, const koord3d ziel, const koord3d start, fahrer_t *fahr, const sint32 max_speed, const uint32 max_cost, const uint32 weight)
{
	bool ok = false;

	// check for existing koordinates
	const grund_t *gr=welt->lookup(start);
	if(gr==NULL  ||  welt->lookup(ziel)==NULL) {
		return false;
	}

	// we clear it here probably twice: does not hurt ...
	route.clear();
	max_weight = MAXUINT32;

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
	if(!MAX_STEP)
	{
		INIT_NODES(welt->get_settings().get_max_route_steps(), welt->get_groesse_x(), welt->get_groesse_y());
	}

	INT_CHECK("route 347");

	// there are several variant for maintaining the open list
	// however, only binary heap and HOT queue with binary heap are worth considering
#if defined(tpl_HOT_queue_tpl_h)
    ///static 
	HOT_queue_tpl <ANode *> queue;
#elif defined(tpl_binary_heap_tpl_h)
    //static 
	binary_heap_tpl <ANode *> queue;
#elif defined(tpl_sorted_heap_tpl_h)
    //static 
	sorted_heap_tpl <ANode *> queue;
#else
    //static 
	prioqueue_tpl <ANode *> queue;
#endif

	ANode *nodes;
	uint8 ni = GET_NODES(&nodes);

	uint32 step = 0;
	ANode* tmp = &nodes[step];
	step ++;
	if (route_t::max_used_steps < step)
		route_t::max_used_steps = step;

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
	const uint8 enforce_weight_limits = welt->get_settings().get_enforce_weight_limits();
	uint32 beat=1;
	do {
		// Hajo: this is too expensive to be called each step
		if((beat++ & 255) == 0) 
		{
			INT_CHECK("route 161");
		}

		ANode *test_tmp = queue.pop();

		if(welt->ist_markiert(test_tmp->gr))
		{
			// we were already here on a faster route, thus ignore this branch
			// (trading speed against memory consumption)
			continue;
		}

		tmp = test_tmp;
		gr = tmp->gr;
		welt->markiere(gr);

//DBG_DEBUG("add to close","(%i,%i,%i) f=%i",gr->get_pos().x,gr->get_pos().y,gr->get_pos().z,tmp->f);

		// we took the target pos out of the closed list
		if(ziel==gr->get_pos())
		{
			ziel_erreicht = true; //"a goal reaches" (Babelfish).
			break;
		}

		// testing all four possible directions
		const ribi_t::ribi ribi =  fahr->get_ribi(gr);
		const ribi_t::ribi *next_ribi = get_next_dirs(gr->get_pos().get_2d(),ziel.get_2d());
		for(int r=0; r<4; r++) {

			// a way in our direction?
			if(  (ribi & next_ribi[r])==0  ) 
			{
				continue;
			}

			to = NULL;
			if(is_airplane) 
			{
				const planquadrat_t *pl=welt->lookup(gr->get_pos().get_2d()+koord(next_ribi[r]));
				if(pl) 
				{
					to = pl->get_kartenboden();
				}
			}

			// a way goes here, and it is not marked (i.e. in the closed list)
			if((to  ||  gr->get_neighbour(to, wegtyp, next_ribi[r]))  &&  fahr->ist_befahrbar(to)  &&  !welt->ist_markiert(to)) 
			{
				// Do not go on a tile, where a oneway sign forbids going.
				// This saves time and fixed the bug, that a oneway sign on the final tile was ignored.
				ribi_t::ribi last_dir=next_ribi[r];
				weg_t *w=to->get_weg(wegtyp);
				ribi_t::ribi go_dir = (w==NULL) ? 0 : w->get_ribi_maske();
				if((last_dir&go_dir)!=0) 
				{
						continue;
				}
				
				if (enforce_weight_limits && w != NULL)
				{
					// Bernd Gabriel, Mar 10, 2010: way limit info
					const uint32 way_max_weight = w->get_max_weight();
					max_weight = min(max_weight, way_max_weight);

					if(enforce_weight_limits == 2 && weight > way_max_weight)
					{
						// Avoid routing over ways for which the convoy is overweight.
						continue;
					}
				}

				// new values for cost g (without way it is either in the air or in water => no costs)
				uint32 new_g = tmp->g + (w ? fahr->get_kosten(to, max_speed, tmp->gr->get_pos().get_2d()) : 1);

				// check for curves (usually, one would need the lastlast and the last;
				// if not there, then we could just take the last
				uint8 current_dir;
				if(tmp->parent!=NULL) 
				{
					current_dir = ribi_typ( tmp->parent->gr->get_pos().get_2d(), to->get_pos().get_2d() );
					if(tmp->dir!=current_dir)
					{
						new_g += 3;
						if(tmp->parent->dir!=tmp->dir  &&  tmp->parent->parent!=NULL) {
							// discourage 90° turns
							new_g += 10;
						}
						else if(ribi_t::ist_exakt_orthogonal(tmp->dir,current_dir))
						{
							// discourage v turns heavily
							new_g += 25;
						}
					}

				}
				else 
				{
					 current_dir = ribi_typ( gr->get_pos().get_2d(), to->get_pos().get_2d() );
				}

				const uint32 new_f = new_g + calc_distance( to->get_pos(), ziel );

				// add new
				ANode* k = &nodes[step];
				step ++;
				if (route_t::max_used_steps < step)
					route_t::max_used_steps = step;

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
		route.clear();
		route.resize(tmp->count+16);
		while(tmp != NULL) {
			route.store_at( tmp->count, tmp->gr->get_pos() );
//DBG_DEBUG("add","%i,%i at pos %i",tmp->gr->get_pos().x,tmp->gr->get_pos().y,tmp->count);
			tmp = tmp->parent;
		}
		ok = true;
	}

	RELEASE_NODES(ni);
	return ok;
}



/* searches route, uses intern_calc_route() for distance between stations
 * handles only driving in stations by itself
 * corrected 12/2005 for station search
 * @author Hansjörg Malthaner, prissi
 */
bool route_t::calc_route(karte_t *welt, const koord3d ziel, const koord3d start, fahrer_t *fahr, const sint32 max_khm, const uint32 weight, sint32 max_len, const uint32 max_cost)
{
	route.clear();

	INT_CHECK("route 336");

#ifdef DEBUG_ROUTES
	// profiling for routes ...
	long ms=dr_time();
#endif
	bool ok = intern_calc_route(welt, start, ziel, fahr, max_khm, max_cost, weight);
#ifdef DEBUG_ROUTES
	if(fahr->get_waytype()==water_wt) {DBG_DEBUG("route_t::calc_route()","route from %d,%d to %d,%d with %i steps in %u ms found.",start.x, start.y, ziel.x, ziel.y, route.get_count()-1, dr_time()-ms );}
#endif

	INT_CHECK("route 343");

	if(!ok)
	{
DBG_MESSAGE("route_t::calc_route()","No route from %d,%d to %d,%d found",start.x, start.y, ziel.x, ziel.y);
		// no route found
		route.resize(1);
		route.append(start); // just to be safe
		return false;
	}
	// advance so all convoy fits into a halt (only set for trains and cars)
	else if(max_len > 1)
	{

		// we need a halt of course ...
		halthandle_t halt = welt->lookup(start)->get_halt();
		// NOTE: halt is actually the *destination* halt.
		if(halt.is_bound()) 
		{
			sint32 platform_size = 0;
			// Count the station size
			for(sint32 i = route.get_count() - 1; i >= 0 && max_len > 0 && halt == haltestelle_t::get_halt(welt, route[i], NULL); i--) 
			{
				platform_size++;
 			}

			// Find the end of the station, and append these tiles to the route.
			const uint32 max_n = route.get_count() - 1;
			const koord zv = route[max_n].get_2d() - route[max_n - 1].get_2d();
			const int ribi = ribi_typ(zv);//fahr->get_ribi(welt->lookup(start));

			grund_t *gr = welt->lookup(start);
			const waytype_t wegtyp = fahr->get_waytype();

			while(gr->get_neighbour(gr, wegtyp, ribi) && gr->get_halt() == halt && fahr->ist_befahrbar(gr) && (fahr->get_ribi(gr) && ribi) != 0)
			{
				// Do not go on a tile where a one way sign forbids going.
				// This saves time and fixed the bug that a one way sign on the final tile was ignored.
				ribi_t::ribi go_dir=gr->get_weg(wegtyp)->get_ribi_maske();
				if((ribi & go_dir) != 0)
				{
					break;
				}
				route.append(gr->get_pos());
				platform_size++;
			}

			sint32 truncate_from_route = 0;

			if(platform_size > max_len)
			{
				// Do not go to the end, but stop part way along the platform.
				const sint32 difference = platform_size - max_len;
				truncate_from_route = (difference + 1) / 2;
				if(truncate_from_route > get_count() - 1)
				{
					truncate_from_route = get_count() - 1;
				}
				
				for(sint32 n = 0; n < truncate_from_route && n < get_count(); n++)
				{
					route.pop_back();
				}
			}
		}
	}
	return true;
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

