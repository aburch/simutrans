#include "api.h"

/** @file api_convoy.cc exports convoy related functions. */

#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simconvoi.h"
#include "../../simhalt.h"
#include "../../simline.h"
#include "../../simworld.h"
#include "../../vehicle/simvehikel.h"

using namespace script_api;


waytype_t get_convoy_wt(convoi_t* cnv)
{
	if (cnv  &&  cnv->get_vehikel_anzahl() > 0) {
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
	end_class(vm);

	/**
	 * Class to access a convoy.
	 * Player vehicles are convoys, which themselves consist of individual vehicles (trucks, trailers, ...).
	 */
	begin_class(vm, "convoy_x", "extend_get");
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
	 * Position of convoy.
	 * @returns pos
	 */
	register_method(vm, &convoi_t::get_pos,      "get_pos");
	/**
	 * Owner of convoy.
	 * @returns owner, which is instance of player_x
	 */
	register_method(vm, &convoi_t::get_besitzer, "get_owner");
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

	end_class(vm);
}
