/*
 * Interface to connect the vehicle with its route
 *
 * 15.01.00, Hj. Malthaner
 */

#ifndef fahrer_h
#define fahrer_h


class grund_t;

/**
 * Interface to connect the vehicle with its route
 *
 * @author Hj. Malthaner, 15.01.00
 */
class fahrer_t
{
public:
	virtual ~fahrer_t() {}

	virtual bool ist_befahrbar(const grund_t* ) const = 0;

	/**
	 * Determine the direction bits (ribi) for the applicable vehicle,
	 * Depends of the ground type.
	 *
	 * @author Hj. Malthaner, 03.01.01
	 */
	virtual ribi_t::ribi get_ribi(const grund_t* ) const = 0;

	virtual waytype_t get_waytype() const = 0;

	// how expensive to go here (for way search) with the maximum convoi speed as second parameter
	virtual int get_kosten(const grund_t *, const sint32, koord from_pos) const = 0;

	// returns true for the way search to an unknown target.
	// first is current ground, second is starting ground
	virtual bool ist_ziel(const grund_t *,const grund_t *) const = 0;
};

#endif
