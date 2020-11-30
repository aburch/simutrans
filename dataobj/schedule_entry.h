/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SCHEDULE_ENTRY_H
#define DATAOBJ_SCHEDULE_ENTRY_H


#include "koord3d.h"

/**
 * A schedule entry.
 */
struct schedule_entry_t
{
public:
	schedule_entry_t() {}

	schedule_entry_t(koord3d const& pos, uint16 const minimum_loading, sint8 const waiting_time_shift, sint16 spacing_shift, sint8 reverse, bool wait_for_time) :
		pos(pos),
		minimum_loading(minimum_loading),
		spacing_shift(spacing_shift),
		waiting_time_shift(waiting_time_shift),
		reverse(reverse),
		wait_for_time(wait_for_time)
	{}

	/**
	 * target position
	 */
	koord3d pos;

	/**
	 * Wait for % load at this stops
	 * (ignored on waypoints)
	 */
	uint16 minimum_loading;

	/**
	 * spacing shift
	 */
	sint16 spacing_shift;

	/**
	* maximum waiting time in 1/2^(16-n) parts of a month
	* (only active if minimum_loading!=0)
	*/
	sint8 waiting_time_shift;

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

inline bool operator ==(const schedule_entry_t &a, const schedule_entry_t &b)
{
	return a.pos == b.pos  &&  a.minimum_loading == b.minimum_loading  &&  a.waiting_time_shift == b.waiting_time_shift;
}


#endif
