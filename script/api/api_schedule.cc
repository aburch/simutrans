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
#include "../../simworld.h"

using namespace script_api;


halthandle_t get_halt_from_koord3d(koord3d pos, const player_t *player )
{
	if(  player == NULL  ) {
		return halthandle_t();
	}
	return haltestelle_t::get_stoppable_halt(pos, player);
}

SQInteger schedule_constructor(HSQUIRRELVM vm) // instance, wt, entries
{
	waytype_t wt = param<waytype_t>::get(vm, 2);
	SQInteger res = set_slot(vm, "waytype", wt, 1);
	uint16 wait = 0;

	if (SQ_SUCCEEDED(res)) {
		sq_pushstring(vm, "entries", -1);
		sq_push(vm, 3); // entries
		res = sq_set(vm, 1);

		if (sq_gettop(vm) >= 4) {
			wait = param<uint16>::get(vm, 4);
		}
		set_slot(vm, "base_waiting_time", wait, 1);
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
		sched->set_additional_base_waiting_time(wait);
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

	uint32 stop_flags = 0;
	get_slot(vm, "flags", stop_flags, index);

	uint8 maximum_loading = 100;
	get_slot(vm, "maximum_load", maximum_loading, index);

	uint16 length_coupling_done = 0;
	get_slot(vm, "length_coupling_done", length_coupling_done, index);

	uint16 max_speed = 0;
	get_slot(vm, "max_speed", max_speed, index);

	uint16 balance_speed = 0;
	get_slot(vm, "balance_speed", balance_speed, index);

	uint16 spacing = 1;
	get_slot(vm, "spacing", spacing, index);

	uint16 spacing_shift = 0;
	get_slot(vm, "spacing_shift", spacing_shift, index);

	uint16 delay_tolerance = 0;
	get_slot(vm, "delay_tolerance", delay_tolerance, index);

	grund_t *gr = welt->lookup(pos);
	if (gr) {
		if (sched->append(gr, minimum_loading, waiting_time_shift, stop_flags, max_speed, length_coupling_done, maximum_loading, balance_speed)) {
			sched->at(sched->get_count() - 1).set_spacing(spacing, spacing_shift, delay_tolerance);
		}
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
		linehandle_t const next_line = sched->get_next_line();
		sched->remove_all();
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

		uint16 wait = 0;
		get_slot(vm, "base_waiting_time", wait, index);
		sched->set_additional_base_waiting_time(wait);

		uint8 current = 0;
		get_slot(vm, "current", current, index);
		sched->set_current_stop(current);

		uint8 schedule_flags = 0;
		get_slot(vm, "flags", schedule_flags, index);
		sched->set_flags(schedule_flags);

		uint16 schedule_max_speed = 0;
		get_slot(vm, "max_speed", schedule_max_speed, index);
		sched->set_max_speed(schedule_max_speed);

		linehandle_t dsgid_handle;
		get_slot(vm, "departure_slot_group_id", dsgid_handle, index);
		sched->set_departure_slot_group_id(dsgid_handle);

		sched->set_next_line(next_line);
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

#ifdef SQAPI_DOC //document members
	/**
	 * X-coordinate of entry position
	 */
	integer x;
	/**
	 * Y-coordinate of entry position
	 */
	integer y;
	/**
	 * Z-coordinate of entry position
	 */
	integer z;
#endif

	create_slot(vm, "x", 0);
	create_slot(vm, "y", 0);
	create_slot(vm, "z", 0);

#ifdef SQAPI_DOC // document members
	/**
	 * Minimum loading percentage.
	 */
	integer load;
	/**
	 * Waiting time setting.
	 */
	integer wait;
	/**
	 * Stop flags bitmask (NO_LOAD=4, NO_UNLOAD=8, etc.).
	 */
	integer flags;
#endif

	create_slot(vm, "flags", 0);
	create_slot(vm, "maximum_load", 100);
	create_slot(vm, "length_coupling_done", 0);
	create_slot(vm, "max_speed", 0);
	create_slot(vm, "balance_speed", 0);
	create_slot(vm, "spacing", 1);
	create_slot(vm, "spacing_shift", 0);
	create_slot(vm, "delay_tolerance", 0);

	/**
	 * Returns halt at this entry position.
	 * @param pl player that wants to use halt here
	 * @return halt instance
	 * @typemask halt_x(player_x)
	 */
	register_method(vm, &get_halt_from_koord3d, "get_halt", true);

	end_class(vm);

	/**
	 * Class holding the schedule
	 */
	create_class(vm, "schedule_x");

	/**
	 * Constructor
	 * @typemask command_x(way_types)
	 */
	register_function(vm, schedule_constructor, "constructor", -3, "xi.i");
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
	
	/**
	 * Additional base waiting time for this schedule.
	 */
	integer base_waiting_time;

	/**
	 * Current stop of schedule:
	 * default first stop (schedule of line)
	 * or next stop of convoy (schedule of convoy).
	 *
	 * In order to change this value for a convoy
	 * call @ref convoy_x::change_schedule.
	 */
	integer current;

	/**
	 * Schedule-level flags bitmask (TEMPORARY=1, SAME_DEP_TIME=2, FULL_LOAD_ACCELERATION=4, FULL_LOAD_TIME=8, REVERSE_DEFAULT=16).
	 */
	integer flags;

	/**
	 * Schedule-level operational maximum speed (km/h). 0 means no limit.
	 */
	integer max_speed;

	/**
	 * The leader line of the departure slot group.
	 * A line in its own group has this set to its own line.
	 * null if not assigned to any departure slot group.
	 */
	line_x departure_slot_group_id;
#else
	create_slot(vm, "entries", 0);
	create_slot(vm, "waytype", 0);
	create_slot(vm, "base_waiting_time", 0);
	create_slot(vm, "current", 0);
	create_slot(vm, "flags", 0);
	create_slot(vm, "max_speed", 0);
	create_slot(vm, "departure_slot_group_id", linehandle_t());
#endif

	end_class(vm);
}
