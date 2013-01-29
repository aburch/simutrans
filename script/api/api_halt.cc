#include "api.h"

/** @file api_halt.cc exports halt/station related functions. */

#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simhalt.h"

using namespace script_api;

vector_tpl<sint64> const& get_halt_stat(halthandle_t halt, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (halt.is_bound()  &&  0<=INDEX  &&  INDEX<MAX_HALT_COST) {
		for(uint16 i = 0; i < MAX_MONTHS; i++) {
			v.append( halt->get_finance_history(i, INDEX) );
		}
	}
	return v;
}


#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);

void export_halt(HSQUIRRELVM vm)
{
	/**
	 * Class to access halts.
	 */
	begin_class("halt_x", "extend_get");

	/**
	 * Station name.
	 * @returns name
	 */
	register_method(vm, &haltestelle_t::get_name, "get_name");

	/**
	 * Station owner.
	 * @returns owner
	 */
	register_method(vm, &haltestelle_t::get_besitzer, "get_owner");

	/**
	 * Does this station accept this type of good?
	 * @returns the answer to this question
	 */
	register_method<bool (haltestelle_t::*)(const ware_besch_t*) const>(vm, &haltestelle_t::is_enabled, "accepts_good", false);

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

	end_class();
}
