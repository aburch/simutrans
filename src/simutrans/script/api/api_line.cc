/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_line.cc exports line related functions. */

#include "get_next.h"

#include "api_obj_desc_base.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simhalt.h"
#include "../../simline.h"
#include "../../world/simworld.h"
#include "../../player/simplay.h"

// for manipulation of lines
#include "../../tool/simmenu.h"
#include "../../dataobj/schedule.h"

// template<> schedule_t* script_api::param<schedule_t*>::get(HSQUIRRELVM, SQInteger);

using namespace script_api;

vector_tpl<sint64> const& get_line_stat(simline_t *line, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (line  &&  0<=INDEX  &&  INDEX<MAX_LINE_COST) {
		for(uint16 i = 0; i < MAX_MONTHS; i++) {
			v.append( line->get_stat_converted(i, INDEX) );
		}
	}
	return v;
}


waytype_t line_way_type(simline_t *line)
{
	if (line) {
		switch (line->get_linetype()) {
			case simline_t::truckline: return road_wt;
			case simline_t::trainline: return track_wt;
			case simline_t::shipline: return water_wt;
			case simline_t::monorailline: return monorail_wt;
			case simline_t::maglevline: return maglev_wt;
			case simline_t::narrowgaugeline: return narrowgauge_wt;
			case simline_t::airline: return air_wt;
			case simline_t::tramline: return tram_wt;
			default: ;
		}
	}
	return invalid_wt;
}

call_tool_init line_change_schedule(simline_t* line, player_t *player, schedule_t *sched)
{
	if (sched) {
		cbuffer_t buf;
		// make a copy, and perform validation on it
		schedule_t *copy = sched->copy();
		copy->make_valid();
		if (copy->get_count() >= 2) {
			// build param string (see line_management_gui_t::apply_schedule)
			buf.printf( "g,%i,", line->get_handle().get_id() );
			copy->sprintf_schedule( buf );
		}
		else {
			return "Invalid schedule provided: less than two entries remained after removing doubles";
		}
		delete copy;
		return call_tool_init(TOOL_CHANGE_LINE | SIMPLE_TOOL, buf, 0, player);
	}
	return "Invalid schedule provided";
}

call_tool_init line_delete(simline_t* line, player_t *player)
{
	if (line->count_convoys() == 0) {
		cbuffer_t buf;
		buf.printf( "d,%i", line->get_handle().get_id() );

		return call_tool_init(TOOL_CHANGE_LINE | SIMPLE_TOOL, buf, 0, player);
	}
	return "Cannot delete lines with associated convoys";
}

SQInteger line_export_convoy_list(HSQUIRRELVM vm)
{
	linehandle_t line = param<linehandle_t>::get(vm, 1);
	if (line.is_bound()) {
		push_instance(vm, "convoy_list_x");
		set_slot(vm, "line_id", line.get_id());
		return 1;
	}
	sq_raise_error(vm, "Invalid line handle provided");
	return SQ_ERROR;
}

call_tool_init line_set_name(simline_t* line, const char* name)
{
	return command_rename(line->get_owner(), 'l', line->get_handle().get_id(), name);
}

vector_tpl<linehandle_t> const* generic_get_line_list(HSQUIRRELVM vm, SQInteger index)
{
	uint16 id;
	if (SQ_SUCCEEDED(get_slot(vm, "halt_id", id, index))) {
		halthandle_t halt;
		halt.set_id(id);
		if (halt.is_bound()) {
			return &halt->registered_lines;
		}
	}
	if (SQ_SUCCEEDED(get_slot(vm, "player_id", id, index))  &&  id < PLAYER_UNOWNED) {
		if (player_t *player = welt->get_player(id)) {
			return &player->simlinemgmt.get_line_list();
		}
	}
	sq_raise_error(vm, "Invalid line list.");
	return NULL;
}

SQInteger generic_get_next_line(HSQUIRRELVM vm)
{
	vector_tpl<linehandle_t> const* list = generic_get_line_list(vm, 1);
	return list ? generic_get_next(vm, list->get_count()) : SQ_ERROR;
}

SQInteger generic_get_line_by_index(HSQUIRRELVM vm)
{
	vector_tpl<linehandle_t> const* list = generic_get_line_list(vm, 1);
	sint32 index = param<sint32>::get(vm, 2);
	linehandle_t line;
	if (list) {
		line = (0<=index  &&  (uint32)index < list->get_count()) ?  (*list)[index] : linehandle_t();
		return push_instance(vm, "line_x",  line.is_bound() ? line.get_id() :  0);
	}
	return SQ_ERROR;
}

SQInteger generic_get_line_count(HSQUIRRELVM vm)
{
	vector_tpl<linehandle_t> const* list = generic_get_line_list(vm, 1);
	return param<uint32>::push(vm, list ? list->get_count() : 0);
}

void export_line(HSQUIRRELVM vm)
{
	/**
	 * Implements iterator to iterate through lists of lines.
	 *
	 * Usage:
	 * @code
	 * local list = player_x(0).get_line_list()
	 * foreach(line in list) {
	 *     ... // line is an instance of the line_x class
	 * }
	 * @endcode
	 *
	 * @see player_x::get_line_list, halt_x::get_line_list
	 */
	begin_class(vm, "line_list_x", 0);
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, generic_get_next_line,     "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 * @typemask line_x()
	 */
	register_function(vm, generic_get_line_by_index, "_get",    2, "xi");
	/**
	 * Returns number of lines in the list.
	 * @typemask integer()
	 */
	register_function(vm, generic_get_line_count, "get_count",  1, "x");
	end_class(vm);

	/**
	 * Class to access lines.
	 */
	begin_class(vm, "line_x", "extend_get,ingame_object");

	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<simline_t*>(vm); //register_function("is_valid")
	/**
	 * Line name.
	 * @returns name
	 */
	register_method(vm, &simline_t::get_name, "get_name");
	/**
	 * Sets line name.
	 * @ingroup rename_func
	 * @typemask void(string)
	 */
	register_method(vm, &line_set_name, "set_name", true);
	/**
	 * Line owner.
	 * @returns owner
	 */
	register_method(vm, &simline_t::get_owner, "get_owner");
	/**
	 * Schedule of this line.
	 */
	register_method(vm, &simline_t::get_schedule, "get_schedule");
	/**
	 * Returns array of goods categories that can be transported by this convoy.
	 * @returns array
	 */
	register_method(vm, &simline_t::get_goods_catg_index, "get_goods_catg_index");
	/**
	 * Get monthly statistics of capacity.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_capacity",          freevariable<sint32>(LINE_CAPACITY), true);
	/**
	 * Get monthly statistics of number of transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_transported_goods", freevariable<sint32>(LINE_TRANSPORTED_GOODS), true );
	/**
	 * Get monthly statistics of number of convoys in this line.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_convoy_count",      freevariable<sint32>(LINE_CONVOIS), true );
	/**
	 * Get monthly statistics of revenue.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_revenue",           freevariable<sint32>(LINE_REVENUE), true );
	/**
	 * Get monthly statistics of running costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_cost",              freevariable<sint32>(LINE_OPERATIONS), true );
	/**
	 * Get monthly statistics of profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_profit",            freevariable<sint32>(LINE_PROFIT), true );
	/**
	 * Get monthly statistics of traveled distance.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_traveled_distance", freevariable<sint32>(LINE_DISTANCE), true );
	/**
	 * Get monthly statistics of income/loss due to way tolls.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_way_tolls",         freevariable<sint32>(LINE_WAYTOLL), true );
	/**
	 * Exports list of convoys belonging to this line.
	 * @typemask convoy_list_x()
	 */
	register_function(vm, &line_export_convoy_list, "get_convoy_list", 1, param<linehandle_t>::typemask());

	/**
	 * @return waytype of the line
	 */
	register_method(vm, &line_way_type, "get_waytype", true);

	/**
	 * Change schedule of line.
	 * Schedule should not contain doubled entries and more than two entries.
	 * @ingroup game_cmd
	 */
	register_method(vm, line_change_schedule, "change_schedule", true);

	/**
	 * Delete line
	 * @ingroup game_cmd
	 */
	register_method(vm, line_delete, "destroy", true);

	end_class(vm);
}
