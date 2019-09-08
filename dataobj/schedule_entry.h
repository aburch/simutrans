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

	schedule_entry_t(koord3d const& pos, uint const minimum_loading, sint8 const waiting_time_shift, uint8 const stop_flags) :
		pos(pos),
		minimum_loading(minimum_loading),
		waiting_time_shift(waiting_time_shift),
		stop_flags(stop_flags)
	{}

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
	
	uint16 spacing, spacing_shift, delay_tolerance;
	
private:
	uint8 stop_flags;
	
public:
	uint8 get_coupling_point() const { return (stop_flags&0x03); }
	void set_wait_for_coupling() { stop_flags |= WAIT_FOR_COUPLING; stop_flags &= ~TRY_COUPLING; }
	void set_try_coupling() { stop_flags |= TRY_COUPLING; stop_flags &= ~WAIT_FOR_COUPLING; }
	void reset_coupling() { stop_flags &= ~TRY_COUPLING; stop_flags &= ~WAIT_FOR_COUPLING; }
	uint8 get_stop_flags() const { return stop_flags; }
	void set_stop_flags(uint8 f) { stop_flags = f; }
	
	uint64 get_combined_spacing() const { return spacing+(spacing_shift<<16)+(delay_tolerance<<32); }
	void set_combined_spacing(uint64 s) {
		spacing = (s&0x000f);
		spacing_shift = (s&0x00f0)>>16;
		delay_tolerance = (s&0x0f00)>>32;
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
