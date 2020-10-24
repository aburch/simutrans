/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IFC_SIMTESTDRIVER_H
#define IFC_SIMTESTDRIVER_H


#include "../simtypes.h"
#include "../dataobj/ribi.h"


class grund_t;
class weg_t;


/**
 * Interface to connect the vehicle with its route
 */
class test_driver_t
{
public:
	virtual ~test_driver_t() {}

	virtual bool check_next_tile(const grund_t* ) const = 0;

	/**
	 * Determine the direction bits (ribi) for the applicable vehicle,
	 * Depends of the ground type.
	 */
	virtual ribi_t::ribi get_ribi(const grund_t* ) const = 0;

	virtual waytype_t get_waytype() const = 0;

	/**
	 * How expensive to go here (for way search).
	 * @param gr tile to check
	 * @param w way on the tile (can be NULL)
	 * @param max_speed the maximum convoi speed as second parameter
	 * @param from direction in which we enter the tile
	 */
	virtual int get_cost(const grund_t *gr, const weg_t *w, const sint32 max_speed, ribi_t::ribi from) const = 0;

	// returns true for the way search to an unknown target.
	// first is current ground, second is starting ground
	virtual bool is_target(const grund_t *,const grund_t *) const = 0;

	// return the cost of a single step upwards
	virtual uint32 get_cost_upslope() const { return 0; }
};

#endif
