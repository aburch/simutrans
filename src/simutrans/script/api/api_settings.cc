/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_settings.cc exports game settings functions. */

#include "api_command.h"
#include "api_simple.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../dataobj/settings.h"
#include "../../tool/simmenu.h"
#include "../../world/simworld.h"

using namespace script_api;


mytime_t get_start_time(settings_t* settings)
{
	uint32 yearmonth = 12*( max( settings->get_starting_year(),0) ) + max( settings->get_starting_month(),0);
	return yearmonth;
}


call_tool_init set_traffic_level(settings_t*, sint16 rate)
{
	cbuffer_t buf;
	buf.printf("%i", rate);
	return call_tool_init(TOOL_TRAFFIC_LEVEL | SIMPLE_TOOL, buf, 0, welt->get_public_player());
}


void export_settings(HSQUIRRELVM vm)
{
	/**
	 * Table with methods to access game settings.
	 */
	begin_class(vm, "settings", 0);

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
	register_method(vm, &settings_t::get_traffic_level, "get_traffic_level");

	/**
	 * Set traffic level. The higher the level the more city cars will be created.
	 * @param rate new traffic level, must be between 0 and 16
	 */
	register_method(vm, &set_traffic_level, "set_traffic_level", true);

	/**
	 * Returns starting time of the game.
	 * @returns table { "year" = .., "month" = .. }
	 */
	register_local_method(vm, get_start_time, "get_start_time");

	/// @returns station coverage
	register_method(vm, &settings_t::get_station_coverage, "get_station_coverage");
	/// @returns passenger factors influences passenger generation in cities
	register_method(vm, &settings_t::get_passenger_factor, "get_passenger_factor");
	/// @returns maximum distance of city to factory for supplying workers
	register_method(vm, &settings_t::get_factory_worker_radius, "get_factory_worker_radius");
	/// @returns minimum number of cities to supply workers for a factory
	register_method(vm, &settings_t::get_factory_worker_minimum_towns, "get_factory_worker_minimum_towns");
	/// @returns maximum number of cities to supply workers for a factory
	register_method(vm, &settings_t::get_factory_worker_maximum_towns, "get_factory_worker_maximum_towns");
	/// @returns freight will not enter convoy if next transfer halt is overcrowded
	register_method(vm, &settings_t::is_avoid_overcrowding, "avoid_overcrowding");
	/// @returns freight will not start when best route goes through overcrowded halt
	register_method(vm, &settings_t::is_no_routing_over_overcrowding, "no_routing_over_overcrowding");
	/// @returns true if halt capacity is separated between passengers, mail, freight
	register_method(vm, &settings_t::is_separate_halt_capacities, "separate_halt_capacities");
	/// @returns true if it is allowed to buy obsolete vehicles
	register_method(vm, &settings_t::get_allow_buying_obsolete_vehicles, "obsolete_vehicles_allowed");
	/// @returns max number of vehicles of road convois
	register_method(vm, &settings_t::get_max_road_convoi_length, "get_max_road_convoi_length");
	/// @returns max number of vehicles of rail convois
	register_method(vm, &settings_t::get_max_rail_convoi_length, "get_max_rail_convoi_length");
	/// @returns max number of vehicles of ship convois
	register_method(vm, &settings_t::get_max_ship_convoi_length, "get_max_ship_convoi_length");
	/// @returns max number of vehicles of air convois
	register_method(vm, &settings_t::get_max_air_convoi_length, "get_max_air_convoi_length");
	/// @returns true, if drive-on-left is on
	register_method(vm, &settings_t::is_drive_left, "get_drive_on_left");
	/**
	 * Three modes of transfer payment:
	 * 0 = pay for travelled Manhattan distance
	 * 1 = pay for distance difference to next transfer stop
	 * 2 = pay for distance to destination
	 */
	register_method(vm, &settings_t::get_pay_for_total_distance_mode, "get_pay_for_total_distance_mode");

	end_class(vm);
}
