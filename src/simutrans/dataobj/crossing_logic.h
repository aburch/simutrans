/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_CROSSING_LOGIC_H
#define DATAOBJ_CROSSING_LOGIC_H


#include "../simtypes.h"
#include "../tpl/minivec_tpl.h"
#include "../tpl/slist_tpl.h"

class cbuffer_t;
class crossing_t;
class karte_ptr_t;
class crossing_desc_t;
class vehicle_base_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 */
class crossing_logic_t
{
public:
	enum crossing_state_t {
		CROSSING_INVALID = 0,
		CROSSING_OPEN,
		CROSSING_REQUEST_CLOSE,
		CROSSING_CLOSED,
		NUM_CROSSING_STATES
	};

protected:
	static karte_ptr_t welt;

	// the last vehicle that requested a closing
	const vehicle_base_t *request_close;

	crossing_state_t state;
	const crossing_desc_t *desc;
	minivec_tpl<crossing_t *>crossings;

	void set_state( crossing_state_t new_state );

public:
	minivec_tpl<const vehicle_base_t *>on_way1;
	minivec_tpl<const vehicle_base_t *>on_way2;

public:
	crossing_logic_t( const crossing_desc_t *desc );

	/**
	 * @param[out] buf string (only used for debug at the moment)
	 */
	void info(cbuffer_t & buf) const;

	// recalcs the current state
	void recalc_state();

	// returns true, if the crossing can be passed by this vehicle
	bool request_crossing( const vehicle_base_t * );

	// adds to crossing
	void add_to_crossing( const vehicle_base_t *v );

	// removes the vehicle from the crossing
	void release_crossing( const vehicle_base_t * );

	/* states of the crossing;
	 * since way2 has priority over way1 there is a third state, during a closing request
	 */
	crossing_state_t get_state() { return state; }

	void append_crossing( crossing_t *cr ) { crossings.append_unique(cr); }

	// static routines from here
private:
	static slist_tpl<const crossing_desc_t *> list;
	// save all desc' only for waytype0 < waytype1
	static minivec_tpl<const crossing_desc_t *> can_cross_array[36];

public:
	static void register_desc(crossing_desc_t *desc);

	/**
	 * returns descriptor for crossing wrt timeline
	 * tries to match max-speeds:
	 * (1) find crossings with maxspeed close to requested maxspeed
	 * (2) prefer crossings with maxspeed larger than requested
	 */
	static const crossing_desc_t *get_crossing(const waytype_t ns, const waytype_t ow, sint32 way_0_speed, sint32 way_1_speed, uint16 timeline_year_month);

	// returns a new or an existing crossing_logic_t object
	// new, of no matching crossings are next to it
	static void add( crossing_t *cr, crossing_logic_t::crossing_state_t state );

	// remove logic from crossing(s)
	void remove( crossing_t *cr );
};

#endif
