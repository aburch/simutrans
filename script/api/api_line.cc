#include "api.h"

/** @file api_line.cc exports line related functions. */

#include "get_next.h"

#include "api_obj_desc_base.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simhalt.h"
#include "../../simline.h"
#include "../../simworld.h"
#include "../../player/simplay.h"

using namespace script_api;

vector_tpl<sint64> const& get_line_stat(linehandle_t line, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (line.is_bound()  &&  0<=INDEX  &&  INDEX<MAX_LINE_COST) {
		for(uint16 i = 0; i < MAX_MONTHS; i++) {
			v.append( line->get_finance_history(i, (line_cost_t)INDEX) );
		}
	}
	return v;
}


SQInteger line_export_convoy_list(HSQUIRRELVM vm)
{
	linehandle_t line = param<linehandle_t>::get(vm, 1);
	if (line.is_bound()) {
		push_instance(vm, "convoy_list_x");
		set_slot(vm, "line_id", line.get_id());
		return 1;
	}
	return SQ_ERROR;
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
		if (player_t *sp = welt->get_player(id)) {
			return &sp->simlinemgmt.get_line_list();
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
	end_class(vm);

	/**
	 * Class to access lines.
	 */
	begin_class(vm, "line_x", "extend_get");

	/**
	 * Line name.
	 * @returns name
	 */
	register_method(vm, &simline_t::get_name, "get_name");
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
	// LINE_WAYTOLL not in Extended. Possibly unsafe to just comment this out - ACarlotti
	//register_method_fv(vm, &get_line_stat, "get_way_tolls",         freevariable<sint32>(LINE_WAYTOLL), true );
	/**
	 * Exports list of convoys belonging to this line.
	 * @typemask convoy_list_x()
	 */
	register_function(vm, &line_export_convoy_list, "get_convoy_list", 1, param<linehandle_t>::typemask());

	end_class(vm);
}
