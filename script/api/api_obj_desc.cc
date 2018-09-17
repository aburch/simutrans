#include "api.h"

/** @file api_obj_desc.cc exports goods descriptors - *_desc_t. */

#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "export_desc.h"
#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../descriptor/building_desc.h"
#include "../../descriptor/goods_desc.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/goods_manager.h"
#include "../../simworld.h"

#define begin_enum(name)
#define end_enum()
#define enum_slot create_slot
#define STATIC

using namespace script_api;


SQInteger get_next_ware_desc(HSQUIRRELVM vm)
{
	return generic_get_next(vm, goods_manager_t::get_count());
}


SQInteger get_ware_desc_index(HSQUIRRELVM vm)
{
	uint32 index = param<uint32>::get(vm, -1);

	const char* name = "None"; // fall-back
	if (index < goods_manager_t::get_count()) {
		name = goods_manager_t::get_info(index)->get_name();
	}
	return push_instance(vm, "good_desc_x",  name);
}


bool are_equal(const obj_named_desc_t* a, const obj_named_desc_t* b)
{
	return (a==b);
}


sint64 get_scaled_maintenance(const obj_desc_transport_related_t* desc)
{
	return desc ? welt->calc_adjusted_monthly_figure(desc->get_maintenance()) : 0;
}


sint64 get_scaled_maintenance_building(const building_desc_t* desc)
{
	return desc ? welt->calc_adjusted_monthly_figure(desc->get_maintenance()) : 0;
}


bool building_enables(const building_desc_t* desc, uint16 which)
{
	return desc ? desc->get_enabled() & which : 0;
}


mytime_t get_intro_retire(const obj_desc_timelined_t* desc, bool intro)
{
	return (uint32)(desc ? ( intro ? desc->get_intro_year_month() : desc->get_retire_year_month() ) : 1);
}


bool is_obsolete_future(const obj_desc_timelined_t* desc, mytime_t time, uint8 what)
{
	if (desc) {
		switch(what) {
			case 0: return desc->is_future(time.raw);
			case 1: return desc->is_retired(time.raw);
			case 2: return desc->is_available(time.raw);
			default: ;
		}
	}
	return false;
}


const vector_tpl<const building_desc_t*>& get_building_list(building_desc_t::btype type)
{
	const vector_tpl<const building_desc_t*>* p = hausbauer_t::get_list(type);

	static const vector_tpl<const building_desc_t*> dummy;

	return p ? *p : dummy;
}


// export of building_desc_t::btype only here
namespace script_api {
	declare_specialized_param(building_desc_t::btype, "i", "building_desc_x::building_type");

	SQInteger param<building_desc_t::btype>::push(HSQUIRRELVM vm, const building_desc_t::btype & u)
	{
		return param<uint16>::push(vm, u);
	}

	building_desc_t::btype param<building_desc_t::btype>::get(HSQUIRRELVM vm, SQInteger index)
	{
		return (building_desc_t::btype)param<uint16>::get(vm, index);
	}
};


void export_goods_desc(HSQUIRRELVM vm)
{
	/**
	 * Base class of all object descriptors.
	 */
	create_class<const obj_named_desc_t*>(vm, "obj_desc_x", "extend_get");

	/**
	 * @return raw (untranslated) name
	 * @typemask string()
	 */
	register_method(vm, &obj_named_desc_t::get_name, "get_name");
	/**
	 * Checks if two object descriptor are equal.
	 * @param other
	 * @return true if this==other
	 */
	register_method(vm, &are_equal, "is_equal", true);
	end_class(vm);

	/**
	 * Base class of object descriptors with intro / retire dates.
	 */
	create_class<const obj_desc_timelined_t*>(vm, "obj_desc_time_x", "obj_desc_x");

	/**
	 * @return introduction date of this object
	 */
	register_method_fv(vm, &get_intro_retire, "get_intro_date", freevariable<bool>(true), true);
	/**
	 * @return retirement date of this object
	 */
	register_method_fv(vm, &get_intro_retire, "get_retire_date", freevariable<bool>(false), true);
	/**
	 * @param time to test (0 means no timeline game)
	 * @return true if not available as intro date is in future
	 */
	register_method_fv(vm, &is_obsolete_future, "is_future", freevariable<uint8>(0), true);
	/**
	 * @param time to test (0 means no timeline game)
	 * @return true if not available as retirement date already passed
	 */
	register_method_fv(vm, &is_obsolete_future, "is_retired", freevariable<uint8>(1), true);
	/**
	 * @param time to test (0 means no timeline game)
	 * @return true if available: introduction and retirement date checked
	 */
	register_method_fv(vm, &is_obsolete_future, "is_available", freevariable<uint8>(2), true);
	end_class(vm);

	/**
	 * Base class of object descriptors for transport related stuff.
	 */
	create_class<const obj_desc_transport_related_t*>(vm, "obj_desc_transport_x", "obj_desc_time_x");
	/**
	 * @returns monthly maintenance cost of one object of this type.
	 */
	register_local_method(vm, &get_scaled_maintenance, "get_maintenance");
	/**
	 * @returns cost to buy or build on piece or tile of this thing.
	 */
	register_method(vm, &obj_desc_transport_related_t::get_value, "get_cost");
	/**
	 * @returns way type, can be @ref wt_invalid.
	 */
	register_method(vm, &obj_desc_transport_related_t::get_waytype, "get_waytype");
	/**
	 * @returns top speed: maximal possible or allowed speed, in km/h.
	 */
	register_method(vm, &obj_desc_transport_related_t::get_topspeed, "get_topspeed");
	register_method(vm, &obj_desc_transport_related_t::get_topspeed_gradient_1, "get_topspeed_gradient_1");
	register_method(vm, &obj_desc_transport_related_t::get_topspeed_gradient_2, "get_topspeed_gradient_2");

	end_class(vm);

	/**
	 * Object descriptors for trees.
	 */
	begin_desc_class(vm, "tree_desc_x", "obj_desc_x", (GETBESCHFUNC)param<const tree_desc_t*>::getfunc());
	end_class(vm);

	/**
	 * Object descriptors for buildings: houses, attractions, stations and extensions, depots, harbours.
	 */
	begin_desc_class(vm, "building_desc_x", "obj_desc_time_x", (GETBESCHFUNC)param<const building_desc_t*>::getfunc());

	/**
	 * @returns whether building is an attraction
	 */
	register_method(vm, &building_desc_t::is_attraction, "is_attraction");
	/**
	 * @param rotation
	 * @return size of building in the given @p rotation
	 */
	register_method(vm, &building_desc_t::get_size, "get_size");
	/**
	 * @return monthly maintenance cost
	 */
	register_method(vm, &get_scaled_maintenance_building, "get_maintenance", true);
	/**
	 * @return price to build this building
	 */
	register_method_fv(vm, &building_desc_t::get_price, "get_cost", freevariable<karte_t*>(welt));
	/**
	 * @return capacity
	 */
	register_method(vm, &building_desc_t::get_capacity, "get_capacity");
	/**
	 * @return whether building can be built underground
	 */
	register_method(vm, &building_desc_t::can_be_built_underground, "can_be_built_underground");
	/**
	 * @return whether building can be built above ground
	 */
	register_method(vm, &building_desc_t::can_be_built_aboveground, "can_be_built_aboveground");
	/**
	 * @return whether this station building can handle passengers
	 */
	register_method_fv(vm, &building_enables, "enables_pax", freevariable<uint8>(1), true);
	/**
	 * @return whether this station building can handle mail
	 */
	register_method_fv(vm, &building_enables, "enables_mail", freevariable<uint8>(2), true);
	/**
	 * @return whether this station building can handle freight
	 */
	register_method_fv(vm, &building_enables, "enables_freight", freevariable<uint8>(4), true);
	/// building types
	begin_enum("building_type")
	/// tourist attraction to be built in cities
	enum_slot(vm, "attraction_city", (uint8)building_desc_t::attraction_city, true);
	/// tourist attraction to be built outside cities
	enum_slot(vm, "attraction_land", (uint8)building_desc_t::attraction_land, true);
	/// monument, built only once per map
	enum_slot(vm, "monument", (uint8)building_desc_t::monument, true);
	/// factory
	enum_slot(vm, "factory", (uint8)building_desc_t::factory, true);
	/// townhall
	enum_slot(vm, "townhall", (uint8)building_desc_t::townhall, true);
	/// company headquarters
	enum_slot(vm, "headquarters", (uint8)building_desc_t::headquarters, true);
	/// harbour
	enum_slot(vm, "harbour", (uint8)building_desc_t::dock, true);
	/// harbour without a slope (buildable on flat ground beaches)
	enum_slot(vm, "flat_harbour", (uint8)building_desc_t::flat_dock, true);
	/// depot
	enum_slot(vm, "depot", (uint8)building_desc_t::depot, true);
	/// signalbox
	enum_slot(vm, "signalbox", (uint8)building_desc_t::signalbox, true);
	/// station
	enum_slot(vm, "station", (uint8)building_desc_t::generic_stop, true);
	/// station extension
	enum_slot(vm, "station_extension", (uint8)building_desc_t::generic_extension, true);
	end_enum();
	/**
	 * @returns building type
	 */
	register_method(vm, &building_desc_t::get_type, "get_type");

	/**
	 * @returns way type, can be @ref wt_invalid.
	 */
	register_method(vm, &building_desc_t::get_finance_waytype, "get_waytype");

	/**
	 * @returns headquarter level (or -1 if building is not headquarter)
	 */
	register_method(vm, &building_desc_t::get_headquarter_level, "get_headquarter_level");

	/**
	 * Returns an array with all buildings of the given type.
	 * @warning If @p type is one of building_desc_x::harbour, building_desc_x::depot, building_desc_x::station, building_desc_x::station_extension then always the same list is generated.
	 *          You have to filter out e.g. station buildings yourself.
	 */
	STATIC register_method(vm, &get_building_list, "get_building_list");

	end_class(vm);

	/**
	 * Object descriptors for ways.
	 */
	begin_desc_class(vm, "way_desc_x", "obj_desc_transport_x", (GETBESCHFUNC)param<const way_desc_t*>::getfunc());
	/**
	 * @returns true if this way can be build on the steeper (double) slopes.
	 */
	register_method(vm, &way_desc_t::has_double_slopes, "has_double_slopes");
	/**
	 * @returns system type of the way, see @ref way_system_types.
	 */
	register_method(vm, &way_desc_t::get_styp, "get_system_type");

	end_class(vm);

	/**
	 * Implements iterator to iterate through the list of all good types.
	 *
	 * Usage:
	 * @code
	 * local list = good_desc_list_x()
	 * foreach(good_desc in list) {
	 *     ... // good_desc is an instance of the good_desc_x class
	 * }
	 * @endcode
	 */
	create_class(vm, "good_desc_list_x", 0);
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, get_next_ware_desc,  "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, get_ware_desc_index, "_get",    2, "xi");

	end_class(vm);

	/**
	 * Descriptor of goods and freight types.
	 */
	begin_desc_class(vm, "good_desc_x", "obj_desc_x", (GETBESCHFUNC)param<const goods_desc_t*>::getfunc());

	// dummy entry to create documentation of constructor
	/**
	 * Constructor.
	 * @param name raw name of the freight type.
	 * @typemask (string)
	 */
	// register_function( .., "constructor", .. )

	/**
	 * @return freight category. 0=Passengers, 1=Mail, 2=None, >=3 anything else
	 */
	register_method(vm, &goods_desc_t::get_catg_index, "get_catg_index");


	end_class(vm);
}
