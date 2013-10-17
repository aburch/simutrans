#ifndef linieneintrag_t_h
#define linieneintrag_t_h

#include "koord3d.h"

/**
 * A schedule entry.
 * @author Hj. Malthaner
 */
struct linieneintrag_t
{
public:
	linieneintrag_t() {}

	linieneintrag_t(koord3d const& pos, uint const ladegrad, sint8 const waiting_time_shift) :
		pos(pos),
		ladegrad(ladegrad),
		waiting_time_shift(waiting_time_shift)
	{}

	/**
	 * target position
	 * @author Hj. Malthaner
	 */
	koord3d pos;

	/**
	 * Wait for % load at this stops
	 * (ignored on waypoints)
	 * @author Hj. Malthaner
	 */
	uint8 ladegrad;

	/**
	 * maximum waiting time in 1/2^(16-n) parts of a month
	 * (only active if ladegrad!=0)
	 * @author prissi
	 */
	sint8 waiting_time_shift;
};

#endif
