#include "api.h"

/** @file api_settings.cc exports game settings functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/einstellungen.h"
#include "../../simmenu.h"
#include "../../simworld.h"

using namespace script_api;

#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);

// see api_world.cc
SQInteger push_time(HSQUIRRELVM vm, uint32 yearmonth);

SQInteger get_start_time(HSQUIRRELVM vm)
{
	settings_t* settings = param<settings_t*>::get(vm, 1);
	uint32 yearmonth = 12*( max( settings->get_starting_year(),0) ) + max( settings->get_starting_month(),0);
	return push_time(vm, yearmonth );
}

void_t set_traffic_level(settings_t*, sint16 rate)
{
	static char level[16];
	sprintf(level, "%i", rate);
	werkzeug_t *wkz = werkzeug_t::simple_tool[WKZ_TRAFFIC_LEVEL];
	wkz->set_default_param( level );
	wkz->flags |=  werkzeug_t::WFL_SCRIPT;
	welt->set_werkzeug( wkz, welt->get_spieler(1) );
	wkz->flags &= ~werkzeug_t::WFL_SCRIPT;
	return void_t();
}

void export_settings(HSQUIRRELVM vm)
{
	/**
	 * Table with methods to access game settings.
	 */
	begin_class("settings", 0);

	/**
	 * New industries will be spawned if cities grow to over 2^n times this setting.
	 * @returns number
	 */
	register_method(vm, &settings_t::get_industry_increase_every, "get_industry_increase_every");

	/**
	 * New industries will be spawned if cities grow to over 2^n times @p count.
	 * Set to zero to prevent new industries to be spawned.
	 * @param count
	 * @warning cannot be used in network games.
	 */
	register_method(vm, &settings_t::set_industry_increase_every, "set_industry_increase_every");

	/**
	 * Get traffic level.
	 */
	register_method(vm, &settings_t::get_verkehr_level, "get_traffic_level");

	/**
	 * Set traffic level. The higher the level the more city cars will be created.
	 * @param rate new traffic level, must be between 0 and 16
	 */
	register_method(vm, &set_traffic_level, "set_traffic_level", true);

	/**
	 * Returns starting time of the game.
	 * @returns table { "year" = .., "month" = .. }
	 * @typemask table()
	 */
	register_function(vm, get_start_time, "get_start_time", 1, ".");

	end_class();
}
