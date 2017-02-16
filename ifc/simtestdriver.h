/*
 * Interface to connect the vehicle with its route
 *
 * 15.01.00, Hj. Malthaner
 */

#ifndef SIMTESTDRIVER_H
#define SIMTESTDRIVER_H


class grund_t;

/**
 * Interface to connect the vehicle with its route
 *
 * @author Hj. Malthaner, 15.01.00
 */
class test_driver_t
{
public:
	virtual ~test_driver_t() {}

	virtual bool check_next_tile(const grund_t* ) const = 0;

	/**
	 * Determine the direction bits (ribi) for the applicable vehicle,
	 * Depends of the ground type.
	 *
	 * @author Hj. Malthaner, 03.01.01
	 */
	virtual ribi_t::ribi get_ribi(const grund_t* ) const = 0;

	virtual waytype_t get_waytype() const = 0;

	// how expensive to go here (for way search) with the maximum convoi speed as second parameter
	virtual int get_cost(const grund_t *, const sint32, koord from_pos) = 0;

	// returns true for the way search to an unknown target.
	// first is current ground, second is starting ground
	virtual bool is_target(const grund_t *,const grund_t *) = 0;

	// return the cost of a single step upwards
	virtual uint32 get_cost_upslope() const { return 0; } // Standard is 25
};

#endif
