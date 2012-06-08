/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/** @file powernet.h Data structure to manage a net of powerlines - a powernet */

#ifndef powernet_t_h
#define powernet_t_h

#include "../simtypes.h"
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
	 * Must be called when a new map is started or loaded. Clears the table of networks.
	 * @author Hj. Malthaner
	 */
	static void neue_karte();

	/// Steps all powernets
	static void step_all(long delta_t);

private:
	static slist_tpl<powernet_t *> powernet_list;

	/// Max power capacity of each network, only purpose: avoid integer overflows
	static uint32 max_capacity;

	/// Power supply in next step
	uint32 next_supply;
	/// Power supply in current step
	uint32 this_supply;
	/// Power demand in next step
	uint32 next_demand;
	/// Power demand in current step
	uint32 this_demand;

	/// Just transfers power demand and supply to current step
	void step(long delta_t);

public:
	powernet_t();
	~powernet_t();

	uint32 get_max_capacity() { return max_capacity; }

	/// add to power supply for next step, respect max_capacity
	void add_supply(uint32 p)
	{
		next_supply += p;
		if(  next_supply > max_capacity  ) {
			next_supply = max_capacity;
		}
	}

	/// @returns current power supply
	uint32 get_supply() { return this_supply; }

	/// add to power demand for next step, respect max_capacity
	void add_demand(uint32 p) {
		next_demand += p;
		if(  next_demand>max_capacity  ) {
			next_demand = max_capacity;
		}
	}

	/// @returns current power demand
	uint32 get_demand() { return this_demand; }
};

#endif
