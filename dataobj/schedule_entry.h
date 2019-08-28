#ifndef schedule_entry_t_h
#define schedule_entry_t_h

#include "koord3d.h"

/**
 * A schedule entry.
 * @author Hj. Malthaner
 */
struct schedule_entry_t
{
public:
	schedule_entry_t() {}

	schedule_entry_t(koord3d const& pos, uint const minimum_loading, sint8 const waiting_time_shift, uint8 const coupling_point) :
		pos(pos),
		minimum_loading(minimum_loading),
		waiting_time_shift(waiting_time_shift),
		coupling_point(coupling_point)
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
	uint8 minimum_loading;

	/**
	 * maximum waiting time in 1/2^(16-n) parts of a month
	 * (only active if minimum_loading!=0)
	 * @author prissi
	 */
	sint8 waiting_time_shift;
	
	/**
	 * Is this entry a coupling or uncoupling point?
	 * 0... nothing
	 * 1... the convoy wait for the child
	 * 2... the convoy find a parent and couple with. 
	 * @author THLeaderH
	 */
	uint8 coupling_point;
};

inline bool operator ==(const schedule_entry_t &a, const schedule_entry_t &b)
{
	return a.pos == b.pos  &&  a.minimum_loading == b.minimum_loading  &&  a.waiting_time_shift == b.waiting_time_shift  &&  a.coupling_point == b.coupling_point;
}


#endif
