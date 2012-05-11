/*
 * Copyright (c) 2007 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef crossing_logic_h
#define crossing_logic_h

#include "../simtypes.h"
#include "../tpl/minivec_tpl.h"
#include "../tpl/slist_tpl.h"

class cbuffer_t;
class crossing_t;
class karte_t;
class kreuzung_besch_t;
class vehikel_basis_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class crossing_logic_t
{
public:
	enum crossing_state_t { CROSSING_INVALID=0, CROSSING_OPEN, CROSSING_REQUEST_CLOSE, CROSSING_CLOSED };
protected:
	static karte_t *welt;

	// the last vehikel, taht request a closing
	const vehikel_basis_t *request_close;

	crossing_state_t zustand;
	const kreuzung_besch_t *besch;
	minivec_tpl<crossing_t *>crossings;

	void set_state( crossing_state_t new_state );

public:
	minivec_tpl<const vehikel_basis_t *>on_way1;
	minivec_tpl<const vehikel_basis_t *>on_way2;

public:
	// do not call th
	crossing_logic_t( const kreuzung_besch_t *besch );

	/**
	 * @return string (only used for debugg at the moment)
	 * @author prissi
	 */
	void info(cbuffer_t & buf) const;

	// recalcs the current state
	void recalc_state();

	// returns true, if the crossing can be passed by this vehicle
	bool request_crossing( const vehikel_basis_t * );

	// adds to crossing
	void add_to_crossing( const vehikel_basis_t *v );

	// removes the vehicle from the crossing
	void release_crossing( const vehikel_basis_t * );

	/* states of the crossing;
	 * since way2 has priority over way1 there is a third state, during a closing request
	 */
	crossing_state_t get_state() { return zustand; }

	void append_crossing( crossing_t *cr ) { crossings.append_unique(cr,14); }

	// static routines from here
private:
	static slist_tpl<const kreuzung_besch_t *> liste;
	// save all besch' only for waytype0 < waytype1
	static minivec_tpl<const kreuzung_besch_t *> can_cross_array[36];

public:
	static void register_besch(kreuzung_besch_t *besch);

	/**
	 * returns descriptor for crossing wrt timeline
	 * tries to match max-speeds:
	 * (1) find crossings with maxspeed close to requested maxspeed
	 * (2) prefer crossings with maxspeed larger than requested
	 */
	static const kreuzung_besch_t *get_crossing(const waytype_t ns, const waytype_t ow, sint32 way_0_speed, sint32 way_1_speed, uint16 timeline_year_month);

	// returns a new or an existing crossing_logic_t object
	// new, of no matching crossings are next to it
	static void add( karte_t *welt, crossing_t *cr, crossing_logic_t::crossing_state_t zustand );

	// remove logic from crossing(s)
	void remove( crossing_t *cr );
};

#endif
