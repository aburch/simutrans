/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_convoy.cc exports convoy related functions. */

#include "get_next.h"
#include "api_obj_desc_base.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simconvoi.h"
#include "../../simhalt.h"
#include "../../simline.h"
#include "../../world/simworld.h"
#include "../../vehicle/vehicle.h"

// for convoy tools
#include "../../tool/simmenu.h"
#include "../../dataobj/schedule.h"

using namespace script_api;


waytype_t get_convoy_wt(convoi_t* cnv)
{
	if (cnv  &&  cnv->get_vehicle_count() > 0) {
		return cnv->front()->get_waytype();
	}
	return invalid_wt;
}


vector_tpl<sint64> const& get_convoy_stat(convoi_t* cnv, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (cnv  &&  0<=INDEX  &&  INDEX<convoi_t::MAX_CONVOI_COST) {
		for(uint16 i = 0; i < MAX_MONTHS; i++) {
			v.append(cnv->get_stat_converted(i, INDEX));
		}
	}
	return v;
}


vector_tpl<vehicle_desc_t const*> const& convoi_get_vehicles(convoi_t* cnv)
{
	static vector_tpl<vehicle_desc_t const*> v;
	v.clear();
	if (cnv) {
		for(uint16 i=0; i<cnv->get_vehicle_count(); i++) {
			v.append(cnv->get_vehicle(i)->get_desc());
		}
	}
	return v;
}


sint32 convoy_get_kmh(convoi_t const* cnv)
{
	return cnv ? speed_to_kmh(cnv->get_akt_speed()) : -1;
}


vector_tpl<convoihandle_t> const* generic_get_convoy_list(HSQUIRRELVM vm, SQInteger index)
{
	uint16 id;
	bool use_world;
	if (SQ_SUCCEEDED(get_slot(vm, "halt_id", id, index))) {
		halthandle_t halt;
		halt.set_id(id);
		if (halt.is_bound()) {
			return &halt->registered_convoys;
		}
	}
	if (SQ_SUCCEEDED(get_slot(vm, "line_id", id, index))) {
		linehandle_t line;
		line.set_id(id);
		if (line.is_bound()) {
			return &line->get_convoys();
		}
	}
	if (SQ_SUCCEEDED(get_slot(vm, "use_world", use_world, index))  &&  use_world) {
		return &welt->convoys();
	}
	sq_raise_error(vm, "Invalid convoy list.");
	return NULL;
}

SQInteger generic_get_convoy_count(HSQUIRRELVM vm)
{
	vector_tpl<convoihandle_t> const* list = generic_get_convoy_list(vm, 1);
	return list ? param<uint32>::push(vm, list->get_count()) : SQ_ERROR;
}

SQInteger generic_get_next_convoy(HSQUIRRELVM vm)
{
	vector_tpl<convoihandle_t> const* list = generic_get_convoy_list(vm, 1);
	return list ? generic_get_next(vm, list->get_count()) : SQ_ERROR;
}

SQInteger generic_get_convoy_by_index(HSQUIRRELVM vm)
{
	vector_tpl<convoihandle_t> const* list = generic_get_convoy_list(vm, 1);
	sint32 index = param<sint32>::get(vm, 2);
	convoihandle_t cnv;
	if (list) {
		 cnv = (0<=index  &&  (uint32)index < list->get_count()) ?  (*list)[index] : convoihandle_t();
		 return push_instance(vm, "convoy_x",  cnv.is_bound() ? cnv.get_id() :  0);
	}
	return SQ_ERROR;
}

uint32 calc_max_kmh(uint32 power, uint32 weight, sint32 speed_limit)
{
	return speed_to_kmh(convoi_t::calc_max_speed(power, weight, kmh_to_speed(speed_limit)) );
}

uint32 kmh_to_tiles_per_month(uint32 kmh)
{
	return welt->speed_to_tiles_per_month( kmh_to_speed(kmh));
}

call_tool_init convoy_set_name(convoi_t *cnv, const char* name)
{
	return command_rename(cnv->get_owner(), 'c', cnv->self.get_id(), name);
}

call_tool_init convoy_set_line(convoi_t *cnv, player_t *player, linehandle_t line)
{
	if (!line.is_bound()) {
		return "Invalid line handle provided";
	}
	// see depot_frame_t::apply_line() and convoi_t::call_convoi_tool()
	cbuffer_t buf;
	buf.printf("%c,%u,%i", 'l', cnv->self.get_id(), line->get_handle().get_id());

	return call_tool_init(TOOL_CHANGE_CONVOI | SIMPLE_TOOL, buf, 0, player);
}

call_tool_init convoy_generic_tool(convoi_t *cnv, player_t *player, uint8 cnvtool)
{
	char c = (char)cnvtool;
	if (strchr("wx", c) == NULL) {
		return "Invalid convoy tool called";
	}
	// see convoi_t::call_convoi_tool()
	cbuffer_t buf;
	buf.printf("%c,%u", c, cnv->self.get_id());

	return call_tool_init(TOOL_CHANGE_CONVOI | SIMPLE_TOOL, buf, 0, player);
}

bool convoy_is_schedule_editor_open(convoi_t *cnv)
{
	return cnv->get_state() == convoi_t::EDIT_SCHEDULE;
}

bool convoy_is_loading(convoi_t *cnv)
{
	return cnv->get_state() == convoi_t::LOADING;
}

call_tool_init convoy_change_schedule(convoi_t *cnv, player_t *player, schedule_t *sched)
{
	if (sched) {
		cbuffer_t buf;
		// make a copy, and perform validation on it
		schedule_t *copy = sched->copy();
		copy->make_valid();
		if (copy->get_count() >= 2) {
			// build param string (see convoi_info_t::apply_schedule and convoi_t::call_convoi_tool())
			buf.printf("g,%u,", cnv->self.get_id());
			copy->sprintf_schedule(buf);
		}
		else {
			return "Invalid schedule provided: less than two entries remained after removing doubles";
		}
		delete copy;
		return call_tool_init(TOOL_CHANGE_CONVOI | SIMPLE_TOOL, buf, 0, player);
	}
	return "Invalid schedule provided";
}

void export_convoy(HSQUIRRELVM vm)
{
	/**
	 * Implements iterator to iterate through lists of convoys.
	 *
	 * Usage:
	 * @code
	 * local list = world.get_convoy_list_x()
	 * foreach(cnv in list) {
	 *     ... // cnv is an instance of the convoy_x class
	 * }
	 * @endcode
	 *
	 * @see world::get_convoy_list, halt_x::get_convoy_list, line_x::get_convoy_list
	 */
	begin_class(vm, "convoy_list_x", 0);

	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, generic_get_next_convoy,     "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 * @typemask convoy_x()
	 */
	register_function(vm, generic_get_convoy_by_index, "_get",    2, "xi");
	/**
	 * Returns number of convoys in the list.
	 * @typemask integer()
	 */
	register_function(vm, generic_get_convoy_count, "get_count",  1, "x");

	end_class(vm);

	/**
	 * Class to access a convoy.
	 * Player vehicles are convoys, which themselves consist of individual vehicles (trucks, trailers, ...).
	 */
	begin_class(vm, "convoy_x", "extend_get,ingame_object");
	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<convoi_t*>(vm); //register_function("is_valid")
	/**
	 * Does convoy needs electrified ways?
	 * @returns true if this is the case
	 */
	register_method(vm, &convoi_t::needs_electrification, "needs_electrification");
	/**
	 * Name of convoy.
	 * @returns name
	 */
	register_method(vm, &convoi_t::get_name,     "get_name");
	/**
	 * Sets convoy name.
	 * @ingroup rename_func
	 */
	register_method(vm, &convoy_set_name, "set_name", true);
	/**
	 * Position of convoy.
	 * @returns pos
	 */
	register_method(vm, &convoi_t::get_pos,      "get_pos");
	/**
	 * Owner of convoy.
	 * @returns owner, which is instance of player_x
	 */
	register_method(vm, &convoi_t::get_owner, "get_owner");
	/**
	 * Returns array of goods categories that can be transported by this convoy.
	 * @returns array
	 */
	register_method(vm, &convoi_t::get_goods_catg_index, "get_goods_catg_index");
	/**
	 * Returns waytype of convoy.
	 * @returns waytype
	 * @see way_types
	 */
	register_method(vm, &get_convoy_wt, "get_waytype", true);
	/**
	 * Schedule of this convoy.
	 */
	register_method(vm, &convoi_t::get_schedule, "get_schedule");
	/**
	 * Get monthly statistics of capacity.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_convoy_stat, "get_capacity",          freevariable<sint32>(convoi_t::CONVOI_CAPACITY), true);
	/**
	 * Get monthly statistics of number of transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_convoy_stat, "get_transported_goods", freevariable<sint32>(convoi_t::CONVOI_TRANSPORTED_GOODS), true );
	/**
	 * Get monthly statistics of revenue.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_convoy_stat, "get_revenue",           freevariable<sint32>(convoi_t::CONVOI_REVENUE), true );
	/**
	 * Get monthly statistics of running costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_convoy_stat, "get_cost",              freevariable<sint32>(convoi_t::CONVOI_OPERATIONS), true );
	/**
	 * Get monthly statistics of profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_convoy_stat, "get_profit",            freevariable<sint32>(convoi_t::CONVOI_PROFIT), true );
	/**
	 * Get monthly statistics of traveled distance.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_convoy_stat, "get_traveled_distance", freevariable<sint32>(convoi_t::CONVOI_DISTANCE), true );
	/**
	 * Get monthly statistics of income/loss due to way tolls.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_convoy_stat, "get_way_tolls",         freevariable<sint32>(convoi_t::CONVOI_WAYTOLL), true );
	/**
	 * @returns lifetime traveled distance of this convoy
	 */
	register_method(vm, &convoi_t::get_total_distance_traveled, "get_distance_traveled_total");
	/**
	 * @returns the line the convoy belongs to, null if there is no line
	 */
	register_method(vm, &convoi_t::get_line, "get_line");
	/**
	 * Assigns the convoy to the given line.
	 * @param player
	 * @param line
	 * @ingroup game_cmd
	 */
	register_method(vm, convoy_set_line, "set_line", true);
	/**
	 * @returns returns an array containing the vehicle_desc_x 's of the vehicles of this convoy
	 */
	register_method(vm, convoi_get_vehicles, "get_vehicles", true);
	/**
	 * @returns current speed of convoy
	 */
	register_method(vm, convoy_get_kmh, "get_speed", true);
	/**
	 * @returns get current loading limit: waiting for full load gives 100
	 */
	register_method(vm, &convoi_t::get_loading_limit, "get_loading_limit");
	/**
	 * @returns get current loading level
	 */
	register_method(vm, &convoi_t::get_loading_level, "get_loading_level");
	/**
	 * @returns gets location of home depot
	 */
	register_method(vm, &convoi_t::get_home_depot, "get_home_depot");
	/**
	 * @returns whether convoi has obsolete vehicles
	 */
	register_method(vm, &convoi_t::has_obsolete_vehicles, "has_obsolete_vehicles");
	/**
	 * Toggle the flag 'withdraw convoy'
	 * @ingroup game_cmd
	 */
	register_method_fv(vm, convoy_generic_tool, "toggle_withdraw", freevariable<uint8>('w'), true );
	/**
	 * @returns the flag 'withdraw convoy'
	 */
	register_method(vm, &convoi_t::get_withdraw, "is_withdrawn");
	/**
	 * @returns true if convoy is in depot
	 */
	register_method(vm, &convoi_t::in_depot, "is_in_depot");
	/**
	 * @returns true if convoy is currently waiting (for way clearance)
	 */
	register_method(vm, &convoi_t::is_waiting, "is_waiting");
	/**
	 * @returns true if convoy is currently waiting (for way clearance)
	 */
	register_method(vm, convoy_is_loading, "is_loading", true);
	/**
	 * Destroy the convoy.
	 * The convoy will be marked for destroying, it will be destroyed when the simulation continues.
	 * Call @ref sleep to be sure that the convoy is destroyed before script continues.
	 * @ingroup game_cmd
	 */
	register_method_fv(vm, convoy_generic_tool, "destroy", freevariable<uint8>('x'), true);
	/**
	 * @returns returns true if the schedule of the convoy is currently being edited.
	 */
	register_method(vm, convoy_is_schedule_editor_open, "is_schedule_editor_open", true);
	/**
	 * @returns returns the number of station tiles covered by the convoy.
	 */
	register_method(vm, &convoi_t::get_tile_length, "get_tile_length");
	/**
	 * Change schedule of convoy.
	 * Schedule should not contain doubled entries and more than two entries.
	 * This might make the convoy lose its line.
	 * @ingroup game_cmd
	 */
	register_method(vm, convoy_change_schedule, "change_schedule", true);

#define STATIC
	/**
	 * Static method to compute the potential max speed of a convoy
	 * with the given parameters.
	 * @param power       total power of convoy
	 * @param weight      weight of convoy
	 * @param speed_limit speed limit induced by convoy's vehicles
	 * @returns max speed
	 */
	STATIC register_method(vm, &calc_max_kmh, "calc_max_speed");
	/**
	 * Static method to convert speed (from km per hour) to tiles per month.
	 * @param speed
	 * @returns tile per month
	 */
	STATIC register_method(vm, &kmh_to_tiles_per_month, "speed_to_tiles_per_month");

	end_class(vm);
}
