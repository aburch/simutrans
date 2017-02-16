#ifndef schedule_entry_h
#define schedule_entry_h

#include "koord3d.h"

/**
 * A schedule entry.
 * @author Hj. Malthaner
 */
struct schedule_entry_t
{
public:
	schedule_entry_t() {}

	schedule_entry_t(koord3d const& pos, uint16 const minimum_loading, sint8 const waiting_time_shift, sint16 spacing_shift, sint8 reverse, bool wait_for_time) :
		pos(pos),
		minimum_loading(minimum_loading),
		waiting_time_shift(waiting_time_shift),
		spacing_shift(spacing_shift),
		reverse(reverse),
		wait_for_time(wait_for_time)
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
	uint16 minimum_loading;

	/**
	 * maximum waiting time in 1/2^(16-n) parts of a month
	 * (only active if minimum_loading!=0)
	 * @author prissi
	 */
	sint8 waiting_time_shift;

	/**
	 * spacing shift
	 * @author Inkelyad
	 */
	sint16 spacing_shift;

	/**
	 * Whether a convoy needs to reverse after this entry.
	 * 0 = no; 1 = yes; -1 = undefined
	 * @author: jamespetts
	 */
	sint8 reverse;
	
	/**
	 * Whether a convoy must wait for a 
	 * time slot at this entry.
	 * @author: jamespetts
	 */
	bool wait_for_time;
};

#endif
