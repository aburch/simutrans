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
	static const uint64 max_capacity;

	/// Power supply in next step
	uint64 next_supply;
	/// Power supply in current step
	uint64 this_supply;
	/// Power demand in next step
	uint64 next_demand;
	/// Power demand in current step
	uint64 this_demand;

	/// Just transfers power demand and supply to current step
	void step(long delta_t);

public:
	powernet_t();
	~powernet_t();

	uint64 get_max_capacity() const { return max_capacity; }

	/// add to power supply for next step, respect max_capacity
	void add_supply(const uint32 p);

	/// @returns current power supply
	uint64 get_supply() const { return this_supply; }

	/// add to power demand for next step, respect max_capacity
	void add_demand(const uint32 p);

	/// @returns current power demand
	uint64 get_demand() const { return this_demand; }
};

#endif
