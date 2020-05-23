/*
 * This file is part of the Simutrans project under the Artistic License.
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
	schedule_entry_t() {
		// journey time is not loaded or saved.
		journey_time[0] = journey_time[1] = journey_time[2] = 0;
		jtime_index = 0;
	}

	schedule_entry_t(koord3d const& pos, uint const minimum_loading, sint8 const waiting_time_shift, uint8 const stop_flags) :
		pos(pos),
		minimum_loading(minimum_loading),
		waiting_time_shift(waiting_time_shift),
		stop_flags(stop_flags)
	{
		spacing = 1;
		spacing_shift = delay_tolerance = 0;
		journey_time[0] = journey_time[1] = journey_time[2] = 0;
		jtime_index = 0;
	}

	enum {
		NONE              = 0,
		WAIT_FOR_COUPLING = 1U << 0, // The convoy waits for the child to couple with.
		TRY_COUPLING      = 1U << 1, // The convoy finds a parent and couple with. 
		NO_LOAD           = 1U << 2, // The convoy loads nothing here.
		NO_UNLOAD         = 1U << 3, // The convoy unloads nothing here.
		WAIT_FOR_TIME     = 1U << 4  // The convoy waits for the departure time.
	};

	/**
	 * target position
	 */
	koord3d pos;

	/**
	 * Wait for % load at this stops
	 * (ignored on waypoints)
	 */
	uint8 minimum_loading;

	/**
	 * maximum waiting time in 1/2^(16-n) parts of a month
	 * (only active if minimum_loading!=0)
	 */
	sint8 waiting_time_shift;
	
	uint16 spacing, spacing_shift, delay_tolerance;
	
	/*
	 * store last 3 lengths of journey time which a convoy took from a prev stop.
	 * time = 0 means that journey time is not registered.
	 * journey times are not saved to reduce save/load time.
	 */
	uint32 journey_time[4];
	uint8 jtime_index; // which index of journey_time should be overwritten next.
	
private:
	uint8 stop_flags;
	
public:
	uint8 get_coupling_point() const { return (stop_flags&0x03); }
	void set_wait_for_coupling() { stop_flags |= WAIT_FOR_COUPLING; stop_flags &= ~TRY_COUPLING; }
	void set_try_coupling() { stop_flags |= TRY_COUPLING; stop_flags &= ~WAIT_FOR_COUPLING; }
	void reset_coupling() { stop_flags &= ~TRY_COUPLING; stop_flags &= ~WAIT_FOR_COUPLING; }
	bool is_no_load() const { return (stop_flags&NO_LOAD)>0; }
	void set_no_load(bool y) { y ? stop_flags |= NO_LOAD : stop_flags &= ~NO_LOAD; }
	bool is_no_unload() const { return (stop_flags&NO_UNLOAD)>0; }
	void set_no_unload(bool y) { y ? stop_flags |= NO_UNLOAD : stop_flags &= ~NO_UNLOAD; }
	bool get_wait_for_time() const { return (stop_flags&WAIT_FOR_TIME)>0; }
	void set_wait_for_time(bool y) { y ? stop_flags |= WAIT_FOR_TIME : stop_flags &= ~WAIT_FOR_TIME; }
	uint8 get_stop_flags() const { return stop_flags; }
	void set_stop_flags(uint8 f) { stop_flags = f; }
	
	void set_spacing(uint16 a, uint16 b, uint16 c) {
		spacing = a;
		spacing_shift = b;
		delay_tolerance = c;
	}
	
	void push_journey_time(uint32 t) {
		journey_time[jtime_index] = t;
		jtime_index = (jtime_index+1)%3;
	}
	
	bool operator ==(const schedule_entry_t &a) {
		return a.pos == this->pos
		  &&  a.minimum_loading    == this->minimum_loading
			&&  a.waiting_time_shift == this->waiting_time_shift
			&&  a.get_stop_flags()   == this->stop_flags
			&&  a.spacing            == this->spacing
			&&  a.spacing_shift      == this->spacing_shift
			&&  a.delay_tolerance    == this->delay_tolerance;
	}
};


#endif
