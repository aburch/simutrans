#include "api.h"

/** @file api_factory.cc exports factory related functions. */

#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/scenario.h"
#include "../../simfab.h"

using namespace script_api;

SQInteger exp_factory_constructor(HSQUIRRELVM vm)
{
	sint16 x = param<sint16>::get(vm, 2);
	sint16 y = param<sint16>::get(vm, 3);
	// set coordinates
	set_slot(vm, "x", x, 1);
	set_slot(vm, "y", y, 1);
	// transform coordinates
	koord pos(x,y);
	welt->get_scenario()->koord_sq2w(pos);
	fabrik_t *fab =  fabrik_t::get_fab(pos);
	if (!fab) {
		sq_raise_error(vm, "No factory found at (%s)", pos.get_str());
		return -1;
	}
	// create input/output tables
	for (int io=0; io<2; io++) {
		sq_pushstring(vm, io==0 ? "input" : "output", -1);
		sq_newtable(vm);
		const array_tpl<ware_production_t> &prodslot = io==0 ? fab->get_input() :fab->get_output();
		for(uint32 p=0; p < prodslot.get_count(); p++) {
			// create slots 'good name' <- {x,y,name}   //'factory_production'
			sq_pushstring(vm, prodslot[p].get_typ()->get_name(), -1);
			// create instance of factory_production_x
			if(!SQ_SUCCEEDED(push_instance(vm, "factory_production_x",
				x, y, prodslot[p].get_typ()->get_name(), p + (io > 0  ?  fab->get_input().get_count() : 0))))
			{
				// create empty table
				sq_newtable(vm);
			}
			// set max value
			set_slot(vm, "max_storage", prodslot[p].max >> fabrik_t::precision_bits, -1);
			// put class into table
			sq_newslot(vm, -3, false);
		}
		sq_set(vm, 1);
	}
	return 0; // dummy return type
}


vector_tpl<sint64> const& get_factory_stat(fabrik_t *fab, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (fab  &&  0<=INDEX  &&  INDEX<MAX_FAB_STAT) {
		for(uint16 i = 0; i < MAX_MONTH; i++) {
			v.append(fab->get_stat_converted(i, INDEX));
		}
	}
	return v;
}


vector_tpl<sint64> const& get_factory_production_stat(const ware_production_t *prod_slot, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (prod_slot  &&  0<=INDEX  &&  INDEX<MAX_FAB_GOODS_STAT) {
		for(uint16 i = 0; i < MAX_MONTH; i++) {
			v.append(prod_slot->get_stat_converted(i, INDEX));
		}
	}
	return v;
}


SQInteger world_get_next_factory(HSQUIRRELVM vm)
{
	return generic_get_next(vm, welt->get_fab_list().get_count());
}


SQInteger world_get_factory_by_index(HSQUIRRELVM vm)
{
	uint32 index = param<uint32>::get(vm, -1);
	fabrik_t *fab = welt->get_fab(index);
	return param<fabrik_t*>::push(vm, fab);
}


void export_factory(HSQUIRRELVM vm)
{
	/**
	 * Implements iterator to iterate through the list of all factories on the map.
	 *
	 * Usage:
	 * @code
	 * local list = factory_list_x()
	 * foreach(factory in list) {
	 *     ... // factory is an instance of the factory_x class
	 * }
	 * @endcode
	 */
	begin_class(vm, "factory_list_x", 0);
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, world_get_next_factory,     "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, world_get_factory_by_index, "_get",    2, "xi");

	end_class(vm);


	/**
	 * Class to access information about factories.
	 * Identified by coordinate.
	 */
	begin_class(vm, "factory_x", "extend_get,coord");

	/**
	 * Constructor.
	 * @param x x-coordinate
	 * @param y y-coordinate
	 * @typemask (integer,integer)
	 */
	register_function(vm, exp_factory_constructor, "constructor", 3, "xii");

#ifdef SQAPI_DOC // document members
	/**
	 * Table containing input/consumption slots.
	 * Entries can be retrieved by raw name of the good (as defined in the associated pak-file).
	 *
	 * Entries are of type factory_production_x.
	 *
	 * To print a list of all available goods names use this example:
	 * @code
	 * foreach(key,value in input) {
	 * 	// print raw name of the good
	 * 	print("Input slot key: " + key)
	 * 	// print current storage
	 * 	print("Input slot storage: " + value.get_storage()[0])
	 * }
	 * @endcode
	 * To catch the output of this example see @ref sec_err.
	 */
	table<factory_production_x> input;
	/**
	 * Table containing output/production slots.
	 * Entries can be retrieved by raw name of the good (as defined in the associated pak-file).
	 *
	 * Entries are of type factory_production_x.
	 *
	 * For an example to retrieve the list of goods, see factory_x::input.
	 */
	table<factory_production_x> output;
#endif

	/**
	 * Get list of consumers of this factory.
	 * @returns array of coordinates of consumers
	 */
	register_method(vm, &fabrik_t::get_lieferziele, "get_consumers");

	/**
	 * Get list of consumers of this factory.
	 * @returns array of coordinates of suppliers
	 */
	register_method(vm, &fabrik_t::get_suppliers, "get_suppliers");

	/**
	 * Get (translated or custom) name of factory.
	 * @returns name
	 */
	register_method(vm, &fabrik_t::get_name, "get_name");

	/**
	 * Change name.
	 */
	register_method(vm, &fabrik_t::set_name, "set_name");

	/**
	 * Get monthly statistics of production.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_production",     freevariable<sint32>(FAB_PRODUCTION), true);

	/**
	 * Get monthly statistics of power consumption/production.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_power",          freevariable<sint32>(FAB_POWER), true);

	/**
	 * Get monthly statistics of production boost due to electric power supply.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_boost_electric", freevariable<sint32>(FAB_BOOST_ELECTRIC), true);

	/**
	 * Get monthly statistics of production boost due to arrived passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_boost_pax",      freevariable<sint32>(FAB_BOOST_PAX), true);

	/**
	 * Get monthly statistics of production boost due to arrived mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_boost_mail",     freevariable<sint32>(FAB_BOOST_MAIL), true);

	/**
	 * Get monthly statistics of generated passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_pax_generated",  freevariable<sint32>(FAB_PAX_GENERATED), true);

	/**
	 * Get monthly statistics of departed passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_pax_departed",   freevariable<sint32>(FAB_PAX_DEPARTED), true);

	/**
	 * Get monthly statistics of arrived passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_pax_arrived",    freevariable<sint32>(FAB_PAX_ARRIVED), true);

	/**
	 * Get monthly statistics of generated mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_mail_generated", freevariable<sint32>(FAB_MAIL_GENERATED), true);

	/**
	 * Get monthly statistics of departed mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_mail_departed",  freevariable<sint32>(FAB_MAIL_DEPARTED), true);

	/**
	 * Get monthly statistics of arrived mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_stat, "get_mail_arrived",   freevariable<sint32>(FAB_MAIL_ARRIVED), true);

	// pop class
	end_class(vm);

	/**
	 * Class to access storage slots of factories.
	 * Are automatically instantiated by factory_x constructor.
	 */
	begin_class(vm, "factory_production_x", "extend_get");

#ifdef SQAPI_DOC // document members
	/**
	 * Maximum storage of this slot.
	 */
	integer max_storage;
#endif
	/**
	 * Get monthly statistics of storage.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_production_stat, "get_storage",   freevariable<sint32>(FAB_GOODS_STORAGE), true);

	/**
	 * Get monthly statistics of received goods (for input slots).
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_production_stat, "get_received",  freevariable<sint32>(FAB_GOODS_RECEIVED), true);

	/**
	 * Get monthly statistics of consumed goods (for input slots).
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_production_stat, "get_consumed",  freevariable<sint32>(FAB_GOODS_CONSUMED), true);

	/**
	 * Get monthly statistics of in-transit goods (for input slots).
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_production_stat, "get_in_transit",freevariable<sint32>(FAB_GOODS_TRANSIT), true);

	/**
	 * Get monthly statistics of delivered goods (for output slots).
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_production_stat, "get_delivered", freevariable<sint32>(FAB_GOODS_DELIVERED), true);

	/**
	 * Get monthly statistics of produced goods (for output slots).
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_factory_production_stat, "get_produced",  freevariable<sint32>(FAB_GOODS_PRODUCED), true);

	// pop class
	end_class(vm);
}
