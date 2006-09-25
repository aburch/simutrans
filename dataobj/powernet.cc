/*
 * powernet_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "../simdebug.h"

#include "../tpl/ptrhashtable_tpl.h"
#include "powernet.h"

static ptrhashtable_tpl <powernet_t *, powernet_t *> loading_table;


/**
 * Must be caled before powernets get loaded. Clears the
 * table of networks
 * @author Hj. Malthaner
 */
void powernet_t::prepare_loading()
{
	loading_table.clear();
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



/**
 * Adds some power to the net
 * @author Hj. Malthaner
 */
void powernet_t::add_power(uint32 amount)
{
	power_this += amount;
	if(power_this>max_capacity) {
		power_this = max_capacity;
	}
}



/**
 * Tries toget a certain amount of power from the net.
 * @return granted amount of power
 * @author Hj. Malthaner
 */
uint32
powernet_t::withdraw_power(uint32 want)
{
	const int result = power_last > want ? want : power_last;
	power_last -= result;
	return result;
}



powernet_t::powernet_t()
{
	current_capacity = 0;
	next_t = 0;
	max_capacity = 380000*256;
	for( int i=0;  i<8;  i++ ) {
		capacity[i] = 0;
	}
	power_this = 0;
	power_last = 0;
}



/* calculates the last amout of power draw (a little smoothed)
 * @author prissi
 */
uint32
powernet_t::get_capacity() const
{
	uint32 medium_capacity=0;
	for( uint32 i=0;  i<8;  i++  ) {
		medium_capacity += capacity[i];
	}
	return medium_capacity>>(3+8);
}



/**
 * Methode für Echtzeitfunktionen eines Objekts.
 * @return false wenn Objekt aus der Liste der synchronen
 * Objekte entfernt werden sol
 * @author Hj. Malthaner
 */
bool powernet_t::sync_step(long delta_t)
{
	next_t += delta_t;
	if(next_t>1000) {
		next_t -= 1000;
		power_last = power_this;
		power_this = 0;
		current_capacity &= 7;
		capacity[current_capacity++] = power_last;
	}
	return true;
}
