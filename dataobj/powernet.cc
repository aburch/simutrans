/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"

#include "powernet.h"

ptrhashtable_tpl<powernet_t *, powernet_t *> powernet_t::loading_table;
slist_tpl<powernet_t *> powernet_t::powernet_list;



void powernet_t::neue_karte()
{
	loading_table.clear();
	powernet_list.clear();

}



/**
 * Loads a powernet object or hand back already loaded object
 * @author Hj. Malthaner
 */
powernet_t *
powernet_t::load_net(powernet_t *key)
{
	powernet_t * result = loading_table.get(key);
	if(result == 0) {
		result = new powernet_t ();
		loading_table.put(key, result);
	}
	return result;
}



void powernet_t::step_all(long delta_t)
{
	slist_iterator_tpl<powernet_t *> powernet_iter( powernet_list );
	while(  powernet_iter.next()  ) {
		powernet_iter.get_current()->step( delta_t );
	}
}



powernet_t::powernet_t()
{
	powernet_list.insert( this );

	//max_capacity = 524288*256-1; //max allowing dings/leitung2.cc senke_t::sync() power_load calculation in uint32
	max_capacity = 480000*256; // nicer number for human display

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
