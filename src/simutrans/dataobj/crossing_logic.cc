/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../vehicle/vehicle_base.h"
#include "../world/simworld.h"
#include "../simsound.h"

#include "translator.h"

#include "../descriptor/crossing_desc.h"

#include "../utils/cbuffer.h"

#include "../tpl/slist_tpl.h"

#include "crossing_logic.h"
#include "../obj/crossing.h"


karte_ptr_t crossing_logic_t::welt;


crossing_logic_t::crossing_logic_t( const crossing_desc_t *desc )
{
	state = CROSSING_INVALID;
	this->desc = desc;
	request_close = NULL;
}


/**
 * @param[out] buf string; currently unused but useful for debugging
 */
void crossing_logic_t::info(cbuffer_t & buf) const
{
	static char const* const state_str[NUM_CROSSING_STATES] =
	{
		"invalid",
		"open",
		"request closing",
		"closed"
	};

	buf.printf("%s%u%s%u%s%s\n",
		translator::translate("\nway1 reserved by"), on_way1.get_count(),
		translator::translate("\nway2 reserved by"), on_way2.get_count(),
		translator::translate("cars.\nstate"), translator::translate(state_str[state])
	);
}


// after merging or splitting two crossings ...
void crossing_logic_t::recalc_state()
{
	if(  !crossings.empty()  ) {
		on_way1.clear();
		on_way2.clear();
		for(crossing_t* const i : crossings) {
			// add vehicles already there
			if (grund_t* const gr = welt->lookup(i->get_pos())) {
				for( uint8 i=3;  i<gr->get_top();  i++  ) {
					if(  vehicle_base_t const* const v = obj_cast<vehicle_base_t>(gr->obj_bei(i))  ) {
						add_to_crossing( v );
					}
				}
			}
		}
	}
	request_close = NULL;
	if(state==CROSSING_INVALID) {
		// now just set the state, if needed
		if(on_way2.empty()) {
			set_state( CROSSING_OPEN );
		}
		else {
			set_state( CROSSING_CLOSED );
		}
	}
}


// request permission to pass crossing
bool crossing_logic_t::request_crossing( const vehicle_base_t *v )
{
	if(v->get_waytype()==desc->get_waytype(0)) {
		if(on_way2.empty()  &&  state == CROSSING_OPEN) {
			// way2 is empty ...
			return true;
		}
		// passage denied, since there are vehicle on way2
		// which has priority
		// => ok only if I am already crossing
		return on_way1.is_contained(v);
	}
	else if(v->get_waytype()==desc->get_waytype(1)) {

		// vehicle from way2 arrives
		if(on_way1.get_count()) {
			// sorry, still things on the crossing, but we will prepare
			set_state( CROSSING_REQUEST_CLOSE );
			return false;
		}
		else {
			request_close = v;
			set_state( CROSSING_CLOSED );
			return true;
		}
	}
	// likely an airplane ...
	return true;
}


// request permission to pass crossing
void crossing_logic_t::add_to_crossing( const vehicle_base_t *v )
{
	if(  v->get_typ()!=obj_t::pedestrian  ) {
		if(v->get_waytype()==desc->get_waytype(0)) {
			on_way1.append_unique(v);
		}
		else if (v->get_waytype() == desc->get_waytype(1)) {
			// add it and close crossing
			on_way2.append_unique(v);
			if(  request_close==v  ) {
				request_close = NULL;
			}
			set_state( CROSSING_CLOSED );
		}
	}
}


// called after passing of the last vehicle (in a convoi)
// or of a city car; releases the crossing which may switch state
void crossing_logic_t::release_crossing( const vehicle_base_t *v )
{
	if(  v->get_waytype() == desc->get_waytype(0)  ) {
		on_way1.remove(v);
		if(  state == CROSSING_REQUEST_CLOSE  &&  on_way1.empty()  ) {
			set_state( CROSSING_CLOSED );
		}
	}
	else {
		on_way2.remove(v);
		if(  request_close == v  ) {
			request_close = NULL;
		}
		if(  on_way2.empty()  &&  request_close == NULL  ) {
			set_state( CROSSING_OPEN );
		}
	}
}


// change state; mark dirty and plays sound
void crossing_logic_t::set_state( crossing_state_t new_state )
{
	// play sound (if there and closing)
	if(new_state==CROSSING_CLOSED  &&  desc->get_sound()>=0  &&  !welt->is_fast_forward()) {
		welt->play_sound_area_clipped(crossings[0]->get_pos().get_2d(), desc->get_sound(), CROSSING_SOUND );
	}

	if(new_state!=state) {
		state = new_state;
		for(crossing_t* const i : crossings) {
			i->state_changed();
		}
	}
}


/* static stuff from here on ... */


/**
 * nothing can cross airways, so waytype 0..7 is enough
 * only save this entries:
 * way0 way1
 *  0 .. 1 2 3 4 5 6 7 8
 *  1 ..   2 3 4 5 6 7 8
 *  2 ..     3 4 5 6 7 8
 * ..          ...
 */
minivec_tpl<const crossing_desc_t *> crossing_logic_t::can_cross_array[36];


/**
 * compare crossings for the same waytype-combinations
 */
int compare_crossing(const crossing_desc_t *c0, const crossing_desc_t *c1)
{
	// sort descending wrt maxspeed
	int diff = c1->get_maxspeed(0) - c0->get_maxspeed(0);
	if (diff==0) {
		diff = c1->get_maxspeed(1) - c0->get_maxspeed(1);
	}
	if (diff==0) {
		diff = strcmp(c0->get_name(), c1->get_name());
	}
	return diff;
}


void crossing_logic_t::register_desc(crossing_desc_t *desc)
{
	// mark if crossing possible
	const waytype_t way0 = (waytype_t)min(desc->get_waytype(0), desc->get_waytype(1));
	const waytype_t way1 = (waytype_t)max(desc->get_waytype(0), desc->get_waytype(1));
	if (way1 == way0) {
		dbg->error("crossing_logic_t::register_desc()", "Same waytype for both ways for crossing %S: Not added!", desc->get_name());
		return;
	}
	if(  way0<8  &&  way1<9  ) {
		uint8 index = way0 * 9 + way1 - ((way0+2)*(way0+1))/2;
		// max index = 7*9 + 8 - 9*4 = 71-36 = 35
		// .. overwrite double entries
		minivec_tpl<const crossing_desc_t *> &vec = can_cross_array[index];
		// first check for existing crossing with the same name
		for(uint8 i=0; i<vec.get_count(); i++) {
			if (strcmp(vec[i]->get_name(), desc->get_name())==0) {
				vec.remove_at(i);
				dbg->doubled( "crossing", desc->get_name() );
			}
		}
DBG_DEBUG( "crossing_logic_t::register_desc()","%s", desc->get_name() );
		// .. then make sorted insert
		for(uint8 i=0; i<vec.get_count(); i++) {
			if (compare_crossing(desc, vec[i])<0) {
				vec.insert_at(i, desc);
				return;
			}
		}
		vec.append(desc);
	}
}


const crossing_desc_t *crossing_logic_t::get_crossing(const waytype_t w0, const waytype_t w1, sint32 way_0_speed, sint32 way_1_speed, uint16 timeline_year_month)
{
	// mark if crossing possiblea
	const waytype_t way0 = w0 < w1 ? w0 : w1;
	const waytype_t way1 = w0 < w1 ? w1 : w0;
	const sint32 speed0 = w0 < w1 ? way_0_speed : way_1_speed;
	const sint32 speed1 = w0 < w1 ? way_1_speed : way_0_speed;
	const crossing_desc_t *best = NULL;
	// index 8 is narrowgauge, only air_wt and powerline_wt have higher indexes
	if(  way0 <= 8  &&  way1 <= 8  &&  way0 != way1  ) {

		const uint8 index = way0 * 9 + way1 - ((way0+2)*(way0+1))/2;
		for(crossing_desc_t const* const i :  can_cross_array[index]  ) {
			if(  !i->is_available(timeline_year_month)  ) {
				continue;
			}
			// match maxspeed of first way
			uint8  const swap_way = i->get_waytype(0) != way0;
			sint32 const imax0   = i->get_maxspeed(swap_way);
			sint32 const bmax0   = best ? best->get_maxspeed(swap_way) : 9999;
			if(  imax0 >= speed0  &&  imax0 <= bmax0  ) {
				// match maxspeed of second way
				sint32 const imax1   = i->get_maxspeed(!swap_way);
				sint32 const bmax1   = best ? best->get_maxspeed(!swap_way) : 9999;
				if(  imax1 >= speed1  &&  imax1 <= bmax1  ) {
					best = i;
				}
			}
		}
	}
	return best;
}


/**
 * compare crossings for the same waytype-combinations
 */
bool have_crossings_same_wt(const crossing_desc_t *c0, const crossing_desc_t *c1)
{
	return c0->get_waytype(0) == c1->get_waytype(0)  &&  c0->get_waytype(1) == c1->get_waytype(1);
}


// returns a new or an existing crossing_logic_t object
// new, of no matching crossings are next to it
void crossing_logic_t::add( crossing_t *start_cr, crossing_state_t state )
{
	koord3d pos = start_cr->get_pos();
	const koord zv = start_cr->get_dir() ? koord::west : koord::north;
	slist_tpl<crossing_t *>crossings;
	minivec_tpl<crossing_logic_t *>crossings_logics;

	crossings.append_unique( start_cr );
	if (crossing_logic_t *start_logic = start_cr->get_logic() ) {
		crossings_logics.append(start_logic);
	}
	// go north/west
	while(1) {
		pos += zv;
		grund_t *gr = welt->lookup( pos );
		if(gr==NULL) {
			break;
		}
		crossing_t *found_cr = gr->find<crossing_t>();
		if(found_cr==NULL  ||  !have_crossings_same_wt(found_cr->get_desc(),start_cr->get_desc())  ||  start_cr->get_dir() != found_cr->get_dir()) {
			break;
		}
		crossings.append( found_cr );
		if(  found_cr->get_logic()!=NULL  ) {
			crossings_logics.append_unique( found_cr->get_logic() );
		}
	}
	// go east/south
	pos = start_cr->get_pos();
	while(1) {
		pos -= zv;
		grund_t *gr = welt->lookup( pos );
		if(gr==NULL) {
			break;
		}
		crossing_t *found_cr = gr->find<crossing_t>();
		if(found_cr==NULL  ||  !have_crossings_same_wt(found_cr->get_desc(),start_cr->get_desc())  ||  start_cr->get_dir() != found_cr->get_dir()) {
			break;
		}
		crossings.append( found_cr );
		if(  found_cr->get_logic()!=NULL  ) {
			crossings_logics.append_unique( found_cr->get_logic() );
		}
	}
	// remove all old crossing logics
	crossing_logic_t *found_logic = NULL;
	if(  crossings_logics.get_count()>=1  ) {
		// leave one logic to be used further
		while(  crossings_logics.get_count()>1  ) {
			crossing_logic_t *cl = crossings_logics[0];
			crossings_logics.remove_at(0);
			delete cl;
		}
		found_logic = crossings_logics[0];
	}
	// no old logic there create a new one
	if(  found_logic == NULL ) {
		found_logic = new crossing_logic_t( start_cr->get_desc() );
	}

	// set new crossing logic to all
	for(crossing_t* const cr : crossings) {
		cr->set_logic( found_logic );
		found_logic->append_crossing( cr );
	}
	found_logic->set_state( state );
	found_logic->recalc_state();
}


// removes a crossing logic, if all crossings are removed
void crossing_logic_t::remove( crossing_t *cr )
{
	crossings.remove( cr );
	if(  crossings.empty()  ) {
		delete this;
	}
	else {
		// check for a crossing to the east/south
		koord3d pos = cr->get_pos();
		const koord zv = cr->get_dir() ? koord::west : koord::north;
		const grund_t *gr = welt->lookup( pos-zv );
		if(  gr  ) {
			crossing_t *found_cr = gr->find<crossing_t>();
			if(  found_cr  &&  have_crossings_same_wt(found_cr->get_desc(),cr->get_desc())  ) {
				// crossing to the east/south so split logic from any found to the north/west
				crossing_logic_t *split_logic = NULL;
				while(1) {
					pos += zv;
					gr = welt->lookup( pos );
					if(  gr == NULL  ) {
						break;
					}
					found_cr = gr->find<crossing_t>();
					if(  found_cr == NULL  ||  !have_crossings_same_wt(found_cr->get_desc(),cr->get_desc())  ) {
						break;
					}
					assert(this==found_cr->get_logic());

					if(  !split_logic  ) {
						split_logic = new crossing_logic_t( cr->get_desc() );
					}
					crossings.remove( found_cr );
					found_cr->set_logic( split_logic );
					split_logic->append_crossing( found_cr );
				}

				if(  split_logic  ) {
					split_logic->set_state( CROSSING_INVALID );
					split_logic->recalc_state();
				}
			}
		}
		set_state( CROSSING_INVALID );
		recalc_state();
	}
}
