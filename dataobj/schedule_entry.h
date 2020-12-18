/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SCHEDULE_ENTRY_H
#define DATAOBJ_SCHEDULE_ENTRY_H


#include "koord3d.h"
#include "../tpl/minivec_tpl.h"
#include "../simworld.h"

/**
 * A schedule entry.
 */
struct schedule_entry_t
{
public:
	schedule_entry_t() {}

	schedule_entry_t(koord3d const& pos, uint const minimum_loading, sint8 const waiting_time) :
		pos(pos),
		minimum_loading(minimum_loading),
		waiting_time(waiting_time)
	{}

	/**
	 * target position
	 */
	koord3d pos;

	/**
	 * Wait for % load at this stops
	 * (ignored on waypoints)
	 * If this value is greater than 100, waiting_time_shift contains a departure time
	 */
	uint8 minimum_loading;

	/**
	 * (only active if minimum_loading!=0)
	 * contains a departing time in ticks, relative to the length of the month
	 * The actual tick value is waiting_time << (tick_bit_per_month-16)
	 */
	uint16 waiting_time;

	sint32 get_waiting_ticks() const {
		return (sint32)((world()->ticks_per_world_month_shift >= 16) ? ((uint32)waiting_time << (world()->ticks_per_world_month_shift - 16)) : ((uint32)waiting_time >> (16 - world()->ticks_per_world_month_shift)));
	}
};

inline bool operator ==(const schedule_entry_t &a, const schedule_entry_t &b)
{
	return a.pos == b.pos  &&  a.minimum_loading == b.minimum_loading  &&  a.waiting_time == b.waiting_time;
}


#endif
