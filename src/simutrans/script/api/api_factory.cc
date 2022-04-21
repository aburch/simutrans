/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_factory.cc exports factory related functions. */

#include "get_next.h"
#include "api_obj_desc_base.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/scenario.h"
#include "../../simfab.h"
#include "../../descriptor/factory_desc.h"

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
	coordinate_transform_t::koord_sq2w(pos);
	fabrik_t *fab =  fabrik_t::get_fab(pos);
	if (!fab) {
		return sq_raise_error(vm, "No factory found at (%s)", pos.get_str());
	}
	const factory_desc_t *desc = fab->get_desc();
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
			sint64 factor = io == 0 ? desc->get_supplier(p)->get_consumption() : desc->get_product(p)->get_factor();
			// set max value - see fabrik_t::info_prod
			set_slot(vm, "max_storage", convert_goods( (sint64)prodslot[p].max * factor), -1);
			// production/consumption scaling
			set_slot(vm, "scaling", factor, -1);
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


uint32 get_production_factor(const factory_product_desc_t *desc)
{
	return desc ? ( (1<< (DEFAULT_PRODUCTION_FACTOR_BITS-1)) + (uint32)desc->get_factor() * 100) >> DEFAULT_PRODUCTION_FACTOR_BITS : 0;
}


uint32 get_consumption_factor(const factory_supplier_desc_t *desc)
{
	return desc ? ( (1<< (DEFAULT_PRODUCTION_FACTOR_BITS-1)) + (uint32)desc->get_consumption() * 100) >> DEFAULT_PRODUCTION_FACTOR_BITS : 0;
}


vector_tpl<koord> const& factory_get_tile_list(fabrik_t *fab)
{
	static vector_tpl<koord> list;
	fab->get_tile_list(list);
	return list;
}

vector_tpl<halthandle_t> const& square_get_halt_list(planquadrat_t *plan); // api_tiles.cc

vector_tpl<halthandle_t> const& factory_get_halt_list(fabrik_t *fab)
{
	planquadrat_t *plan = welt->access(fab->get_pos().get_2d());
	return square_get_halt_list(plan);
}

 leitung_t* factory_get_transformer( fabrik_t* fab )
 {
	 return fab->get_transformers().empty() ? NULL :fab->get_transformers().front();
 }

call_tool_init factory_set_name(fabrik_t *fab, const char* name)
{
	return command_rename(welt->get_public_player(), 'f', fab->get_pos(), name);
}


const char* fabrik_get_raw_name(fabrik_t *fab)
{
	return fab->get_desc()->get_name();
}


SQInteger ware_production_get_production(HSQUIRRELVM vm)
{
	fabrik_t* fab = param<fabrik_t*>::get(vm, 1);
	sint64 prod = 0;
	if (fab) {
		sint64 scaling = 0;
		if (SQ_SUCCEEDED(get_slot(vm, "scaling", scaling, 1))) {
			// see fabrik_t::step
			prod = (scaling * welt->scale_with_month_length(fab->get_base_production()) * DEFAULT_PRODUCTION_FACTOR ) >> (8+8);
		}
	}
	return param<sint64>::push(vm, prod);
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

SQInteger world_get_factory_count(HSQUIRRELVM vm)
{
	return param<uint32>::push(vm, welt->get_fab_list().get_count());
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
	/**
	 * Returns number of factories in the list.
	 * @typemask integer()
	 */
	register_function(vm, world_get_factory_count, "get_count",  1, "x");

	end_class(vm);


	/**
	 * Class to access information about factories.
	 * Identified by coordinate.
	 */
	begin_class(vm, "factory_x", "extend_get,coord,ingame_object");

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
	 *     // print raw name of the good
	 *     print("Input slot key: " + key)
	 *     // print current storage
	 *     print("Input slot storage: " + value.get_storage()[0])
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
	 * @returns if object is still valid.
	 */
	export_is_valid<fabrik_t*>(vm); //register_function("is_valid")

	/**
	 * Get list of consumers of this factory.
	 * @returns array of coordinates of consumers
	 */
	register_method(vm, &fabrik_t::get_consumer, "get_consumers");

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
	 * Get untranslated name of factory.
	 * @returns name
	 */
	register_method(vm, &fabrik_get_raw_name, "get_raw_name", true);

	/**
	 * Change name.
	 * @ingroup rename_func
	 */
	register_method(vm, &factory_set_name, "set_name", true);

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

	/**
	 * Get list of all tiles occupied by buildings belonging to this factory.
	 * @returns array of tile_x objects
	 */
	register_method(vm, &factory_get_tile_list, "get_tile_list", true);

	/**
	 * Get list of all halts that serve this this factory.
	 * @returns array of tile_x objects
	 */
	register_method(vm, &factory_get_halt_list, "get_halt_list", true);

	/**
	 * Checks whether a transformer is connected.
	 * @returns name
	 */
	register_method(vm, &fabrik_t::is_transformer_connected,  "is_transformer_connected");
	/**
	 * Get connected transformer (if any).
	 * @returns transformer
	 */
	register_method(vm, factory_get_transformer, "get_transformer", true);
	/**
	 * @returns number of fields belonging to this factory
	 */
	register_method(vm, &fabrik_t::get_field_count, "get_field_count");
	/**
	 * @returns minimum number of fields required
	 */
	register_method(vm, &fabrik_t::get_min_field_count, "get_min_field_count");
	/**
	 * @returns factory descriptor
	 */
	register_method(vm, &fabrik_t::get_desc, "get_desc");
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

	/**
	 * Returns base maximum production of this good per month.
	 * Does not take any productivity boost into account.
	 * @typemask integer()
	 */
	register_function(vm, &ware_production_get_production, "get_base_production", 1, "x");

	/**
	 * Returns base maximum consumption of this good per month.
	 * Does not take any productivity boost into account.
	 * @typemask integer()
	 */
	register_function(vm, &ware_production_get_production, "get_base_consumption", 1, "x");

	/**
	 * Returns number of consumed units of this good for the production of 100 units of generic outputs unit.
	 * Does not take any productivity boost into account.
	 * @typemask integer()
	 */
	register_method(vm, &get_consumption_factor, "get_consumption_factor", 1, "x");

	/**
	 * Returns number of produced units of this good due to the production of 100 units of generic outputs unit.
	 * Does not take any productivity boost into account.
	 * @typemask integer()
	 */
	register_method(vm, &get_production_factor, "get_production_factor", 1, "x");


	// pop class
	end_class(vm);
}
