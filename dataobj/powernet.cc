/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "powernet.h"

slist_tpl<powernet_t *> powernet_t::powernet_list;


// max capacity = (max<uint32> >> 5) -1, see senke_t::step in dings/leitung2.cc
uint32 powernet_t::max_capacity = 480000*256; // nicer number for human display (corresponds to 30.000 MW)


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
	powernet_list.insert( this );

	this_supply = 0;
	next_supply = 0;
	this_demand = 0;
	next_demand = 0;
}



powernet_t::~powernet_t()
{
	powernet_list.remove( this );
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
