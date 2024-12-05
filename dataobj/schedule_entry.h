/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SCHEDULE_ENTRY_H
#define DATAOBJ_SCHEDULE_ENTRY_H

#define NUM_ARRIVAL_TIME_STORED 5
#define NUM_WAITING_TIME_STORED 5
#define NUM_STOPPING_TIME_STORED 5

#include "koord3d.h"

/**
 * A schedule entry.
 */
struct schedule_entry_t
{
public:
	schedule_entry_t() {
		init_journey_time();
		init_waiting_time();
		init_convoy_stopping_time();
	}

	schedule_entry_t(koord3d const& pos, uint const minimum_loading, uint16 const waiting_time_shift, uint8 const stop_flags) :
		pos(pos),
		minimum_loading(minimum_loading),
		waiting_time_shift(waiting_time_shift),
		stop_flags(stop_flags)
	{
		spacing = 1;
		spacing_shift = delay_tolerance = 0;
		init_journey_time();
		init_waiting_time();
		init_convoy_stopping_time();
	}

	enum {
		NONE              = 0,
		WAIT_FOR_COUPLING = 1U << 0, // The convoy waits for the child to couple with.
		TRY_COUPLING      = 1U << 1, // The convoy finds a parent and couple with. 
		NO_LOAD           = 1U << 2, // The convoy loads nothing here.
		NO_UNLOAD         = 1U << 3, // The convoy unloads nothing here.
		WAIT_FOR_TIME     = 1U << 4, // The convoy waits for the departure time.
		UNLOAD_ALL        = 1U << 5, // The convoy unloads all loads here.
		LOAD_BEFORE_DEP   = 1U << 6, // The convoy loads just before the departure.
		TRANSFER_INTERVAL = 1U << 7,
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
	 * maximum waiting time in 1/n parts of a month
	 * (only active if minimum_loading!=0)
	 */
	uint16 waiting_time_shift;
	
	uint16 spacing, spacing_shift, delay_tolerance;
	
	/*
	 * store last 5 journey time of this stop.
	 * This is the time between the arrival at the previous stop and the arrival at this stop.
	 * time = 0 means that journey time is not registered.
	 */
	uint32 journey_time[NUM_ARRIVAL_TIME_STORED];
	uint8 jt_at_index; // which index of journey_time should be overwritten next.

	/*
	 * store last 5 average waiting times of goods to be loaded at this stop.
	 * time = 0 means that waiting time is not registered.
	 */
	uint32 waiting_time[NUM_WAITING_TIME_STORED];
	uint8 wt_at_index; // which index of waiting_time should be overwritten next.


	/*
	 * store last 5 convoy stopping times at this stop.
	 * time = 0 means that stopping time is not registered.
	 */
	uint32 convoy_stopping_time[NUM_STOPPING_TIME_STORED];
	uint8 cs_at_index; // which index of convoy_stopping_time should be overwritten next.
	
private:
	uint8 stop_flags;
	
	void init_journey_time() {
		jt_at_index = 0;
		for(uint8 i = 0; i < NUM_ARRIVAL_TIME_STORED; i++) {
			journey_time[i] = 0;
		}
	}

	void init_waiting_time() {
		wt_at_index = 0;
		for(uint8 i = 0; i < NUM_WAITING_TIME_STORED; i++) {
			waiting_time[i] = 0;
		}
	}

	void init_convoy_stopping_time() {
		cs_at_index = 0;
		for(uint8 i = 0; i < NUM_STOPPING_TIME_STORED; i++) {
			convoy_stopping_time[i] = 0;
		}
	}
	
public:
	uint8 get_coupling_point() const { return (stop_flags&0x03); }
	void set_wait_for_coupling() { stop_flags |= WAIT_FOR_COUPLING; stop_flags &= ~TRY_COUPLING; }
	void set_try_coupling() { stop_flags |= TRY_COUPLING; stop_flags &= ~WAIT_FOR_COUPLING; }
	void reset_coupling() { stop_flags &= ~TRY_COUPLING; stop_flags &= ~WAIT_FOR_COUPLING; }
	bool is_no_load() const { return (stop_flags&NO_LOAD)>0; }
	void set_no_load(bool y) { y ? stop_flags |= NO_LOAD : stop_flags &= ~NO_LOAD; }
	bool is_no_unload() const { return (stop_flags&NO_UNLOAD)>0; }
	void set_no_unload(bool y) { y ? stop_flags |= NO_UNLOAD : stop_flags &= ~NO_UNLOAD; }
	bool is_unload_all() const { return (stop_flags&UNLOAD_ALL)>0; }
	void set_unload_all(bool y) { y ? stop_flags |= UNLOAD_ALL : stop_flags &= ~UNLOAD_ALL; }
	bool get_wait_for_time() const { return (stop_flags&WAIT_FOR_TIME)>0; }
	void set_wait_for_time(bool y) { y ? stop_flags |= WAIT_FOR_TIME : stop_flags &= ~WAIT_FOR_TIME; }
	bool is_load_before_departure() const { return (stop_flags&LOAD_BEFORE_DEP)>0; }
	void set_load_before_departure(bool y) { y ? stop_flags |= LOAD_BEFORE_DEP : stop_flags &= ~LOAD_BEFORE_DEP; }
	bool is_transfer_interval() const { return (stop_flags&TRANSFER_INTERVAL)>0; }
	void set_transfer_interval(bool y) { y ? stop_flags |= TRANSFER_INTERVAL : stop_flags &= ~TRANSFER_INTERVAL; }
	uint8 get_stop_flags() const { return stop_flags; }
	void set_stop_flags(uint8 f) { stop_flags = f; }
	
	void set_spacing(uint16 a, uint16 b, uint16 c) {
		spacing = a;
		spacing_shift = b;
		delay_tolerance = c;
	}

	void push_journey_time(uint32 time);
	void push_waiting_time(uint32 time);
	void push_convoy_stopping_time(uint32 time);
	
	uint32 get_median_journey_time() const;
	uint32 get_average_waiting_time() const;
	uint32 get_median_convoy_stopping_time() const;
	
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
