/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "powernet.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t netlist_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// max capacity = (max<uint64> >> 5) -1, see senke_t::step in obj/leitung2.cc
//uint64 powernet_t::max_capacity = (1<<44)-1; // max to allow display with uint32 after POWER_TO_MW shift
const uint64 powernet_t::max_capacity = (1953125ull<<23); // nicer number for human display (corresponds to 4 TW)


slist_tpl<powernet_t *> powernet_t::powernet_list;


void powernet_t::neue_karte()
{
	while(!powernet_list.empty()) {
		powernet_t *net = powernet_list.remove_first();
		delete net;
	}
}


void powernet_t::step_all(long delta_t)
{
	FOR(slist_tpl<powernet_t*>, const p, powernet_list) {
		p->step(delta_t);
	}
}


powernet_t::powernet_t()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &netlist_mutex );
#endif
	powernet_list.insert( this );
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &netlist_mutex );
#endif

	this_supply = 0;
	next_supply = 0;
	this_demand = 0;
	next_demand = 0;
}


powernet_t::~powernet_t()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &netlist_mutex );
#endif
	powernet_list.remove( this );
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &netlist_mutex );
#endif
}


void powernet_t::step(long delta_t)
{
	if(  delta_t==0  ) {
		return;
	}

	this_supply = next_supply;
	next_supply = 0;
	this_demand = next_demand;
	next_demand = 0;
}


void powernet_t::add_supply(const uint32 p)
{
	next_supply += p;
	if(  next_supply>max_capacity  ) {
		next_supply = max_capacity;
	}
}


void powernet_t::add_demand(const uint32 p)
{
	next_demand += p;
	if(  next_demand>max_capacity  ) {
		next_demand = max_capacity;
	}
}
