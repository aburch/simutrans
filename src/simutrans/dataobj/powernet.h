/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_POWERNET_H
#define DATAOBJ_POWERNET_H


#include "../simtypes.h"
#include "../tpl/slist_tpl.h"


/** @file powernet.h Data structure to manage a net of powerlines - a powernet */


/**
 * Data class for power networks. A two phase queue to store
 * and hand out power.
 */
class powernet_t
{
public:
	/**
	 * Max power capacity of each network.
	 * Avoids possible overflows while providing a human friendly number.
	 */
	static const uint64 max_capacity;

	// number of fractional bits for network load values
	static const uint8 FRACTION_PRECISION;

	/**
	 * Must be called when a new map is started or loaded. Clears the table of networks.
	 */
	static void new_world();

	/// Steps all powernets
	static void step_all(uint32 delta_t);

private:
	static slist_tpl<powernet_t *> powernet_list;

	// Network power supply.
	uint64 power_supply;
	// Network power demand.
	uint64 power_demand;

	// Computed normalized demand.
	sint32 norm_demand;
	// Computed normalized supply.
	sint32 norm_supply;

	// Just transfers power demand and supply to current step
	void step(uint32 delta_t);

public:
	powernet_t();
	~powernet_t();

	uint64 get_max_capacity() const { return max_capacity; }

	/**
	 * Add power supply for next step.
	 */
	void add_supply(const uint32 p) { power_supply += (uint64)p; }

	/**
	 * Subtract power supply for next step.
	 */
	void sub_supply(const uint32 p) { power_supply -= (uint64)p; }

	/**
	 * Get the total power supply of the network.
	 */
	uint64 get_supply() const;

	/**
	 * Add power demand for next step.
	 */
	void add_demand(const uint32 p) { power_demand += (uint64)p; }

	/**
	 * Subtract power demand for next step.
	 */
	void sub_demand(const uint32 p) { power_demand -= (uint64)p; }

	/**
	 * Get the total power demand of the network.
	 */
	uint64 get_demand() const;

	/**
	 * Return the normalized value of demand in the network.
	 * Will have a logical value between 0 (no demand) and 1 (all supply consumed).
	 * Will have a logical value of 1 when no supply is present.
	 * Return value is fixed point with FRACTION_PRECISION fractional bits.
	 */
	sint32 get_normal_demand() const { return norm_demand; }

	/**
	 * Return the normalized value of supply in the network.
	 * Will have a logical value between 0 (no supply) and 1 (all demand supplied).
	 * Will have a logical value of 1 when no demand is present.
	 * Return value is fixed point with FRACTION_PRECISION fractional bits.
	 */
	sint32 get_normal_supply() const { return norm_supply; }
};

#endif
