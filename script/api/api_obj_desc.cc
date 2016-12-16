#include "api.h"

/** @file api_obj_desc.cc exports goods descriptors - *_besch_t. */

#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "export_besch.h"
#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../besch/bruecke_besch.h"
#include "../../besch/haus_besch.h"
#include "../../besch/vehikel_besch.h"
#include "../../besch/ware_besch.h"
#include "../../bauer/brueckenbauer.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/tunnelbauer.h"
#include "../../bauer/vehikelbauer.h"
#include "../../bauer/warenbauer.h"
#include "../../bauer/wegbauer.h"
#include "../../simhalt.h"
#include "../../simware.h"
#include "../../simworld.h"

#define begin_enum(name)
#define end_enum()
#define enum_slot create_slot


using namespace script_api;


SQInteger get_next_ware_besch(HSQUIRRELVM vm)
{
	return generic_get_next(vm, warenbauer_t::get_count());
}


SQInteger get_ware_besch_index(HSQUIRRELVM vm)
{
	uint32 index = param<uint32>::get(vm, -1);

	const char* name = "None"; // fall-back
	if (index < warenbauer_t::get_count()) {
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

sint64 get_scaled_maintenance_vehicle(const vehikel_besch_t* besch)
{
	return besch ? welt->scale_with_month_length(besch->vehikel_besch_t::get_maintenance()) : 0;
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


const vector_tpl<const haus_besch_t*>& get_building_list(haus_besch_t::utyp type)
{
	const vector_tpl<const haus_besch_t*>* p = hausbauer_t::get_list(type);

	static const vector_tpl<const haus_besch_t*> dummy;

	return p ? *p : dummy;
}


const vector_tpl<const haus_besch_t*>& get_available_stations(haus_besch_t::utyp type, waytype_t wt, const ware_besch_t *freight)
{
	static vector_tpl<const haus_besch_t*> dummy;
	dummy.clear();

	if (freight == NULL  ||  (type != haus_besch_t::depot  &&  type != haus_besch_t::generic_stop  &&  type != haus_besch_t::generic_extension) ) {
		return dummy;
	}

	const vector_tpl<const haus_besch_t*>* p = hausbauer_t::get_list(type);

	// translate freight to enables-flags
	uint8 enables = haltestelle_t::WARE;
	switch(freight->get_catg_index()) {
		case warenbauer_t::INDEX_PAS:  enables = haltestelle_t::PAX;  break;
		case warenbauer_t::INDEX_MAIL: enables = haltestelle_t::POST; break;
		default: ;
	}
	if (type == haus_besch_t::depot) {
		enables = 0;
	}

	uint16 time = welt->get_timeline_year_month();
	FOR(vector_tpl<haus_besch_t const*>, const besch, *p) {
		if(  besch->get_utyp()==type  &&  besch->get_extra()==(uint32)wt  &&  (enables==0  ||  (besch->get_enabled()&enables)!=0)  &&  besch->is_available(time)) {
			dummy.append(besch);
		}
	}
	return dummy;
}

sint64 building_get_cost(const haus_besch_t* besch)
{
	return besch->get_price(welt) * besch->get_b() * besch->get_h();
}

bool building_is_terminus(const haus_besch_t *besch)
{
	return besch  &&  besch->get_utyp() == haus_besch_t::generic_stop  &&  besch->get_all_layouts() == 4;
}

bool can_be_first(const vehikel_besch_t *besch)
{
	return besch->can_follow(NULL);
}

bool can_be_last(const vehikel_besch_t *besch)
{
	return besch->can_lead(NULL);
}

bool is_coupling_allowed(const vehikel_besch_t *besch1, const vehikel_besch_t *besch2)
{
	return besch1->can_lead(besch2)  &&  besch2->can_follow(besch1);
}

const vector_tpl<const vehikel_besch_t*>& get_predecessors(const vehikel_besch_t *besch)
{
	static vector_tpl<const vehikel_besch_t*> dummy;
	dummy.clear();
	for(int i=0; i<besch->get_vorgaenger_count(); i++) {
		if (besch->get_vorgaenger(i)) {
			dummy.append(besch->get_vorgaenger(i));
		}
	}
	return dummy;
}

const vector_tpl<const vehikel_besch_t*>& get_successors(const vehikel_besch_t *besch)
{
	static vector_tpl<const vehikel_besch_t*> dummy;
	dummy.clear();
	for(int i=0; i<besch->get_nachfolger_count(); i++) {
		if (besch->get_nachfolger(i)) {
			dummy.append(besch->get_nachfolger(i));
		}
	}
	return dummy;
}

const vector_tpl<const vehikel_besch_t*>& get_available_vehicles(waytype_t wt)
{
	static vector_tpl<const vehikel_besch_t*> dummy;

	bool use_obsolete = welt->get_settings().get_allow_buying_obsolete_vehicles();
	uint16 time = welt->get_timeline_year_month();

	dummy.clear();
	slist_tpl<vehikel_besch_t const*> const& list = vehikelbauer_t::get_info(wt);

	FOR(slist_tpl<vehikel_besch_t const*> const, i, list) {
		if (!i->is_retired(time)  ||  use_obsolete) {
			if (!i->is_future(time)) {
				dummy.append(i);
			}
		}
	}
	return dummy;
}

uint32 get_power(const vehikel_besch_t *besch)
{
	return besch->get_leistung() * besch->get_gear();
}

// export of haus_besch_t::utyp only here
namespace script_api {
	declare_specialized_param(haus_besch_t::utyp, "i", "building_desc_x::building_type");

	SQInteger param<haus_besch_t::utyp>::push(HSQUIRRELVM vm, const haus_besch_t::utyp & u)
	{
		return param<uint16>::push(vm, u);
	}

	haus_besch_t::utyp param<haus_besch_t::utyp>::get(HSQUIRRELVM vm, SQInteger index)
	{
		return (haus_besch_t::utyp)param<uint16>::get(vm, index);
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
	 * @returns monthly maintenance cost [in 1/100 credits] of one object of this type.
	 */
	register_local_method(vm, &get_scaled_maintenance, "get_maintenance");
	/**
	 * @returns cost [in 1/100 credits] to buy or build on piece or tile of this thing.
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
	 * Vehicle descriptors
	 */
	begin_besch_class(vm, "vehicle_desc_x", "obj_desc_transport_x", (GETBESCHFUNC)param<const vehikel_besch_t*>::getfunc());
	/**
	 * @returns true if this vehicle can lead a convoy
	 */
	register_method(vm, &can_be_first, "can_be_first", true);
	/**
	 * @returns true if this vehicle can be the last of a convoy
	 */
	register_method(vm, &can_be_last, "can_be_last", true);
	/**
	 * @returns list of possible successors, if all are allowed then list is empty
	 */
	register_method(vm, &get_successors, "get_successors", true);
	/**
	 * @returns list of possible predecessors, if all are allowed then list is empty
	 */
	register_method(vm, &get_predecessors, "get_predecessors", true);
	/**
	 * @returns a list of all available vehicles at the current in-game-time
	 */
	STATIC register_method(vm, &get_available_vehicles, "get_available_vehicles", false, true);
	/**
	 * @returns the power of the vehicle (takes power and gear from pak-files into account)
	 */
	register_method(vm, &get_power, "get_power", true);
	/**
	 * @returns freight that can be transported (or null)
	 */
	register_method(vm, &vehikel_besch_t::get_ware, "get_freight");
	/**
	 * @returns capacity
	 */
	register_method(vm, &vehikel_besch_t::get_zuladung, "get_capacity");
	/**
	 * @returns running cost in 1/100 credits per tile
	 */
	register_method(vm, &vehikel_besch_t::get_betriebskosten, "get_running_cost");
	/**
	 * @returns fixed cost in 1/100 credits per month
	 */
	register_method(vm, &get_scaled_maintenance_vehicle, "get_maintenance", true);
	/**
	 * @returns weight of the empty vehicle
	 */
	register_method(vm, &vehikel_besch_t::get_gewicht, "get_weight"); // in kg
	/**
	 * @returns lengths in @ref units::CARUNITS_PER_TILE
	 */
	register_method(vm, &vehikel_besch_t::get_length, "get_length"); // in CAR_UNITS_PER_TILE
	/**
	 * Checks if the coupling of @p first and @p second is possible in this order.
	 * @param first
	 * @param second
	 * @returns true if coupling is possible
	 */
	STATIC register_method(vm, is_coupling_allowed, "is_coupling_allowed", false, true);
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
	register_method(vm, &haus_besch_t::get_size, "get_size");
	/**
	 * @return monthly maintenance cost
	 */
	register_method(vm, &get_scaled_maintenance_building, "get_maintenance", true);
	/**
	 * Price to build this building, takes size, level, and type into account.
	 * @return price [in 1/100 credits] to build this building
	 */
	register_method(vm, &building_get_cost, "get_cost", true);
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
	enum_slot(vm, "harbour", (uint8)haus_besch_t::dock, true);
	/// harbour without a slope (buildable on flat ground beaches)
	enum_slot(vm, "flat_harbour", (uint8)haus_besch_t::flat_dock, true);
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

	/**
	 * @returns headquarter level (or -1 if building is not headquarter)
	 */
	register_method(vm, &haus_besch_t::get_headquarter_level, "get_headquarter_level");

	/**
	 * Returns an array with all buildings of the given type.
	 * @warning If @p type is one of building_desc_x::harbour, building_desc_x::depot, building_desc_x::station, building_desc_x::station_extension then always the same list is generated.
	 *          You have to filter out e.g. station buildings yourself.
	 */
	STATIC register_method(vm, &get_building_list, "get_building_list", false, true);
	/**
	 * Returns an array of available station/extension/depot buildings.
	 * Entries are of type @ref building_desc_x.
	 * @param type building type from @ref building_desc_x::building_type
	 * @param wt waytype
	 * @param freight station should accept this freight
	 * @returns the list
	 */
	STATIC register_method(vm, &get_available_stations, "get_available_stations", false, true);

	/**
	 * @returns true if this is a station building that can be used as terminus
	 */
	register_method(vm, &building_is_terminus, "is_terminus", true);

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

	/**
	 * Generates a list of all ways available for building.
	 * @param wt waytype
	 * @param st system type of way
	 * @returns the list
	 */
	STATIC register_method(vm, wegbauer_t::get_way_list, "get_available_ways", false, true);

	end_class(vm);

	/**
	 * Object descriptors for tunnels.
	 */
	begin_besch_class(vm, "tunnel_desc_x", "obj_desc_transport_x", (GETBESCHFUNC)param<const tunnel_besch_t*>::getfunc());
	/**
	 * Returns a list with available tunnel types.
	 */
	STATIC register_method(vm, tunnelbauer_t::get_available_tunnels, "get_available_tunnels", false, true);
	end_class(vm);

	/**
	 * Object descriptors for bridges.
	 */
	begin_besch_class(vm, "bridge_desc_x", "obj_desc_transport_x", (GETBESCHFUNC)param<const bruecke_besch_t*>::getfunc());
	/**
	 * @return true if this bridge can raise two level from flat terrain
	 */
	register_method(vm, &bruecke_besch_t::has_double_ramp, "has_double_ramp");
	/**
	 * @return true if this bridge can start or end on a double slope
	 */
	register_method(vm, &bruecke_besch_t::has_double_start, "has_double_start");
	/**
	 * @return maximal bridge length in tiles
	 */
	register_method(vm, &bruecke_besch_t::get_max_length, "get_max_length");
	/**
	 * @return maximal bridge height
	 */
	register_method(vm, &bruecke_besch_t::get_max_height, "get_max_height");
	/**
	 * Returns a list with available bridge types.
	 */
	STATIC register_method(vm, brueckenbauer_t::get_available_bridges, "get_available_bridges", false, true);

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

	create_slot(vm, "passenger", warenbauer_t::passagiere, true);
	create_slot(vm, "mail",      warenbauer_t::post,       true);
#ifdef DOXYGEN
	static good_desc_x passenger; ///< descriptor for passenger
	static good_desc_x mail;      ///< descriptor for mail
#endif
	/**
	 * @return freight category. 0=Passengers, 1=Mail, 2=None, >=3 anything else
	 */
	register_method(vm, &ware_besch_t::get_catg_index, "get_catg_index");

	/**
	 * Checks if this good can be interchanged with the other, in terms of
	 * transportability.
	 */
	register_method(vm, &ware_besch_t::is_interchangeable, "is_interchangeable");
	/**
	 * @returns weight of one unit of this freight
	 */
	register_method(vm, &ware_besch_t::get_weight_per_unit, "get_weight_per_unit"); // in kg

	/**
	 * Calculates transport revenue per tile and freight unit.
	 * Takes speedbonus into account.
	 * @param wt waytype of vehicle
	 * @param speedkmh actual achieved speed in km/h
	 * @returns revenue
	 */
	register_method(vm, &ware_t::calc_revenue, "calc_revenue", true);
	end_class(vm);
}
