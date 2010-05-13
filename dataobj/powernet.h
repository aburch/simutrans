/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef powernet_t_h
#define powernet_t_h

#include "../simtypes.h"
#include "../tpl/ptrhashtable_tpl.h"
#include "../tpl/slist_tpl.h"


/**
 * Data class for power networks. A two phase queue to store
 * and hand out power.
 * @author Hj. Malthaner
 */
class powernet_t
{
public:
	/**
	 * Must be caled before powernets get loaded. Clears the table of networks
	 * @author Hj. Malthaner
	 */
	static void neue_karte();

	/**
	* Loads a powernet object or hand back already loaded object
	* @author Hj. Malthaner
	*/
	static powernet_t * load_net(powernet_t *key);

	// Steps all powernets
	static void step_all(long delta_t);

private:
	static ptrhashtable_tpl<powernet_t *, powernet_t *> loading_table;
	static slist_tpl<powernet_t *> powernet_list;

	uint32 max_capacity;

	uint32 next_supply;
	uint32 this_supply;
	uint32 next_demand;
	uint32 this_demand;

	void step(long delta_t);

public:
	powernet_t();
	~powernet_t();

	uint32 set_max_capacity(uint32 max) { uint32 m=max_capacity;  if(max>0){max_capacity=max;} return m; }
	uint32 get_max_capacity() { return max_capacity; }

	// adds to power supply for next step
	void add_supply(uint32 p) {	next_supply += p;  if(  next_supply > max_capacity  ) { next_supply = max_capacity;	} }

	uint32 get_supply()	{ return this_supply; }

	// add to power demand for next step
	void add_demand(uint32 p) { next_demand += p;  if(  next_demand>max_capacity  ) { next_demand = max_capacity; } }

	uint32 get_demand()	{ return this_demand; }
};

#endif
