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
	return generic_get_next(vm, welt->get_staedte().get_count());
}


SQInteger world_get_city_by_index(HSQUIRRELVM vm)
{
	sint32 index = param<sint32>::get(vm, -1);
	koord pos = (0<=index  &&  (uint32)index<welt->get_staedte().get_count()) ?  welt->get_staedte()[index]->get_pos() : koord::invalid;
	// transform coordinates
	welt->get_scenario()->koord_w2sq(pos);
	return push_instance(vm, "city_x",  pos.x, pos.y);
}


static void_t set_citygrowth(stadt_t *city, bool allow)
{
	static char param[16];
	sprintf(param,"g%hi,%hi,%hi", city->get_pos().x, city->get_pos().y, allow );
	karte_t *welt = city->get_welt();
	werkzeug_t *wkz = werkzeug_t::simple_tool[WKZ_CHANGE_CITY_TOOL];
	wkz->set_default_param( param );
	wkz->flags |=  werkzeug_t::WFL_SCRIPT;
	welt->set_werkzeug( wkz, welt->get_spieler(1) );
	wkz->flags &= ~werkzeug_t::WFL_SCRIPT;
	return void_t();
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
	register_function(vm, world_get_city_by_index, "_get",    2, "xi");
	end_class(vm);

	/**
	 * Class to access cities.
	 */
	begin_class(vm, "city_x", "extend_get,coord");

	/**
	 * Constructor.
	 * @param x x-coordinate
	 * @param y y-coordinate
	 * @typemask (integer,integer)
	 */
	// actually defined simutrans/script/scenario_base.nut
	// register_function(..., "constructor", ...);

	/**
	 * Return name of city
	 * @returns name
	 */
	register_method(vm, &stadt_t::get_name,          "get_name");
	/**
	 * Get monthly statistics of number of citizens.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_citizens",              freevariable2<bool,sint32>(true, HIST_CITICENS), true);
	/**
	 * Get monthly statistics of number of city growth.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_growth",                freevariable2<bool,sint32>(true, HIST_GROWTH), true );
	/**
	 * Get monthly statistics of number of buildings.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_buildings",             freevariable2<bool,sint32>(true, HIST_BUILDING), true );
	/**
	 * Get monthly statistics of number of citycars started.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_citycars",              freevariable2<bool,sint32>(true, HIST_CITYCARS), true );
	/**
	 * Get monthly statistics of number of transported passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_transported_pax",       freevariable2<bool,sint32>(true, HIST_PAS_TRANSPORTED), true );
	/**
	 * Get monthly statistics of number of generated passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_generated_pax",         freevariable2<bool,sint32>(true, HIST_PAS_GENERATED), true );
	/**
	 * Get monthly statistics of number of transported mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_transported_mail",      freevariable2<bool,sint32>(true, HIST_MAIL_TRANSPORTED), true );
	/**
	 * Get monthly statistics of number of generated mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_city_stat, "get_generated_mail",        freevariable2<bool,sint32>(true, HIST_MAIL_GENERATED), true );
	/**
	 * Get per year statistics of number of citizens.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_citizens",         freevariable2<bool,sint32>(false, HIST_CITICENS), true );
	/**
	 * Get per year statistics of number of city growth.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_growth",           freevariable2<bool,sint32>(false, HIST_GROWTH), true );
	/**
	 * Get per year statistics of number of buildings.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_buildings",        freevariable2<bool,sint32>(false, HIST_BUILDING), true );
	/**
	 * Get per year statistics of number of citycars started.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_citycars",         freevariable2<bool,sint32>(false, HIST_CITYCARS), true );
	/**
	 * Get per year statistics of number of transported passengers.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_transported_pax",  freevariable2<bool,sint32>(false, HIST_PAS_TRANSPORTED), true );
	/**
	 * Get per year statistics of number of generated passengers.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_generated_pax",    freevariable2<bool,sint32>(false, HIST_PAS_GENERATED), true );
	/**
	 * Get per year statistics of number of transported mail.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_transported_mail", freevariable2<bool,sint32>(false, HIST_MAIL_TRANSPORTED), true );
	/**
	 * Get per year statistics of number of generated mail.
	 * @returns array, index [0] corresponds to current year
	 */
	register_method_fv(vm, &get_city_stat, "get_year_generated_mail",   freevariable2<bool,sint32>(false, HIST_MAIL_GENERATED), true );

	/**
	 * Check city growth allowance.
	 * @returns whether city growth is enabled for this city
	 */
	register_method(vm, &stadt_t::get_citygrowth,  "get_citygrowth_enabled");

	/**
	 * Position of townhall.
	 * @returns townhall position
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
	 * @warning cannot be used in network games.
	 */
	register_method(vm, &stadt_t::change_size, "change_size");

	/**
	 * Enable or disable city growth.
	 */
	register_method(vm, &set_citygrowth, "set_citygrowth_enabled", true);

	/**
	 * Change city name.
	 * @warning cannot be used in network games.
	 */
	register_method(vm, &stadt_t::set_name, "set_name");

	end_class(vm);
}
