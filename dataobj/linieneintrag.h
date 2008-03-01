#ifndef linieneintrag_t_h
#define linieneintrag_t_h

#include "koord3d.h"

/**
 * Ein Fahrplaneintrag.
 * @author Hj. Malthaner
 */
struct linieneintrag_t
{
public:
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
	 * maximum waiting time in 1/2^n parts of a month
	 * (only active if ladegrad!=0)
	 * @author prissi
	 */
	uint8 waiting_time_shift;
};

#endif
