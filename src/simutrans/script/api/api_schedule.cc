/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_schedule.cc exports schedule structure and related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/schedule.h"
#include "../../simhalt.h"
#include "../../simintr.h"
#include "../../world/simworld.h"

using namespace script_api;


halthandle_t get_halt_from_koord3d(koord3d pos, const player_t *player )
{
	if(  player == NULL  ) {
		return halthandle_t();
	}
	return haltestelle_t::get_halt(pos, player);
}

SQInteger waiting_time_to_string(HSQUIRRELVM vm)
{
	schedule_entry_t entry(koord3d::invalid, 0, 0);
	get_slot(vm, "wait", entry.waiting_time, -1);
	plainstring str = difftick_to_string(entry.get_waiting_ticks(), false);
	return param<plainstring>::push(vm, str);
}

SQInteger schedule_constructor(HSQUIRRELVM vm) // instance, wt, entries
{
	waytype_t wt = param<waytype_t>::get(vm, 2);
	SQInteger res = set_slot(vm, "waytype", wt, 1);

	if (SQ_SUCCEEDED(res)) {
		sq_pushstring(vm, "entries", -1);
		sq_push(vm, 3); // entries
		res = sq_set(vm, 1);
	}
	if (SQ_SUCCEEDED(res)) {
		// attach a schedule instance
		// the c++ schedule_t instance will be refilled everytime when transfered to the c++ side of things.
		schedule_t* sched = NULL;
		switch(wt) {
			case road_wt:        sched = new truck_schedule_t(); break;
			case track_wt:       sched = new train_schedule_t(); break;
			case water_wt:       sched = new ship_schedule_t(); break;
			case air_wt:         sched = new airplane_schedule_t(); break;
			case monorail_wt:    sched = new monorail_schedule_t(); break;
			case tram_wt:        sched = new tram_schedule_t(); break;
			case maglev_wt:      sched = new maglev_schedule_t(); break;
			case narrowgauge_wt: sched = new narrowgauge_schedule_t(); break;
			default:
				sq_raise_error(vm, "Invalid waytype %d", wt);
				return SQ_ERROR;
		}
		attach_instance(vm, 1, sched);
	}
	return res;
}

// read entry at index, append to schedule
void append_entry(HSQUIRRELVM vm, SQInteger index, schedule_t* sched)
{
	koord3d pos = param<koord3d>::get(vm, index);

	uint8 minimum_loading = 0;
	get_slot(vm, "load", minimum_loading, index);

	uint16 waiting_time_shift = 0;
	get_slot(vm, "wait", waiting_time_shift, index);

	grund_t *gr = welt->lookup(pos);
	if (gr) {
		sched->append(gr, minimum_loading, waiting_time_shift);
	}
}

schedule_t* script_api::param<schedule_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	// get waytype
	waytype_t wt = invalid_wt;
	get_slot(vm, "waytype", wt, index);

	// get instance pointer
	schedule_t* sched = get_attached_instance<schedule_t>(vm, index, param<schedule_t*>::tag());
	if (sched) {
		sched->entries.clear();
		// now read the entries
		sq_pushstring(vm, "entries", -1);
		SQInteger new_index = index > 0 ? index : index-1;
		if (SQ_SUCCEEDED(sq_get(vm, new_index))) {
			// now entries array at the top of the stack
			// foreach loop
			sq_pushnull(vm);
			while(SQ_SUCCEEDED(sq_next(vm, -2))) {
				append_entry(vm, -1, sched);
				sq_pop(vm, 2);
			}
			sq_pop(vm, 1);
		}
	}
	return sched;
}

void* script_api::param<schedule_t*>::tag()
{
	return (void*)&script_api::param<schedule_t*>::get;
}

void export_schedule(HSQUIRRELVM vm)
{
	/**
	 * Schedule entries
	 */
	begin_class(vm, "schedule_entry_x", "coord3d");

#ifdef SQAPI_DOC // document members
	/**
	 * Minimum loading percentage.
	 */
	integer load;
	/**
	 * Waiting time setting.
	 */
	integer wait;
#endif

	/**
	 * Returns halt at this entry position.
	 * @param pl player that wants to use halt here
	 * @return halt instance
	 * @typemask halt_x(player_x)
	 */
	register_method(vm, &get_halt_from_koord3d, "get_halt", true);
	/**
	 * Returns waiting time formatted as string.
	 * @typemask string()
	 */
	register_function(vm, waiting_time_to_string, "waiting_time_to_string", 1, "x" );

	end_class(vm);

	/**
	 * Class holding the schedule
	 */
	create_class(vm, "schedule_x");

	/**
	 * Constructor
	 * @typemask command_x(way_types)
	 */
	register_function(vm, schedule_constructor, "constructor", 3, "xi.");
	sq_settypetag(vm, -1, param<schedule_t*>::tag());

#ifdef SQAPI_DOC // document members
	/**
	 * The list of schedule targets.
	 */
	array< schedule_entry_x > entries;

	/**
	 * Waytype of schedule.
	 */
	way_types waytype;
#else
	create_slot(vm, "entries", 0);
	create_slot(vm, "waytype", 0);
#endif

	end_class(vm);
}
