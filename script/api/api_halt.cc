/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_halt.cc exports halt/station related functions. */

#include "get_next.h"

#include "api_obj_desc_base.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simhalt.h"

halthandle_t get_halt_from_koord3d(koord3d pos, const player_t *player ); // api_schedule.cc, interfaces haltestelle_t::get_halt


namespace script_api {

	declare_specialized_param(haltestelle_t::tile_t, "t|x|y", "tile_x");
	declare_specialized_param(haltestelle_t::connection_t, "t|x|y", "halt_x");


	SQInteger param<haltestelle_t::tile_t>::push(HSQUIRRELVM vm, haltestelle_t::tile_t const& v)
	{
		return param<grund_t*>::push(vm, v.grund);
	}

	SQInteger param<haltestelle_t::connection_t>::push(HSQUIRRELVM vm, haltestelle_t::connection_t const& v)
	{
		return param<halthandle_t>::push(vm, v.halt);
	}
};


using namespace script_api;

vector_tpl<sint64> const& get_halt_stat(const haltestelle_t *halt, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (halt  &&  0<=INDEX  &&  INDEX<MAX_HALT_COST) {
		for(uint16 i = 0; i < MAX_MONTHS; i++) {
			v.append( halt->get_finance_history(i, INDEX) );
		}
	}
	return v;
}


SQInteger world_get_next_halt(HSQUIRRELVM vm)
{
	return generic_get_next(vm, haltestelle_t::get_alle_haltestellen().get_count());
}


SQInteger world_get_halt_by_index(HSQUIRRELVM vm)
{
	sint32 index = param<sint32>::get(vm, -1);
	const vector_tpl<halthandle_t>& list = haltestelle_t::get_alle_haltestellen();
	halthandle_t halt = (0<=index  &&  (uint32)index<list.get_count()) ?  list[index] : halthandle_t();
	return param<halthandle_t>::push(vm, halt);
}


SQInteger halt_export_convoy_list(HSQUIRRELVM vm)
{
	halthandle_t halt = param<halthandle_t>::get(vm, 1);
	if (halt.is_bound()) {
		push_instance(vm, "convoy_list_x");
		set_slot(vm, "halt_id", halt.get_id());
		return 1;
	}
	else {
		sq_raise_error(vm, "Invalid halt id %d", halt.get_id());
		return SQ_ERROR;
	}
}


call_tool_init halt_set_name(halthandle_t halt, const char* name)
{
	if (!halt.is_bound()) {
		return "Invalid halt provided";
	}
	return command_rename(halt->get_owner(), 'h', halt.get_id(), name);
}


SQInteger halt_export_line_list(HSQUIRRELVM vm)
{
	halthandle_t halt = param<halthandle_t>::get(vm, 1);
	if (halt.is_bound()) {
		push_instance(vm, "line_list_x");
		set_slot(vm, "halt_id", halt.get_id());
		return 1;
	}
	else {
		sq_raise_error(vm, "Invalid halt id %d", halt.get_id());
		return SQ_ERROR;
	}
}


uint32 halt_get_capacity(const haltestelle_t *halt, const goods_desc_t *desc)
{
	// passenger has index 0, mail 1, everything else >=2
	return halt  &&  desc  ?  halt->get_capacity(min( desc->get_catg_index(), 2 )) : 0;
}

// 0: not connected
// 1: connected
// -1: undecided
sint8 is_halt_connected(const haltestelle_t *a, halthandle_t b, const goods_desc_t *desc)
{
	if (desc == 0  ||  a == NULL  ||  !b.is_bound()) {
		return 0;
	}
	return a->is_connected(b, desc->get_catg_index());
}


// compare two halts to find out if they are the same
SQInteger halt_compare(halthandle_t a, halthandle_t b)
{
	return (SQInteger)a.get_id() - (SQInteger)b.get_id();
}


vector_tpl<haltestelle_t::connection_t> const& halt_get_connections(const haltestelle_t *halt, const goods_desc_t* freight)
{
	static vector_tpl<haltestelle_t::connection_t> dummy;
	dummy.clear();
	return freight ? halt->get_connections(freight->get_catg_index()) : dummy;
}


void export_halt(HSQUIRRELVM vm)
{
	/**
	 * Implements iterator to iterate through the list of all halts on the map.
	 *
	 * Usage:
	 * @code
	 * local list = halt_list_x()
	 * foreach(halt in list) {
	 *     ... // halt is an instance of the halt_x class
	 * }
	 * @endcode
	 */
	create_class(vm, "halt_list_x");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, world_get_next_halt,     "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 * @typemask halt_x()
	 */
	register_function(vm, world_get_halt_by_index, "_get",    2, "xi");
	end_class(vm);

	/**
	 * Class to access halts / stations / bus stops.
	 */
	begin_class(vm, "halt_x", "extend_get,ingame_object");

	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<const haltestelle_t*>(vm); //register_function("is_valid")
	/**
	 * Station name.
	 * @returns name
	 */
	register_method(vm, &haltestelle_t::get_name, "get_name");
	/**
	 * Sets station name.
	 * @ingroup rename_func
	 */
	register_method(vm, &halt_set_name, "set_name", true);

	/**
	 * Station owner.
	 * @returns owner
	 */
	register_method(vm, &haltestelle_t::get_owner, "get_owner");

	/**
	 * compare classes using metamethods
	 * @param halt the other halt
	 * @returns difference in the unique id of the halthandle
	 */
	register_method(vm, &halt_compare, "_cmp", true);

	/**
	 * Quick check if there is connection for certain freight to the other halt.
	 * @param halt the other halt
	 * @param freight_type freight type
	 * @return 0 - not connected, 1 - connected, -1 - undecided (call this method again later)
	 */
	register_method(vm, &is_halt_connected, "is_connected", true);

	/**
	 * Does this station accept this type of good?
	 * @param freight_type freight type
	 * @returns the answer to this question
	 */
	register_method<bool (haltestelle_t::*)(const goods_desc_t*) const>(vm, &haltestelle_t::is_enabled, "accepts_good", false);

	/**
	 * Get monthly statistics of number of arrived goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_arrived", freevariable<sint32>(HALT_ARRIVED), true);
	/**
	 * Get monthly statistics of number of departed goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_departed", freevariable<sint32>(HALT_DEPARTED), true);
	/**
	 * Get monthly statistics of number of waiting goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_waiting", freevariable<sint32>(HALT_WAITING), true);
	/**
	 * Get monthly statistics of number of happy passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_happy", freevariable<sint32>(HALT_HAPPY), true);
	/**
	 * Get monthly statistics of number of unhappy passengers.
	 *
	 * These passengers could not start their journey as station was crowded.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_unhappy", freevariable<sint32>(HALT_UNHAPPY), true);
	/**
	 * Get monthly statistics of number of passengers with no-route.
	 *
	 * These passengers could not start their journey as they find no route to their destination.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_noroute", freevariable<sint32>(HALT_NOROUTE), true);
	/**
	 * Get monthly statistics of number of convoys that stopped at this station.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_convoys", freevariable<sint32>(HALT_CONVOIS_ARRIVED), true);
	/**
	 * Get monthly statistics of number of passengers that could walk to their destination.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_halt_stat, "get_walked", freevariable<sint32>(HALT_WALKED), true);
	/**
	 * Exports list of convoys that stop at this halt.
	 * @typemask convoy_list_x()
	 */
	register_function(vm, &halt_export_convoy_list, "get_convoy_list", 1, param<halthandle_t>::typemask());
	/**
	 * Exports list of lines that serve this halt.
	 * @typemask line_list_x()
	 */
	register_function(vm, &halt_export_line_list, "get_line_list", 1, param<halthandle_t>::typemask());
	/**
	 * Get list of tiles belonging to this station.
	 */
	register_method(vm, &haltestelle_t::get_tiles, "get_tile_list");
	/**
	 * Get list of factories connected to this station.
	 */
	register_method(vm, &haltestelle_t::get_fab_list, "get_factory_list");
	/**
	 * Returns amount of @p freight at this halt that is going to @p target
	 * @param freight freight type
	 * @param target coordinate of target
	 */
	register_method(vm, &haltestelle_t::get_ware_fuer_zielpos, "get_freight_to_dest");
	/**
	 * Returns amount of @p freight at this halt that scheduled to @p stop
	 * @param freight freight type
	 * @param stop next transfer stop
	 */
	register_method(vm, &haltestelle_t::get_ware_fuer_zwischenziel, "get_freight_to_halt");
	/**
	 * Returns capacity of this halt for the given @p freight
	 * @param freight freight type
	 */
	register_method(vm, &halt_get_capacity, "get_capacity", true);
	/**
	 * Returns list of connected halts for the specific @p freight type.
	 * @param freight freight type
	 */
	register_method(vm, &halt_get_connections, "get_connections", true);
	/**
	 * Returns halt at given position.
	 * @param pos coordinate
	 * @param pl player that wants to use halt here
	 * @return halt instance
	 */
	STATIC register_method(vm, &get_halt_from_koord3d, "get_halt", false, true);

	end_class(vm);
}
