/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "powernet.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t netlist_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

extern const uint32 POWER_TO_MW; // defined in leitung2

// maximum possible limit is (1 << (63 - FRACTION_PRECISION))
const uint64 powernet_t::max_capacity = (uint64)4000000 << POWER_TO_MW; // (4 TW)

const uint8 powernet_t::FRACTION_PRECISION = 16;


slist_tpl<powernet_t *> powernet_t::powernet_list;


void powernet_t::new_world()
{
	while(!powernet_list.empty()) {
		powernet_t *net = powernet_list.remove_first();
		delete net;
	}
}


void powernet_t::step_all(uint32 delta_t)
{
	for(powernet_t* const p : powernet_list) {
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

	power_supply = 0;
	power_demand = 0;

	norm_demand = 1 << FRACTION_PRECISION;
	norm_supply = 1 << FRACTION_PRECISION;
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

/**
 * Computes a normalized supply/demand value.
 * If demand fully satisfies supply or supply is 0 then output is 1.
 */
sint32 compute_norm_sd(uint64 const demand, uint64 const supply)
{
	// compute demand factor
	if(  (demand  >=  supply)  ||  (supply  ==  0)  ) {
		return (sint32)1 << powernet_t::FRACTION_PRECISION;
	}
	else {
		return (sint32)((demand << powernet_t::FRACTION_PRECISION) / supply);
	}
}

void powernet_t::step(uint32 delta_t)
{
	if(  delta_t==0  ) {
		return;
	}

	// get limited values
	uint64 const supply = get_supply();
	uint64 const demand = get_demand();

	// compute normalized demand
	norm_demand = compute_norm_sd(demand, supply);
	norm_supply = compute_norm_sd(supply, demand);
}

/**
 * Clamps a power value to be within the maximum capacity.
 */
uint64 clamp_power(uint64 const power)
{
	return power > powernet_t::max_capacity ? powernet_t::max_capacity : power;
}

uint64 powernet_t::get_supply() const
{
	return clamp_power(power_supply);
}

uint64 powernet_t::get_demand() const
{
	return clamp_power(power_demand);
}
