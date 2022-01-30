/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_city.cc exports city related functions. */

#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simcity.h"
#include "../../simmenu.h"
#include "../../simworld.h"
#include "../../dataobj/scenario.h"

using namespace script_api;


vector_tpl<sint64> const& get_city_stat(stadt_t* city, bool monthly, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (city  &&  0<=INDEX  &&  INDEX<MAX_CITY_HISTORY) {
		uint16 maxi = monthly ? MAX_CITY_HISTORY_MONTHS :MAX_CITY_HISTORY_YEARS;
		for(uint16 i = 0; i < maxi; i++) {
			if (monthly) {
				v.append( city->get_finance_history_month(i, INDEX) );
			}
			else {
				v.append( city->get_finance_history_year(i, INDEX) );
			}
		}
	}
	return v;
}


SQInteger world_get_next_city(HSQUIRRELVM vm)
{
	return generic_get_next(vm, welt->get_cities().get_count());
}

namespace script_api {
	declare_fake_param(city_list_t, "city_list_x");
}

stadt_t* world_get_city_by_index(city_list_t, uint32 index)
{
	return index < welt->get_cities().get_count()  ?  welt->get_cities()[index] : NULL;
}

call_tool_init set_citygrowth(stadt_t *city, bool allow)
{
	cbuffer_t buf;
	buf.printf("g%hi,%hi,%hi", city->get_pos().x, city->get_pos().y, (short)allow );
	return call_tool_init(TOOL_CHANGE_CITY | SIMPLE_TOOL, (const char*)buf, 0, welt->get_public_player());
}

call_tool_init city_set_name(stadt_t* city, const char* name)
{
	return command_rename(welt->get_public_player(), 't', welt->get_cities().index_of(city), name);
}


call_tool_work city_change_size(stadt_t *city, sint32 delta)
{
	cbuffer_t buf;
	buf.printf("%i", delta);
	grund_t *gr = welt->lookup_kartenboden(city->get_pos());
	if (gr) {
		return call_tool_work(TOOL_CHANGE_CITY_SIZE | GENERAL_TOOL, (const char*)buf, 0, welt->get_public_player(), gr->get_pos());
	}
	else {
		return "Invalid coordinate.";
	}
}


void export_city(HSQUIRRELVM vm)
{
	/**
	 * Implements iterator to iterate through the list of all cities on the map.
	 *
	 * Usage:
	 * @code
	 * local list = city_list_x()
	 * foreach(city in list) {
	 *     ... // city is an instance of the city_x class
	 * }
	 * @endcode
	 */
	begin_class(vm, "city_list_x", 0);
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, world_get_next_city,     "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_method(vm, &world_get_city_by_index, "_get", true);
	end_class(vm);

	/**
	 * Class to access cities.
	 */
	begin_class(vm, "city_x", "extend_get,coord,ingame_object");

	/**
	 * Constructor.
	 * @param x x-coordinate
	 * @param y y-coordinate
	 * @typemask (integer,integer)
	 */
	// actually defined in simutrans/script/script_base.nut
	// register_function(..., "constructor", ...);

	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<stadt_t*>(vm); //register_function("is_valid")
	/**
	 * Return name of city
	 * @returns name
	 */
	register_method(vm, &stadt_t::get_name,          "get_name");
	/**
	 * Change city name.
	 * @ingroup rename_func
	 */
	register_method(vm, &city_set_name, "set_name", true);
	/**
	 * Get monthly statistics of number of citizens.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_citizens",              freevariable<bool,sint32>(true, HIST_CITIZENS), true);
	/**
	 * Get monthly statistics of number of city growth.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_growth",                freevariable<bool,sint32>(true, HIST_GROWTH), true );
	/**
	 * Get monthly statistics of number of buildings.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_buildings",             freevariable<bool,sint32>(true, HIST_BUILDING), true );
	/**
	 * Get monthly statistics of number of citycars started.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_citycars",              freevariable<bool,sint32>(true, HIST_CITYCARS), true );
	/**
	 * Get monthly statistics of number of transported passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_transported_pax",       freevariable<bool,sint32>(true, HIST_PAS_TRANSPORTED), true );
	/**
	 * Get monthly statistics of number of generated passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_generated_pax",         freevariable<bool,sint32>(true, HIST_PAS_GENERATED), true );
	/**
	 * Get monthly statistics of number of transported mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_transported_mail",      freevariable<bool,sint32>(true, HIST_MAIL_TRANSPORTED), true );
	/**
	 * Get monthly statistics of number of generated mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_generated_mail",        freevariable<bool,sint32>(true, HIST_MAIL_GENERATED), true );
	/**
	 * Get per year statistics of number of citizens.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_citizens",         freevariable<bool,sint32>(false, HIST_CITIZENS), true );
	/**
	 * Get per year statistics of number of city growth.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_growth",           freevariable<bool,sint32>(false, HIST_GROWTH), true );
	/**
	 * Get per year statistics of number of buildings.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_buildings",        freevariable<bool,sint32>(false, HIST_BUILDING), true );
	/**
	 * Get per year statistics of number of citycars started.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_citycars",         freevariable<bool,sint32>(false, HIST_CITYCARS), true );
	/**
	 * Get per year statistics of number of transported passengers.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_transported_pax",  freevariable<bool,sint32>(false, HIST_PAS_TRANSPORTED), true );
	/**
	 * Get per year statistics of number of generated passengers.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_generated_pax",    freevariable<bool,sint32>(false, HIST_PAS_GENERATED), true );
	/**
	 * Get per year statistics of number of transported mail.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_transported_mail", freevariable<bool,sint32>(false, HIST_MAIL_TRANSPORTED), true );
	/**
	 * Get per year statistics of number of generated mail.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_generated_mail",   freevariable<bool,sint32>(false, HIST_MAIL_GENERATED), true );

	/**
	 * Check city growth allowance.
	 * @returns whether city growth is enabled for this city
	 */
	register_method(vm, &stadt_t::get_citygrowth,  "get_citygrowth_enabled");

	/**
	 * Position of town-hall.
	 * @returns town-hall position
	 */
	register_method(vm, &stadt_t::get_pos,         "get_pos");

	/**
	 * City limits.
	 *
	 * City area is between get_pos_nw().x and get_pos_se().x, and get_pos_nw().y and get_pos_se().y.
	 *
	 * It is @b not guaranteed that get_pos_nw().x <= get_pos_se().x or get_pos_nw().y <= get_pos_se().y holds!
	 * @returns coordinate of one corner of city limit
	 */
	register_method(vm, &stadt_t::get_linksoben,   "get_pos_nw");

	/**
	 * City limits.
	 *
	 * City area is between get_pos_nw().x and get_pos_se().x, and get_pos_nw().y and get_pos_se().y.
	 *
	 * It is @b not guaranteed that get_pos_nw().x <= get_pos_se().x or get_pos_nw().y <= get_pos_se().y holds!
	 * @returns coordinate of another corner of city limit
	 */
	register_method(vm, &stadt_t::get_rechtsunten, "get_pos_se");

	/**
	 * Change city size. City will immediately grow.
	 * @param delta City size will change by this number.
	 * @ingroup game_cmd
	 */
	register_method(vm, city_change_size, "change_size", true);

	/**
	 * Enable or disable city growth.
	 * @ingroup game_cmd
	 */
	register_method(vm, &set_citygrowth, "set_citygrowth_enabled", true);

	end_class(vm);
}
