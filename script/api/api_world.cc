#include "api.h"

/** @file api_world.cc exports world-map functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../simworld.h"

using namespace script_api;

#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);
#define STATIC

// pushes table = { year = , month = }
SQInteger push_time(HSQUIRRELVM vm, uint32 yearmonth)
{
	sq_newtableex(vm, 2);
	param<uint32>::create_slot(vm, "year",  yearmonth/12);
	param<uint32>::create_slot(vm, "month", yearmonth%12);
	return 1;
}


SQInteger world_get_time(HSQUIRRELVM vm)
{
	sq_newtableex(vm, 2);
	return push_time(vm, welt->get_current_month() );
}


vector_tpl<sint64> const& get_world_stat(karte_t* welt, bool monthly, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (0<=INDEX  &&  INDEX<karte_t::MAX_WORLD_COST) {
		uint16 maxi = monthly ? MAX_WORLD_HISTORY_MONTHS : MAX_WORLD_HISTORY_YEARS;
		for(uint16 i = 0; i < maxi; i++) {
			if (monthly) {
				v.append( welt->get_finance_history_month(i, INDEX) );
			}
			else {
				v.append( welt->get_finance_history_year(i, INDEX) );
			}
		}
	}
	return v;
}


void export_world(HSQUIRRELVM vm)
{
	/**
	 * Table with methods to access the world, the universe, and everything.
	 */
	begin_class("world", "extend_get");

	/**
	 * Checks whether given coordinate is valid.
	 * @param k coordinate
	 * @returns true if coordinate is valid
	 */
	STATIC register_method< bool(karte_t::*)(koord) const>(vm, &karte_t::is_within_limits,  "is_coord_valid");

	/**
	 * Searches city next to the given coordinate.
	 * @param k coordinate
	 * @returns city if there is any
	 */
	STATIC register_method(vm, &karte_t::suche_naechste_stadt, "find_nearest_city");

	/**
	 * Current season.
	 * @returns season (0=winter, 1=spring, 2=summer, 3=autumn)
	 */
	STATIC register_method(vm, &karte_t::get_jahreszeit, "get_season");

	/**
	 * Returns current in-game time.
	 * @returns table { "year" = .., "month" = .. }
	 * @typemask table()
	 */
	STATIC register_function(vm, world_get_time, "get_time", 1, ".");
	/**
	 * Get monthly statistics of total number of citizens.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_citizens",          freevariable2<bool,sint32>(true, karte_t::WORLD_CITICENS), true );
	/**
	 * Get monthly statistics of total city growth.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_growth",            freevariable2<bool,sint32>(true, karte_t::WORLD_GROWTH), true );
	/**
	 * Get monthly statistics of total number of towns.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_towns",             freevariable2<bool,sint32>(true, karte_t::WORLD_TOWNS), true );
	/**
	 * Get monthly statistics of total number of factories.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_factories",         freevariable2<bool,sint32>(true, karte_t::WORLD_FACTORIES), true );
	/**
	 * Get monthly statistics of total number of convoys.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_convoys",           freevariable2<bool,sint32>(true, karte_t::WORLD_CONVOIS), true );
	/**
	 * Get monthly statistics of total number of citycars.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_citycars",          freevariable2<bool,sint32>(true, karte_t::WORLD_CITYCARS), true );
	/**
	 * Get monthly statistics of ratio transported to generated passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_ratio_pax",         freevariable2<bool,sint32>(true, karte_t::WORLD_PAS_RATIO), true );
	/**
	 * Get monthly statistics of total number of generated passengers.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_pax city_x::get_transported_pax
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_generated_pax",     freevariable2<bool,sint32>(true, karte_t::WORLD_PAS_GENERATED), true );
	/**
	 * Get monthly statistics of ratio transported to generated mail.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_ratio_mail",        freevariable2<bool,sint32>(true, karte_t::WORLD_MAIL_RATIO), true );
	/**
	 * Get monthly statistics of total number of generated mail.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_generated_mail",    freevariable2<bool,sint32>(true, karte_t::WORLD_MAIL_GENERATED), true );
	/**
	 * Get monthly statistics of ratio of factories that got supplied.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_ratio_goods",       freevariable2<bool,sint32>(true, karte_t::WORLD_GOODS_RATIO), true );
	/**
	 * Get monthly statistics of total number of transported goods.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_transported_goods", freevariable2<bool,sint32>(true, karte_t::WORLD_TRANSPORTED_GOODS), true );

	/**
	 * Get per year statistics of total number of citizens.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_citizens",          freevariable2<bool,sint32>(false, karte_t::WORLD_CITICENS), true );
	/**
	 * Get per year statistics of total city growth.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_growth",            freevariable2<bool,sint32>(false, karte_t::WORLD_GROWTH), true );
	/**
	 * Get per year statistics of total number of towns.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_towns",             freevariable2<bool,sint32>(false, karte_t::WORLD_TOWNS), true );
	/**
	 * Get per year statistics of total number of factories.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_factories",         freevariable2<bool,sint32>(false, karte_t::WORLD_FACTORIES), true );
	/**
	 * Get per year statistics of total number of convoys.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_convoys",           freevariable2<bool,sint32>(false, karte_t::WORLD_CONVOIS), true );
	/**
	 * Get per year statistics of total number of citycars.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_citycars",          freevariable2<bool,sint32>(false, karte_t::WORLD_CITYCARS), true );
	/**
	 * Get per year statistics of ratio transported to generated passengers.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_ratio_pax",         freevariable2<bool,sint32>(false, karte_t::WORLD_PAS_RATIO), true );
	/**
	 * Get per year statistics of total number of generated passengers.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_pax city_x::get_transported_pax
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_generated_pax",     freevariable2<bool,sint32>(false, karte_t::WORLD_PAS_GENERATED), true );
	/**
	 * Get per year statistics of ratio transported to generated mail.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_ratio_mail",        freevariable2<bool,sint32>(false, karte_t::WORLD_MAIL_RATIO), true );
	/**
	 * Get per year statistics of total number of generated mail.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_generated_mail",    freevariable2<bool,sint32>(false, karte_t::WORLD_MAIL_GENERATED), true );
	/**
	 * Get per year statistics of ratio of factories that got supplied.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_ratio_goods",       freevariable2<bool,sint32>(false, karte_t::WORLD_GOODS_RATIO), true );
	/**
	 * Get per year statistics of total number of transported goods.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_transported_goods", freevariable2<bool,sint32>(false, karte_t::WORLD_TRANSPORTED_GOODS), true );

	end_class();
}
