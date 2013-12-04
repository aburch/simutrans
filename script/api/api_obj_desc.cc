#include "api.h"

/** @file api_obj_desc.cc exports goods descriptors - *_besch_t. */

#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "export_besch.h"
#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../besch/haus_besch.h"
#include "../../besch/ware_besch.h"
#include "../../bauer/warenbauer.h"
#include "../../simworld.h"

#define begin_enum(name)
#define end_enum()
#define enum_slot create_slot

using namespace script_api;


SQInteger get_next_ware_besch(HSQUIRRELVM vm)
{
	return generic_get_next(vm, warenbauer_t::get_waren_anzahl());
}


SQInteger get_ware_besch_index(HSQUIRRELVM vm)
{
	uint32 index = param<uint32>::get(vm, -1);

	const char* name = "None"; // fall-back
	if (index < warenbauer_t::get_waren_anzahl()) {
		name = warenbauer_t::get_info(index)->get_name();
	}
	return push_instance(vm, "good_desc_x",  name);
}


bool are_equal(const obj_besch_std_name_t* a, const obj_besch_std_name_t* b)
{
	return (a==b);
}


sint64 get_scaled_maintenance(const obj_besch_transport_related_t* besch)
{
	return besch ? welt->scale_with_month_length(besch->get_maintenance()) : 0;
}


sint64 get_scaled_maintenance_building(const haus_besch_t* besch)
{
	return besch ? welt->scale_with_month_length(besch->get_maintenance(welt)) : 0;
}


bool building_enables(const haus_besch_t* besch, uint8 which)
{
	return besch ? besch->get_enabled() & which : 0;
}


mytime_t get_intro_retire(const obj_besch_timelined_t* besch, bool intro)
{
	return (uint32)(besch ? ( intro ? besch->get_intro_year_month() : besch->get_retire_year_month() ) : 1);
}


bool is_obsolete_future(const obj_besch_timelined_t* besch, mytime_t time, uint8 what)
{
	if (besch) {
		switch(what) {
			case 0: return besch->is_future(time.raw);
			case 1: return besch->is_retired(time.raw);
			case 2: return besch->is_available(time.raw);
			default: ;
		}
	}
	return false;
}


// export of haus_besch_t::utyp only here
namespace script_api {
	declare_specialized_param(haus_besch_t::utyp, "i", "building_desc_x::building_type");

	SQInteger param<haus_besch_t::utyp>::push(HSQUIRRELVM vm, const haus_besch_t::utyp & u)
	{
		return param<uint16>::push(vm, u);
	}
};


void export_goods_desc(HSQUIRRELVM vm)
{
	/**
	 * Base class of all object descriptors.
	 */
	create_class<const obj_besch_std_name_t*>(vm, "obj_desc_x", "extend_get");

	/**
	 * @return raw (untranslated) name
	 * @typemask string()
	 */
	register_method(vm, &obj_besch_std_name_t::get_name, "get_name");
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
	create_class<const obj_besch_timelined_t*>(vm, "obj_desc_time_x", "obj_desc_x");

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
	create_class<const obj_besch_transport_related_t*>(vm, "obj_desc_transport_x", "obj_desc_time_x");
	/**
	 * @returns monthly maintenance cost of one object of this type.
	 */
	register_local_method(vm, &get_scaled_maintenance, "get_maintenance");
	/**
	 * @returns cost to buy or build on piece or tile of this thing.
	 */
	register_method(vm, &obj_besch_transport_related_t::get_preis, "get_cost");
	/**
	 * @returns way type, can be @ref wt_invalid.
	 */
	register_method(vm, &obj_besch_transport_related_t::get_waytype, "get_waytype");
	/**
	 * @returns top speed: maximal possible or allowed speed, in km/h.
	 */
	register_method(vm, &obj_besch_transport_related_t::get_topspeed, "get_topspeed");

	end_class(vm);

	/**
	 * Object descriptors for trees.
	 */
	begin_besch_class(vm, "tree_desc_x", "obj_desc_x", (GETBESCHFUNC)param<const baum_besch_t*>::getfunc());
	end_class(vm);

	/**
	 * Object descriptors for buildings: houses, attractions, stations and extensions, depots, harbours.
	 */
	begin_besch_class(vm, "building_desc_x", "obj_desc_time_x", (GETBESCHFUNC)param<const haus_besch_t*>::getfunc());

	/**
	 * @returns whether building is an attraction
	 */
	register_method(vm, &haus_besch_t::ist_ausflugsziel, "is_attraction");
	/**
	 * @param rotation
	 * @return size of building in the given @p rotation
	 */
	register_method(vm, &haus_besch_t::get_groesse, "get_size");
	/**
	 * @return monthly maintenance cost
	 */
	register_method(vm, &get_scaled_maintenance_building, "get_maintenance", true);
	/**
	 * @return price to build this building
	 */
	register_method_fv(vm, &haus_besch_t::get_price, "get_cost", freevariable<karte_t*>(welt));
	/**
	 * @return capacity
	 */
	register_method(vm, &haus_besch_t::get_capacity, "get_capacity");
	/**
	 * @return whether building can be built underground
	 */
	register_method(vm, &haus_besch_t::can_be_built_underground, "can_be_built_underground");
	/**
	 * @return whether building can be built above ground
	 */
	register_method(vm, &haus_besch_t::can_be_built_aboveground, "can_be_built_aboveground");
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
	enum_slot(vm, "attraction_city", (uint8)haus_besch_t::attraction_city, true);
	/// tourist attraction to be built outside cities
	enum_slot(vm, "attraction_land", (uint8)haus_besch_t::attraction_land, true);
	/// monument, built only once per map
	enum_slot(vm, "monument", (uint8)haus_besch_t::denkmal, true);
	/// factory
	enum_slot(vm, "factory", (uint8)haus_besch_t::fabrik, true);
	/// townhall
	enum_slot(vm, "townhall", (uint8)haus_besch_t::rathaus, true);
	/// company headquarter
	enum_slot(vm, "headquarter", (uint8)haus_besch_t::firmensitz, true);
	/// harbour
	enum_slot(vm, "harbour", (uint8)haus_besch_t::hafen, true);
	/// depot
	enum_slot(vm, "depot", (uint8)haus_besch_t::depot, true);
	/// station
	enum_slot(vm, "station", (uint8)haus_besch_t::generic_stop, true);
	/// station extension
	enum_slot(vm, "station_extension", (uint8)haus_besch_t::generic_extension, true);
	end_enum();
	/**
	 * @returns building type
	 */
	register_method(vm, &haus_besch_t::get_utyp, "get_type");

	/**
	 * @returns way type, can be @ref wt_invalid.
	 */
	register_method(vm, &haus_besch_t::get_finance_waytype, "get_waytype");
	end_class(vm);

	/**
	 * Object descriptors for ways.
	 */
	begin_besch_class(vm, "way_desc_x", "obj_desc_transport_x", (GETBESCHFUNC)param<const weg_besch_t*>::getfunc());
	/**
	 * @returns true if this way can be build on the steeper (double) slopes.
	 */
	register_method(vm, &weg_besch_t::has_double_slopes, "has_double_slopes");
	/**
	 * @returns system type of the way, see @ref way_system_types.
	 */
	register_method(vm, &weg_besch_t::get_styp, "get_system_type");

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
	register_function(vm, get_next_ware_besch,  "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, get_ware_besch_index, "_get",    2, "xi");

	end_class(vm);

	/**
	 * Descriptor of goods and freight types.
	 */
	begin_besch_class(vm, "good_desc_x", "obj_desc_x", (GETBESCHFUNC)param<const ware_besch_t*>::getfunc());

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
	register_method(vm, &ware_besch_t::get_catg_index, "get_catg_index");


	end_class(vm);
}
