/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Game settings
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include <string>
#include <math.h>

#include "settings.h"
#include "environment.h"
#include "../simconst.h"
#include "../simtypes.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../path_explorer.h"
#include "../bauer/wegbauer.h"
#include "../descriptor/way_desc.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"
#include "../utils/float32e8_t.h"
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "loadsave.h"
#include "tabfile.h"
#include "translator.h"
#include "../descriptor/intro_dates.h"

#include "../tpl/minivec_tpl.h"


#define NEVER 0xFFFFU

sint8 env_t::reverse_base_offsets[8][3];

settings_t::settings_t() :
	filename(""),
	heightfield("")
{
	// These control when settings from a savegame
	// are overridden by simuconf.tab files
	// The version in default_settings is *always* used
	progdir_overrides_savegame_settings = false;
	pak_overrides_savegame_settings = false;
	userdir_overrides_savegame_settings = false;

	size_x = 256;
	size_y = 256;

	map_number = sim_async_rand(SINT32_MAX_VALUE);

	/* new setting since version 0.85.01
	 * @author prissi
	 */
	factory_count = 12;
	tourist_attractions = 8;

	city_count = 8;
	mean_einwohnerzahl = 1600;

	station_coverage_size = 3;
	station_coverage_size_factories = 3;

	traffic_level = 5;

	show_pax = true;

	// default climate zones
	set_default_climates( );
	winter_snowline = 7;	// not mediterranean
	groundwater = -2;            //25-Nov-01        Markus Weber    Added

	max_mountain_height = 160;                  //can be 0-160.0  01-Dec-01        Markus Weber    Added
	map_roughness = 0.6;                        //can be 0-1      01-Dec-01        Markus Weber    Added

	river_number = 16;
	min_river_length = 16;
	max_river_length = 256;

	// since the turning rules are different, driving must now be saved here
	drive_on_left = false;
	signals_on_left = false;

	// forest setting ...
	forest_base_size = 36; 	// Base forest size - minimal size of forest - map independent
	forest_map_size_divisor = 38;	// Map size divisor - smaller it is the larger are individual forests
	forest_count_divisor = 16;	// Forest count divisor - smaller it is, the more forest are generated
	forest_inverse_spare_tree_density = 5;	// Determines how often are spare trees going to be planted (works inversely)
	max_no_of_trees_on_square = 3;	// Number of trees on square 2 - minimal usable, 3 good, 5 very nice looking
	tree_climates = 0;	// bit set, if this climate is to be covered with trees entirely
	no_tree_climates = 0;	// bit set, if this climate is to be void of random trees
	no_trees = false;	// if set, no trees at all, may be useful for low end engines

	lake = false;	// if set lakes will be added to map

	// some settings more
	allow_player_change = true;
	use_timeline = 2;
	starting_year = 1930;
	starting_month = 0;
	bits_per_month = 20;
	calc_job_replenishment_ticks();
	meters_per_tile = 1000;
	base_meters_per_tile = 1000;
	base_bits_per_month = 18;
	job_replenishment_per_hundredths_of_months = 100;

	beginner_mode = false;
	beginner_price_factor = 1500;

	rotation = 0;

	origin_x = origin_y = 0;

	electric_promille = 1000;

	// town growth factors
	passenger_multiplier = 40;
	mail_multiplier = 20;
	goods_multiplier = 20;
	electricity_multiplier = 20;

	// Also there are size dependent factors (0 causes crash !)
	growthfactor_small = 400;
	growthfactor_medium = 200;
	growthfactor_large = 100;

	industry_increase = 2000;
	city_isolation_factor = 1;

	special_building_distance = 3;

	factory_arrival_periods = 4;

	factory_enforce_demand = true;

	factory_maximum_intransit_percentage = 0;

	electric_promille = 330;

#ifdef OTTD_LIKE
	/* prissi: crossconnect all factories (like OTTD and similar games) */
	crossconnect_factories=true;
	crossconnect_factor=100;
#else
	/* prissi: crossconnect a certain number */
	crossconnect_factories=false;
	crossconnect_factor=33;
#endif

	/* minimum spacing between two factories */
	min_factory_spacing = 6;
	max_factory_spacing = 40;
	max_factory_spacing_percentage = 0; // off

	/* prissi: do not distribute goods to overflowing factories */
	just_in_time = 1;

	random_pedestrians = true;
	stadtauto_duration = 36;	// three years

	// to keep names consistent
	numbered_stations = false;

	num_city_roads = 0;
	num_intercity_roads = 0;

	max_route_steps = 1000000;
	max_choose_route_steps = 200;
	max_transfers = 9;
	max_hops = 2000;

	// Eighteen hours - the control of real journey times
	// should be the journey time tolerance feature, not
	// the maximum waiting: discarding should be a last
	// resort, not an integral part of the journey time
	// measurement mechanics.
	// 19440 / 18 = 1080 (or 18 hours).
	passenger_max_wait = 19440;

	// 4 is faster; 2 is more accurate.
	// Not recommended to use anything other than 2 or 4
	// @author: jamespetts
	max_rerouting_interval_months = 2;

	/* multiplier for steps on diagonal:
	 * 1024: TT-like, factor 2, vehicle will be too long and too fast
	 * 724: correct one, factor sqrt(2)
	 */
	pak_diagonal_multiplier = 724;

	// read default from env_t
	// should be set in simmain.cc (taken from pak-set simuconf.tab
	way_height_clearance = env_t::default_settings.get_way_height_clearance();
	if (way_height_clearance < 0 || way_height_clearance >2) {
		// if outside bounds, then set to default = 1
		way_height_clearance = 1;
	}

	strcpy( language_code_names, "en" );

	// default AIs active
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  i<2  ) {
			player_active[i] = true;
			player_type[i] = player_t::HUMAN;
		}
		else if(  i==3  ) {
			player_active[i] = true;
			player_type[i] = player_t::AI_PASSENGER;
		}
		else if(  i==6  ) {
			player_active[i] = true;
			player_type[i] = player_t::AI_GOODS;
		}
		else {
			player_active[i] = false;
			player_type[i] = player_t::EMPTY;
		}
		// undefined colors
		default_player_color[i][0] = 255;
		default_player_color[i][1] = 255;
	}
	default_player_color_random = false;
	default_ai_construction_speed = env_t::default_ai_construction_speed;

	/* the big cost section */
	freeplay = false;
	starting_money = 20000000;
	for(  int i=0; i<10; i++  ) {
		startingmoneyperyear[i].year = 0;
		startingmoneyperyear[i].money = 0;
		startingmoneyperyear[i].interpol = 0;
	}

	// six month time frame for starting first convoi
	remove_dummy_player_months = 6;

	// off
	unprotect_abandoned_player_months = 0;

	maint_building = 5000;	// normal buildings
	way_toll_runningcost_percentage = 0;
	way_toll_waycost_percentage = 0;
	way_toll_revenue_percentage = 0;
	seaport_toll_revenue_percentage = 0;
	airport_toll_revenue_percentage = 0;

	allow_underground_transformers = true;
	disable_make_way_public = false;

	// stop buildings
	cst_multiply_dock=-50000;
	cst_multiply_station=-60000;
	cst_multiply_roadstop=-40000;
	cst_multiply_airterminal=-300000;
	cst_multiply_post=-30000;
	cst_multiply_headquarter=-100000;
	cst_depot_rail=-100000;
	cst_depot_road=-130000;
	cst_depot_ship=-250000;
	cst_depot_air=-500000;
	// alter landscape
	cst_buy_land=-10000;
	cst_alter_land=-100000;
	cst_alter_climate=-100000;
	cst_set_slope=-250000;
	cst_found_city=-500000000;
	cst_multiply_found_industry=-2000000;
	cst_remove_tree=-10000;
	cst_multiply_remove_haus=-100000;
	cst_multiply_remove_field=-500000;
	// cost for transformers
	cst_transformer=-250000;
	cst_maintain_transformer=-2000;

	cst_make_public_months = 60;

	// costs for the way searcher
	way_count_straight=1;
	way_count_curve=2;
	way_count_double_curve=6;
	way_count_90_curve=15;
	way_count_slope=10;
	way_count_tunnel=8;
	way_max_bridge_len=15;
	way_count_leaving_road=25;

	// default: joined capacities
	separate_halt_capacities = false;

	// Cornering settings
	// @author: jamespetts
	corner_force_divider[waytype_t(road_wt)] = 5;
	corner_force_divider[waytype_t(track_wt)] = 10;
	corner_force_divider[waytype_t(tram_wt)] = 10;
	corner_force_divider[waytype_t(monorail_wt)] = 10;
	corner_force_divider[waytype_t(maglev_wt)] = 10;
	corner_force_divider[waytype_t(narrowgauge_wt)] = 10;

	curve_friction_factor[waytype_t(road_wt)] = 0;
	curve_friction_factor[waytype_t(track_wt)] = 0;
	curve_friction_factor[waytype_t(tram_wt)] = 0;
	curve_friction_factor[waytype_t(monorail_wt)] =0;
	curve_friction_factor[waytype_t(maglev_wt)] = 0;
	curve_friction_factor[waytype_t(narrowgauge_wt)] = 0;

	tilting_min_radius_effect = 1000;

	set_meters_per_tile(250);

	tolerable_comfort_short = 15;
	tolerable_comfort_median_short = 60;
	tolerable_comfort_median_median = 100;
	tolerable_comfort_median_long = 160;
	tolerable_comfort_long = 220;
	tolerable_comfort_short_minutes = 2;
	tolerable_comfort_median_short_minutes = 30;
	tolerable_comfort_median_median_minutes = 120;
	tolerable_comfort_median_long_minutes = 300;
	tolerable_comfort_long_minutes = 720;
	max_luxury_bonus_differential = 75;
	max_luxury_bonus_percent = 50;
	max_discomfort_penalty_differential = 200;
	max_discomfort_penalty_percent = 95;
	cache_comfort_tables();

	catering_min_minutes = 60;
	catering_level1_minutes = 90;
	catering_level1_max_revenue = 150;
	catering_level2_minutes = 120;
	catering_level2_max_revenue = 250;
	catering_level3_minutes = 150;
	catering_level3_max_revenue = 350;
	catering_level4_minutes = 240;
	catering_level4_max_revenue = 400;
	catering_level5_minutes = 300;
	catering_level5_max_revenue = 475;

	// Fill the blank catering revenue tables.
	// There are exactly six of these tables (hence the use of a fixed array).
	// The actual tables have only a little inline data (one pointer, two numbers).
	// The tables are constructed automatically with no data by the C++ compiler.
	// This will load them...
	cache_catering_revenues();

	tpo_min_minutes = 120;
	tpo_revenue = 300;

	// Obsolete vehicles running costs adjustment
	obsolete_running_cost_increase_percent = 400; //Running costs will be this % of normal costs after vehicle has been obsolete
	obsolete_running_cost_increase_phase_years = 20; //for this number of years.

	// Passenger routing settings
	passenger_routing_packet_size = 7;
	max_alternative_destinations_commuting = 3;
	max_alternative_destinations_visiting = 5;

	min_alternative_destinations_visiting = 1;
	min_alternative_destinations_commuting = 2;

	// With a default value of zero, the absolute number of "max_alternative_destinations" will be used.
	max_alternative_destinations_per_job_millionths = 0;
	max_alternative_destinations_per_visitor_demand_millionths = 0;
	min_alternative_destinations_per_job_millionths = 0;
	min_alternative_destinations_per_visitor_demand_millionths = 0;

	always_prefer_car_percent = 10;
	congestion_density_factor = 12;

	//@author: jamespetts
	// Passenger routing settings
	passenger_routing_packet_size = 7;

	//@author: jamespetts
	// Factory retirement settings
	factory_max_years_obsolete = 30;

	//@author: jamespetts
	// Insolvency and debt settings
	interest_rate_percent = 10;
	allow_insolvency  = 0;
	allow_purchases_when_insolvent  = 0;

	// Reversing settings
	// @author: jamespetts
	unit_reverse_time = 0;
	hauled_reverse_time = 0;
	turntable_reverse_time = 0;
	road_reverse_time = 0;

	unit_reverse_time_seconds = 65535;
	hauled_reverse_time_seconds = 65535;
	turntable_reverse_time_seconds = 65535;
	road_reverse_time_seconds = 65535;

	// Global power factor
	// @author: jamespetts
	global_power_factor_percent = 100;
	global_force_factor_percent = 100;

	avoid_overcrowding = false;

	// How and whether weight limits are enforced.
	// @author: jamespetts
	enforce_weight_limits = 1;

	allow_airports_without_control_towers = true;

	allow_buying_obsolete_vehicles = 1;

	// default: load also private extensions of the pak file
	with_private_paks = true;

	// The defaults for journey time tolerance.
	// Applies to passengers (and hand delivery of mail) only.
	// @author: jamespetts

	min_visiting_tolerance = 10 * 10; // 10 minutes
	range_visiting_tolerance = 360 * 10; //: Six hours

	min_commuting_tolerance = 30 * 10; // Half an hour
	range_commuting_tolerance = 120 * 10; // Two hours


	used_vehicle_reduction = 0;

	// some network thing to keep client in sync
	random_counter = 0;	// will be set when actually saving
	frames_per_second = 20;
	frames_per_step = 4;

	quick_city_growth = false;
	assume_everywhere_connected_by_road = false;

	allow_routing_on_foot = false;

	min_wait_airport = 0;

	toll_free_public_roads = false;

	private_car_toll_per_km = 1;

	towns_adopt_player_roads = true;

	max_elevated_way_building_level = 2;

	city_threshold_size = 1000;
	capital_threshold_size = 10000;
	max_small_city_size = 1000;
	max_city_size = 5000;

	// 0 values for these mean that the above absolute threasholds are used instead.
	// Set values in simuconf.tab to activate this feature.
	capital_threshold_percentage = 0;
	city_threshold_percentage = 0;

	power_revenue_factor_percentage=100;

	allow_making_public = true;

	reroute_check_interval_steps = 8192;

	walking_speed = 5;

	random_mode_commuting = random_mode_visiting = 2;

	tolerance_modifier_percentage = 100;

	industry_density_proportion_override = 0;

	max_route_tiles_to_process_in_a_step = 2048;

	for(uint8 i = 0; i < 17; i ++)
	{
		if(i != road_wt)
		{
			default_increase_maintenance_after_years[i] = 30;
		}
		else
		{
			default_increase_maintenance_after_years[i] = 15;
		}
	}
	server_frames_ahead = 4;

	spacing_shift_mode = SPACING_SHIFT_PER_STOP;
	spacing_shift_divisor = 24*60;

	population_per_level = 3;
	visitor_demand_per_level = 3;
	jobs_per_level = 2;
	mail_per_level = 1;

	// Forge cost for ways (array of way types)
	forge_cost_road = 10000;
	forge_cost_track = 20000;
	forge_cost_water = 40000;
	forge_cost_monorail = 20000;
	forge_cost_maglev = 30000;
	forge_cost_tram = 5000;
	forge_cost_narrowgauge = 10000;
	forge_cost_air = 25000;

	parallel_ways_forge_cost_percentage_road = 50;
	parallel_ways_forge_cost_percentage_track = 35;
	parallel_ways_forge_cost_percentage_water = 75;
	parallel_ways_forge_cost_percentage_monorail = 35;
	parallel_ways_forge_cost_percentage_maglev = 40;
	parallel_ways_forge_cost_percentage_tram = 85;
	parallel_ways_forge_cost_percentage_narrowgauge = 35;
	parallel_ways_forge_cost_percentage_air = 85;

	passenger_trips_per_month_hundredths = 200;
	mail_packets_per_month_hundredths = 10;
	max_onward_trips = 3;
	onward_trip_chance_percent = 25;
	commuting_trip_chance_percent = 66;

	max_diversion_tiles = 16;

	way_degradation_fraction = 7;

	way_wear_power_factor_road_type = 4;
	way_wear_power_factor_rail_type = 1;
	standard_axle_load = 8;
	citycar_way_wear_factor = 2;

	sighting_distance_meters = 250;
	assumed_curve_radius_45_degrees = 1000;

	max_speed_drive_by_sight_kmh = 0;
	max_speed_drive_by_sight = 0;

	time_interval_seconds_to_clear = 600;
	time_interval_seconds_to_caution = 300;

	town_road_speed_limit = 50;

	minimum_staffing_percentage_consumer_industry = 66;
	minimum_staffing_percentage_full_production_producer_industry = 80;

	max_comfort_preference_percentage = 500;

	rural_industries_no_staff_shortage = true;
	auto_connect_industries_and_attractions_by_road = 4;

	path_explorer_time_midpoint = 64;
	save_path_explorer_data = true;

	show_future_vehicle_info = true;
}

void settings_t::set_default_climates()
{
	static sint16 borders[MAX_CLIMATES] = { 0, -3, -2, 3, 6, 8, 10, 10 };
	memcpy( climate_borders, borders, sizeof(sint16)*MAX_CLIMATES );
}



void settings_t::rdwr(loadsave_t *file)
{
	// used to be called einstellungen_t - keep old name during save/load for compatibility
	xml_tag_t e( file, "einstellungen_t" );

	if(file->get_version() < 86000) {
		uint32 dummy;

		file->rdwr_long(size_x );
		size_y = size_x;

		file->rdwr_long(map_number );

		// to be compatible with previous savegames
		dummy = 0;
		file->rdwr_long(dummy );	//dummy!
		factory_count = 12;
		tourist_attractions = 12;

		// now towns
		mean_einwohnerzahl = 1600;
		dummy =  city_count;
		file->rdwr_long(dummy );
		dummy &= 127;
		if(dummy>63) {
			dbg->warning("settings_t::rdwr()", "This game was saved with too many cities! (%i of maximum 63). Simutrans may crash!", dummy);
		}
		city_count = dummy;

		// rest
		file->rdwr_long(dummy );	// scroll ignored
		file->rdwr_long(traffic_level );
		file->rdwr_long(show_pax );
		dummy = groundwater;
		file->rdwr_long(dummy );
		groundwater = (sint16)(dummy/16);
		file->rdwr_double(max_mountain_height );
		file->rdwr_double(map_roughness );

		station_coverage_size = 3;
		station_coverage_size_factories = 3;
		beginner_mode = false;
		rotation = 0;
	}
	else {
		// newer versions
		file->rdwr_long(size_x );
		file->rdwr_long(map_number );

		// industries
		file->rdwr_long(factory_count );
		if(file->get_version()<99018) {
			uint32 dummy;	// was city chains
			file->rdwr_long(dummy );
		}
		else {
			file->rdwr_long( electric_promille );
		}
		file->rdwr_long(tourist_attractions );

		// now towns
		file->rdwr_long(mean_einwohnerzahl );
		file->rdwr_long(city_count );

		// rest
		if(file->get_version() < 101000) {
			uint32 dummy;	// was scroll dir
			file->rdwr_long(dummy);
		}
		file->rdwr_long(traffic_level );
		file->rdwr_long(show_pax );
		sint32 dummy = groundwater;
		file->rdwr_long(dummy );
		if(file->get_version() < 99005) {
			groundwater = (sint16)(dummy/16);
		}
		else {
			groundwater = (sint16)dummy;
		}
		file->rdwr_double(max_mountain_height );
		file->rdwr_double(map_roughness );

		if(file->get_version() >= 86003) {
			dummy = station_coverage_size;
			file->rdwr_long(dummy);
			station_coverage_size = (uint16)dummy;
		}

		if(file->get_extended_version() >= 11)
		{
			file->rdwr_short(station_coverage_size_factories);
			if ( file->get_version() <= 112002) {
				// Correct broken save files on load.
				if (station_coverage_size_factories < 3) {
					station_coverage_size_factories = 3;
				}
			}
		}

		if(file->get_version() >= 86006) {
			// handle also size on y direction
			file->rdwr_long(size_y );
		}
		else {
			size_y = size_x;
		}

		if(file->get_version() >= 86011) {
			// some more settings
			file->rdwr_byte(allow_player_change);
			file->rdwr_byte(use_timeline);
			file->rdwr_short(starting_year);
		}
		else {
			allow_player_change = 1;
			use_timeline = 1;
			starting_year = 1930;
		}

		if(file->get_version()>=88005) {
			file->rdwr_short(bits_per_month);
		}
		else {
			bits_per_month = 18;
			calc_job_replenishment_ticks();
		}

		if(file->get_version()>=89003) {
			file->rdwr_bool(beginner_mode);
		}
		else {
			beginner_mode = false;
		}
		if(  file->get_version()>120000  ){
			file->rdwr_byte( just_in_time );
		}
		else if(file->get_version()>=89004) {
			bool jit = just_in_time;
			file->rdwr_bool(jit);
			just_in_time = jit ? 1 : 0;
		}
		// rotation of the map with respect to the original value
		if(file->get_version()>=99015) {
			file->rdwr_byte(rotation);
		}
		else {
			rotation = 0;
		}

		// clear the name when loading ...
		if(file->is_loading()) {
			filename = "";
		}

		// climate borders
		if(file->get_version()>=91000) {
			for(  int i=0;  i<8;  i++ ) {
				file->rdwr_short(climate_borders[i] );
			}
			file->rdwr_short(winter_snowline );
		}

		if(  file->is_loading()  &&  file->get_version() < 112007  ) {
			groundwater *= env_t::pak_height_conversion_factor;
			for(  int i = 0;  i < 8;  i++  ) {
				climate_borders[i] *= env_t::pak_height_conversion_factor;
			}
			winter_snowline *= env_t::pak_height_conversion_factor;
			way_height_clearance = env_t::pak_height_conversion_factor;
		}

		// since vehicle will need realignment afterwards!
		if(file->get_version()<=99018) {
			vehicle_base_t::set_diagonal_multiplier( pak_diagonal_multiplier, 1024 );
		}
		else {
			uint16 old_multiplier = pak_diagonal_multiplier;
			file->rdwr_short( old_multiplier );
			vehicle_base_t::set_diagonal_multiplier( pak_diagonal_multiplier, old_multiplier );
			// since vehicle will need realignment afterwards!
		}

		// 16 was the default value of this setting.
		uint32 old_passenger_factor = 16;

		if(file->get_version()>=101000) {
			// game mechanics
			file->rdwr_short(origin_x );
			file->rdwr_short(origin_y );

			if(file->get_extended_version() < 12)
			{
				// Was passenger factor.
				file->rdwr_long(old_passenger_factor);
			}

			// town growth stuff
			if(file->get_version()>102001) {
				file->rdwr_long(passenger_multiplier );
				file->rdwr_long(mail_multiplier );
				file->rdwr_long(goods_multiplier );
				file->rdwr_long(electricity_multiplier );
				file->rdwr_long(growthfactor_small );
				file->rdwr_long(growthfactor_medium );
				file->rdwr_long(growthfactor_large );
				if(file->get_extended_version() < 12)
				{
					// Was factory_worker_percentage, tourist_percentage and factory_worker_radius.
					uint16 dummy;
					file->rdwr_short(dummy);
					file->rdwr_short(dummy);
					file->rdwr_short(dummy);
				}
			}

			file->rdwr_long(electric_promille);

			file->rdwr_short(min_factory_spacing);
			file->rdwr_bool(crossconnect_factories);
			file->rdwr_short(crossconnect_factor);

			file->rdwr_bool(random_pedestrians);
			file->rdwr_long(stadtauto_duration);

			file->rdwr_bool(numbered_stations);
			if(file->get_version() <= 102002 || (file->get_extended_version() < 8 && file->get_extended_version() != 0))
			{
				if(file->is_loading())
				{
					num_city_roads = 1;
					city_roads[0].intro = 0;
					city_roads[0].retire = 0;
					// intercity roads were not saved in old savegames
					num_intercity_roads = 0;
				}
				file->rdwr_str(city_roads[0].name, lengthof(city_roads[0].name));
			}
			else
			{
				// several roads ...
				file->rdwr_short(num_city_roads);
				if(  num_city_roads>=16  ) {
					dbg->fatal("settings_t::rdwr()", "Too many (%i) city roads!", num_city_roads);
				}
				for(  int i=0;  i<num_city_roads;  i++  ) {
					file->rdwr_str(city_roads[i].name, lengthof(city_roads[i].name) );
					file->rdwr_short(city_roads[i].intro );
					file->rdwr_short(city_roads[i].retire );
				}
				// several intercity roads ...
				file->rdwr_short(num_intercity_roads);
				if(  num_intercity_roads>=16  ) {
					dbg->fatal("settings_t::rdwr()", "Too many (%i) intercity roads!", num_intercity_roads);
				}
				for(  int i=0;  i<num_intercity_roads;  i++  ) {
					file->rdwr_str(intercity_roads[i].name, lengthof(intercity_roads[i].name) );
					file->rdwr_short(intercity_roads[i].intro );
					file->rdwr_short(intercity_roads[i].retire );
				}
			}
			file->rdwr_long(max_route_steps );
			file->rdwr_long(max_transfers );
			file->rdwr_long(max_hops );

			file->rdwr_long(beginner_price_factor );

			// name of stops
			file->rdwr_str(language_code_names, lengthof(language_code_names) );

			// restore AI state
			//char password[16]; // unused
			for(  int i=0;  i<15;  i++  ) {
				file->rdwr_bool( player_active[i]);
				file->rdwr_byte( player_type[i]);
				if(  file->get_version()<=102002 || file->get_extended_version() == 7) {
					char dummy[17];
					dummy[0] = 0;
					file->rdwr_str(dummy, lengthof(dummy));
				}
			}

			// cost section ...
			file->rdwr_bool( freeplay);
			if(  file->get_version()>102002 && file->get_extended_version() != 7 ) {
				file->rdwr_longlong( starting_money);
				// these must be saved, since new player will get different amounts eventually
				for(  int i=0;  i<10;  i++  ) {
					file->rdwr_short(startingmoneyperyear[i].year );
					file->rdwr_longlong(startingmoneyperyear[i].money );
					file->rdwr_bool(startingmoneyperyear[i].interpol );
				}
			}
			else {
				// compatibility code
				sint64 save_starting_money = starting_money;
				if(  file->is_saving()  ) {
					if(save_starting_money==0) {
						save_starting_money = get_starting_money(starting_year );
					}
					if(save_starting_money==0) {
						save_starting_money = env_t::default_settings.get_starting_money(starting_year );
					}
					if(save_starting_money==0) {
						save_starting_money = 20000000;
					}
				}
				file->rdwr_longlong(save_starting_money );
				if(file->is_loading()) {
					if(save_starting_money==0) {
						save_starting_money = env_t::default_settings.get_starting_money(starting_year );
					}
					if(save_starting_money==0) {
						save_starting_money = 20000000;
					}
					starting_money = save_starting_money;
				}
			}
			file->rdwr_long(maint_building );

			file->rdwr_longlong(cst_multiply_dock );
			file->rdwr_longlong(cst_multiply_station );
			file->rdwr_longlong(cst_multiply_roadstop );
			file->rdwr_longlong(cst_multiply_airterminal );
			file->rdwr_longlong(cst_multiply_post );
			file->rdwr_longlong(cst_multiply_headquarter );
			file->rdwr_longlong(cst_depot_rail );
			file->rdwr_longlong(cst_depot_road );
			file->rdwr_longlong(cst_depot_ship );
			file->rdwr_longlong(cst_depot_air );
			if(  file->get_version()<=102001  ) {
				sint64 dummy64 = 100000;
				file->rdwr_longlong(dummy64 );
				file->rdwr_longlong(dummy64 );
				file->rdwr_longlong(dummy64 );
			}
			// alter landscape
			file->rdwr_longlong(cst_buy_land );
			file->rdwr_longlong(cst_alter_land );
			file->rdwr_longlong(cst_set_slope );
			file->rdwr_longlong(cst_found_city );
			file->rdwr_longlong(cst_multiply_found_industry );
			file->rdwr_longlong(cst_remove_tree );
			file->rdwr_longlong(cst_multiply_remove_haus );
			file->rdwr_longlong(cst_multiply_remove_field );
			// cost for transformers
			file->rdwr_longlong(cst_transformer );
			file->rdwr_longlong(cst_maintain_transformer );
			if ((file->get_version() > 120002 && (file->get_extended_revision() == 0 || file->get_extended_revision() >= 16)) || file->get_extended_version() >= 13)
			{
				file->rdwr_longlong(cst_make_public_months);
			}
			// wayfinder
			file->rdwr_long(way_count_straight );
			file->rdwr_long(way_count_curve );
			file->rdwr_long(way_count_double_curve );
			file->rdwr_long(way_count_90_curve );
			file->rdwr_long(way_count_slope );
			file->rdwr_long(way_count_tunnel );
			file->rdwr_long(way_max_bridge_len );
			file->rdwr_long(way_count_leaving_road );
		}
		else {
			// name of stops
			set_name_language_iso( env_t::language_iso );

			// default AIs active
			for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				if(  i<2  ) {
					player_type[i] = player_t::HUMAN;
				}
				else if(  i==3  ) {
					player_type[i] = player_t::AI_PASSENGER;
				}
				else if(  i<8  ) {
					player_type[i] = player_t::AI_GOODS;
				}
				else {
					player_type[i] = player_t::EMPTY;
				}
				player_active[i] = false;
			}
		}

		if(file->get_version()>101000) {
			file->rdwr_bool( separate_halt_capacities);
			if(file->get_extended_version() < 2)
			{
				// Was pay for total distance.
				// Now depracated.
				uint8 dummy;
				file->rdwr_byte( dummy);
			}

			file->rdwr_short(starting_month );

			file->rdwr_short( river_number );
			file->rdwr_short( min_river_length );
			file->rdwr_short( max_river_length );
		}

		if(file->get_version()>102000) {
			file->rdwr_bool(avoid_overcrowding);
		}

		if(file->get_version()>102001)
		{
			bool dummy = false;
			file->rdwr_bool(dummy);
			file->rdwr_bool(with_private_paks);
		}

		if(file->get_version()>=102003) {
			// network stuff
			random_counter = get_random_seed( );
			file->rdwr_long( random_counter );
			if(  !env_t::networkmode  ||  env_t::server  ) {
				frames_per_second = clamp(env_t::fps,5,100 );	// update it on the server to the current setting
				frames_per_step = env_t::network_frames_per_step;
			}
			file->rdwr_long( frames_per_second );
			file->rdwr_long( frames_per_step );
			if(  !env_t::networkmode  ||  env_t::server  ) {
				frames_per_second = env_t::fps;	// update it on the server to the current setting
				frames_per_step = env_t::network_frames_per_step;
			}
			if(file->get_extended_version() >= 14 && file->get_extended_revision() > 1)
			{
				file->rdwr_byte( allow_buying_obsolete_vehicles);
			} else {
				bool compat = allow_buying_obsolete_vehicles > 0;
				file->rdwr_bool( compat );
				allow_buying_obsolete_vehicles = 0;
				if (compat) {
					allow_buying_obsolete_vehicles = 1;
				}
			}
			if(file->get_extended_version() < 12 && (file->get_extended_version() >= 8 || file->get_extended_version() == 0))
			{
				// Was factory_worker_minimum_towns and factory_worker_maximum_towns
				uint32 dummy;
				file->rdwr_long(dummy);
				file->rdwr_long(dummy);
			}
			if(file->get_extended_version() >= 9 || file->get_extended_version() == 0)
			{
				// forest stuff
				file->rdwr_byte( forest_base_size);
				file->rdwr_byte( forest_map_size_divisor);
				file->rdwr_byte( forest_count_divisor);
				file->rdwr_short( forest_inverse_spare_tree_density);
				file->rdwr_byte( max_no_of_trees_on_square);
				file->rdwr_short( tree_climates);
				file->rdwr_short( no_tree_climates);
				file->rdwr_bool( no_trees);
				if(file->get_extended_version() < 9)
				{
					sint32 dummy = 0;
					file->rdwr_long(dummy); // Was "minimum city distance"
				}
				file->rdwr_long( industry_increase );
			}
			if(file->get_extended_version() >= 9)
			{
				file->rdwr_long( city_isolation_factor );
			}
		}

		if(file->get_extended_version() >= 1)
		{
			uint16 dummy = 0;

			if (file->get_extended_version() < 13 && file->get_extended_revision() < 24)
			{
				// Former speed bonus settings, now deprecated.
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
			}

			if(file->get_extended_version() == 1)
			{
				uint16 dummy;
				file->rdwr_short(dummy);
			}
			else
			{
				if (file->get_extended_version() < 13 && file->get_extended_revision() < 24)
				{
					// Former speed bonus settings, now deprecated.
					file->rdwr_short(dummy);
					file->rdwr_short(dummy);
				}

				if(file->get_extended_version() <= 9)
				{
					uint16 distance_per_tile_integer = meters_per_tile / 10;
					file->rdwr_short(distance_per_tile_integer);
					if(file->get_extended_version() < 5 && file->get_extended_version() >= 1)
					{
						// In earlier versions, the default was set to a higher level. This
						// is a problem when the new journey time tolerance features is used.
						if(file->is_loading())
						{
							distance_per_tile_integer = (distance_per_tile_integer * 8) / 10;
						}
						else
						{
							distance_per_tile_integer = (distance_per_tile_integer * 10) / 8;
						}
					}
					set_meters_per_tile(distance_per_tile_integer * 10);
				}
				else
				{
					// Version 10.0 and above - meters per tile stored precisely
					uint16 mpt = meters_per_tile;
					file->rdwr_short(mpt);
					set_meters_per_tile(mpt);
				}

				file->rdwr_byte(tolerable_comfort_short);
				file->rdwr_byte(tolerable_comfort_median_short);
				file->rdwr_byte(tolerable_comfort_median_median);
				file->rdwr_byte(tolerable_comfort_median_long);
				file->rdwr_byte(tolerable_comfort_long);

				file->rdwr_short(tolerable_comfort_short_minutes);
				file->rdwr_short(tolerable_comfort_median_short_minutes);
				file->rdwr_short(tolerable_comfort_median_median_minutes);
				file->rdwr_short(tolerable_comfort_median_long_minutes);
				file->rdwr_short(tolerable_comfort_long_minutes);

				file->rdwr_byte(max_luxury_bonus_differential);
				file->rdwr_byte(max_discomfort_penalty_differential);
				file->rdwr_short(max_discomfort_penalty_percent);
				file->rdwr_short(max_luxury_bonus_percent);

				file->rdwr_short(catering_min_minutes);
				file->rdwr_short(catering_level1_minutes);
				file->rdwr_short(catering_level1_max_revenue);
				file->rdwr_short(catering_level2_minutes);
				file->rdwr_short(catering_level2_max_revenue);
				file->rdwr_short(catering_level3_minutes);
				file->rdwr_short(catering_level3_max_revenue);
				file->rdwr_short(catering_level4_minutes);
				file->rdwr_short(catering_level4_max_revenue);
				file->rdwr_short(catering_level5_minutes);
				file->rdwr_short(catering_level5_max_revenue);

				file->rdwr_short(tpo_min_minutes);
				file->rdwr_short(tpo_revenue);

				if ( file->is_loading() ) {
					cache_comfort_tables();
					cache_catering_revenues();
				}
			}

			float32e8_t distance_per_tile(meters_per_tile, 1000);

			if(file->get_extended_version() < 6)
			{
				// Scale the costs to match the scale factor.
				// Note that this will fail for attempts to save in the old format.
				cst_multiply_dock *= distance_per_tile;
				cst_multiply_station *= distance_per_tile;
				cst_multiply_roadstop *= distance_per_tile;
				cst_multiply_airterminal *= distance_per_tile;
				cst_multiply_post *= distance_per_tile;
				maint_building *= distance_per_tile;
				cst_buy_land *= distance_per_tile;
				cst_remove_tree *= distance_per_tile;
			}

			file->rdwr_short(obsolete_running_cost_increase_percent);
			file->rdwr_short(obsolete_running_cost_increase_phase_years);

			if(file->get_extended_version() >= 9)
			{
				if (file->get_extended_version() < 12)
				{
					// Was formerly passenger distance ranges, now deprecated.
					uint32 dummy = 0;
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
				}
			}
			else
			{
				uint16 dummy = 0;
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
			}

			file->rdwr_byte(passenger_routing_packet_size);
			if(file->get_extended_version() >= 12)
			{
				file->rdwr_short(max_alternative_destinations_commuting);
				file->rdwr_short(max_alternative_destinations_visiting);


				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 25)
				{
					file->rdwr_short(min_alternative_destinations_visiting);
					file->rdwr_short(min_alternative_destinations_commuting);

					file->rdwr_long(min_alternative_destinations_per_visitor_demand_millionths);
					file->rdwr_long(min_alternative_destinations_per_job_millionths);
				}
				else
				{
					// In earlier versions, the maxima for alternative destinations were stored without
					// adjustment for the minima. Subtract the minima here as they are added back
					// after the random number is generated.

					max_alternative_destinations_visiting -= min_alternative_destinations_visiting;
					max_alternative_destinations_commuting -= min_alternative_destinations_commuting;
				}

				if (max_alternative_destinations_commuting + min_alternative_destinations_commuting == 0)
				{
					max_alternative_destinations_commuting = 1;
				}
				if (max_alternative_destinations_visiting + min_alternative_destinations_visiting == 0)
				{
					max_alternative_destinations_visiting = 1;
				}

				file->rdwr_long(max_alternative_destinations_per_job_millionths);
				file->rdwr_long(max_alternative_destinations_per_visitor_demand_millionths);

				// These are necessary to prevent underflows when the actual maxima are calculated
				if (max_alternative_destinations_per_job_millionths < min_alternative_destinations_per_job_millionths)
				{
					max_alternative_destinations_per_job_millionths = min_alternative_destinations_per_job_millionths * 2;
				}
				if (max_alternative_destinations_per_visitor_demand_millionths < min_alternative_destinations_per_visitor_demand_millionths)
				{
					max_alternative_destinations_per_visitor_demand_millionths = min_alternative_destinations_per_visitor_demand_millionths * 2;
				}
			}
			else
			{
				uint8 eight_bit_alternative_destinations = max_alternative_destinations_visiting > 255 ? 255 : max_alternative_destinations_visiting;
				file->rdwr_byte(eight_bit_alternative_destinations);
				if(file->is_loading())
				{
					max_alternative_destinations_visiting = max_alternative_destinations_commuting = (uint16)eight_bit_alternative_destinations;
				}
			}
			if(file->get_extended_version() < 12)
			{
				// Was passenger_routing_local_chance and passenger_routing_midrange_chance
				uint8 dummy;
				file->rdwr_byte(dummy);
				file->rdwr_byte(dummy);
			}
			if(file->get_extended_version() < 11)
			{
				// Was base_car_preference_percent
				uint8 dummy = 0;
				file->rdwr_byte(dummy);
			}
			file->rdwr_byte(always_prefer_car_percent);
			file->rdwr_byte(congestion_density_factor);

			waytype_t wt = road_wt;
			for(int n = 1; n < 7; n ++)
			{
				switch(n)
				{
				case 1:
					wt = road_wt;
					break;
				case 2:
					wt = track_wt;
					break;
				case 3:
					wt = tram_wt;
					break;
				case 4:
					wt = monorail_wt;
					break;
				case 5:
					wt = maglev_wt;
					break;
				case 6:
					wt = narrowgauge_wt;
					break;
				default:
					dbg->fatal("settings_t::rdwr", "Invalid waytype");
				}

				if(file->get_extended_version() >= 12)
				{
#ifdef SPECIAL_RESCUE_12_4
					if(file->is_saving())
					{
#endif
						file->rdwr_long(corner_force_divider[waytype_t(wt)]);
#ifdef SPECIAL_RESCUE_12_4
					}
#endif
				}
#ifdef SPECIAL_RESCUE_12_4
				if(file->is_loading())
#else
				else
#endif
				{
					// Former cornering settings. Were numerous.
					sint32 dummy = 0;
					sint16 dummy_short = 0;
					sint8 dummy_byte = 0;
					file->rdwr_long(dummy);
					file->rdwr_long(dummy);
					if(file->get_extended_version() < 10)
					{
						double dummy_double;
						file->rdwr_double(dummy_double);
						file->rdwr_double(dummy_double);
					}
					else
					{
						file->rdwr_short(dummy_short);
						file->rdwr_short(dummy_short);
					}
					file->rdwr_byte(dummy_byte);
					file->rdwr_byte(dummy_byte);
				}
				if(file->get_extended_version() >= 10)
				{
					file->rdwr_byte(curve_friction_factor[waytype_t(wt)]);
				}
			}
#ifdef SPECIAL_RESCUE_12_4
			if(file->get_extended_version() >= 12 && file->is_saving())
#else
			if(file->get_extended_version() >= 12)
#endif
			{
				file->rdwr_long(tilting_min_radius_effect);
			}

			file->rdwr_short(factory_max_years_obsolete);

			file->rdwr_byte(interest_rate_percent);
			file->rdwr_bool(allow_insolvency);
			file->rdwr_bool(allow_purchases_when_insolvent);

			if(file->get_extended_version() >= 11)
			{
				file->rdwr_long(unit_reverse_time);
				file->rdwr_long(hauled_reverse_time);
				file->rdwr_long(turntable_reverse_time);
				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 19)
				{
					file->rdwr_long(road_reverse_time);
				}

				file->rdwr_short(unit_reverse_time_seconds);
				file->rdwr_short(hauled_reverse_time_seconds);
				file->rdwr_short(turntable_reverse_time_seconds);
				if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 19)
				{
					file->rdwr_short(road_reverse_time_seconds);
				}
			}
			else
			{
				if(env_t::networkmode)
				{
					if(unit_reverse_time > 65535)
					{
						unit_reverse_time = 65535;
					}
					if(hauled_reverse_time > 65535)
					{
						hauled_reverse_time = 65535;
					}
					if(turntable_reverse_time > 65535)
					{
						turntable_reverse_time = 65535;
					}
					if (road_reverse_time > 65535)
					{
						road_reverse_time = 65535;
					}

					road_reverse_time_seconds = turntable_reverse_time_seconds = hauled_reverse_time_seconds = unit_reverse_time_seconds = 65535;
				}
				uint16 short_unit_reverse_time = unit_reverse_time < 65535 ? unit_reverse_time : 65535;
				uint16 short_hauled_reverse_time = hauled_reverse_time < 65535 ? hauled_reverse_time : 65535;
				uint16 short_turntable_reverse_time = turntable_reverse_time < 65535 ? turntable_reverse_time : 65535;

				file->rdwr_short(short_unit_reverse_time);
				file->rdwr_short(short_hauled_reverse_time);
				file->rdwr_short(short_turntable_reverse_time);

				unit_reverse_time = short_unit_reverse_time;
				hauled_reverse_time = short_hauled_reverse_time;
				turntable_reverse_time = short_turntable_reverse_time;
			}
		}

		if(file->get_extended_version() >= 2)
		{
			file->rdwr_short(global_power_factor_percent);
			if(file->get_extended_version() <= 7)
			{
				uint16 old_passenger_max_wait;
				file->rdwr_short(old_passenger_max_wait);
				passenger_max_wait = (uint32)old_passenger_max_wait;
			}
			else
			{
				file->rdwr_long(passenger_max_wait);
			}
			file->rdwr_byte(max_rerouting_interval_months);
		}

		if(file->get_extended_version() >= 3)
		{
			if(file->get_extended_version() < 7)
			{
				// Was city weight factor. Now replaced by a more
				// sophisticated customisable city growth from Standard.
				uint16 dummy;
				file->rdwr_short(dummy);
			}
			file->rdwr_byte(enforce_weight_limits);

			if (file->get_extended_version() < 13 || file->get_extended_revision() < 2)
			{
				// Former speed bonus settings, now deprecated.
				uint16 dummy_2 = 0;
				file->rdwr_short(dummy_2);
			}
		}
		else
		{
			// For older saved games, enforcing weight limits strictly is likely to lead
			// to problems, so, whilst strictly enforced weight limits should be allowed
			// for new games and games saved with this feature enabled, it should not be
			// allowed for older saved games.
			enforce_weight_limits = enforce_weight_limits == 0 ? 0 : 1;
		}

		if(file->get_extended_version() >= 4 && file->get_extended_version() < 10)
		{
			uint8 dummy;
			file->rdwr_byte(dummy);
		}

		if(file->get_extended_version() >= 5)
		{
			if((file->get_extended_version() >= 14 && file->get_extended_revision() >= 1) || file->get_extended_version() >= 15)
			{
				file->rdwr_long(min_visiting_tolerance);
				file->rdwr_long(range_commuting_tolerance);
				file->rdwr_long(min_commuting_tolerance);
				file->rdwr_long(range_visiting_tolerance);
			}
			else
			{
				uint16 temp = (uint16)min_visiting_tolerance;
				file->rdwr_short(temp);
				min_visiting_tolerance = temp;

				temp = (uint16)range_commuting_tolerance;
				file->rdwr_short(temp);
				range_commuting_tolerance = temp;

				temp = (uint16)min_commuting_tolerance;
				file->rdwr_short(temp);
				min_commuting_tolerance = temp;

				temp = (uint16)range_visiting_tolerance;
				file->rdwr_short(temp);
				range_visiting_tolerance = temp;
			}

			if(file->get_extended_version() < 12)
			{
				// Was min_longdistance_tolerance and max_longdistance_tolerance
				uint16 dummy;
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
			}
		}

		if(  file->get_version()>=110000  ) {
			if(  !env_t::networkmode  ||  env_t::server  ) {
				server_frames_ahead = env_t::server_frames_ahead;
			}
			file->rdwr_long( server_frames_ahead );
			if(  !env_t::networkmode  ||  env_t::server  ) {
				server_frames_ahead = env_t::server_frames_ahead;
			}
			file->rdwr_short( used_vehicle_reduction );
		}

		if(file->get_extended_version() >= 8)
		{
			if(file->get_extended_version() < 11)
			{
				uint16 dummy = 0;
				file->rdwr_short(dummy);
				// Was max_walking_distance
			}
			file->rdwr_bool(quick_city_growth);
			file->rdwr_bool(assume_everywhere_connected_by_road);
			for(uint8 i = 0; i < 17; i ++)
			{
				file->rdwr_short(default_increase_maintenance_after_years[i]);
			}
			file->rdwr_long(capital_threshold_size);
			file->rdwr_long(city_threshold_size);
			if (file->get_extended_version() >= 15 || (file->get_extended_version() >= 14 && file->get_extended_revision() >= 15))
			{
				file->rdwr_byte(capital_threshold_percentage);
				file->rdwr_byte(city_threshold_percentage);
			}
		}

		if(  file->get_version()>=110001  ) {
			file->rdwr_bool( default_player_color_random );
			for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				file->rdwr_byte( default_player_color[i][0] );
				file->rdwr_byte( default_player_color[i][1] );
			}
		}
		else if(  file->is_loading()  ) {
			default_player_color_random = false;
			for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				// default colors for player ...
				default_player_color[i][0] = 255;
				default_player_color[i][1] = 255;
			}
		}

		if(  file->get_version()>=110005  ) {
			file->rdwr_short(factory_arrival_periods);
			file->rdwr_bool(factory_enforce_demand);
		}

		if(  file->get_version()>=110007  )
		{
			if(file->get_extended_version() == 0 )
			{
				// Unfortunately, with this new system from Standard, it is no longer possible
				// to parse these values in a way that makes sense to Extended. This must
				// be maintained to retain saved game compatibility only.
				uint32 dummy_32 = 0;
				uint16 dummy_16 = 0;
				for(  int i=0;  i<10;  i++  )
				{
					file->rdwr_short(dummy_16);
					file->rdwr_long(dummy_32);
				}
			}
		}

		if(file->get_extended_version() >= 10 || (file->get_extended_version() == 0 && file->get_version() >= 110007))
		{
			file->rdwr_bool( drive_on_left );
			file->rdwr_bool( signals_on_left );
		}

		if(file->get_version() >= 110007)
		{
			file->rdwr_long( way_toll_runningcost_percentage );
			file->rdwr_long( way_toll_waycost_percentage );
			if(file->get_extended_version() >= 10)
			{
				file->rdwr_long(way_toll_revenue_percentage);
				file->rdwr_long(seaport_toll_revenue_percentage);
				file->rdwr_long(airport_toll_revenue_percentage);
			}
		}

		if (file->get_extended_version() >= 9 && file->get_version() >= 110006)
		{
			file->rdwr_byte(spacing_shift_mode);
			file->rdwr_short(spacing_shift_divisor);

			uint16 livery_schemes_count = 0;
			if(file->is_loading())
			{
				livery_schemes.clear();
			}
			if(file->is_saving())
			{
				livery_schemes_count = livery_schemes.get_count();
			}

			file->rdwr_short(livery_schemes_count);

			if(strcmp(file->get_pak_extension(), "settings only"))
			{
				for(int i = 0; i < livery_schemes_count; i ++)
				{
					if(file->is_saving())
					{
						livery_schemes[i]->rdwr(file);
					}
					else
					{
						livery_scheme_t* scheme = new livery_scheme_t("default", DEFAULT_RETIRE_DATE);
						scheme->rdwr(file);
						livery_schemes.append(scheme);
					}
				}
			}
		}

		if(file->get_extended_version() >= 10)
		{
			file->rdwr_bool(allow_routing_on_foot);
			file->rdwr_short(min_wait_airport);
			if(file->get_version() >= 110007)
			{
				file->rdwr_bool(toll_free_public_roads);
			}
			if(file->get_version() >= 111000)
			{
				file->rdwr_bool(allow_making_public);
			}
		}

		if(file->get_extended_version() >= 11)
		{
			file->rdwr_longlong(private_car_toll_per_km);
			file->rdwr_bool(towns_adopt_player_roads);
			file->rdwr_long(reroute_check_interval_steps);
			file->rdwr_byte(walking_speed);
		}
		else if(env_t::networkmode)
		{
			// This is necessary to prevent desyncs.
			private_car_toll_per_km = 1;
			towns_adopt_player_roads = true;
			reroute_check_interval_steps = 8192;
			walking_speed = 5;
		}

		if(file->get_version()>=111002 && file->get_extended_version() == 0)
		{
			// Was bonus_basefactor
			uint32 dummy = 0;
			file->rdwr_long(dummy);
		}

		if(file->get_extended_version() >= 10 && file->get_version() >= 111002)
		{
			file->rdwr_long(max_small_city_size);
			file->rdwr_long(max_city_size);
			file->rdwr_byte(max_elevated_way_building_level);
			file->rdwr_bool(allow_airports_without_control_towers);
		}

		if(  file->get_version()>=111004  ) {
			file->rdwr_bool( allow_underground_transformers );
		}

		if(  file->get_version()>=111005  ) {
			file->rdwr_short( special_building_distance );
		}

		if(  file->get_version()>=112001  ) {
			file->rdwr_short( factory_maximum_intransit_percentage );
		}

		if(  file->get_version()>=112002  ) {
			file->rdwr_short( remove_dummy_player_months );
			file->rdwr_short( unprotect_abandoned_player_months );
		}

		if(  file->get_version()>=112003  ) {
			file->rdwr_short( max_factory_spacing );
			file->rdwr_short( max_factory_spacing_percentage );
		}
		if(  file->get_version()>=112008  ) {
			file->rdwr_longlong( cst_alter_climate );
			file->rdwr_byte( way_height_clearance );
		}
		if(  file->get_version()>=120002 && file->get_extended_version() == 0 ) {
			file->rdwr_long( default_ai_construction_speed );
			// This feature is used in Standard only
		}
		else if(  file->is_loading()  ) {
			default_ai_construction_speed = env_t::default_ai_construction_speed;
		}
		if(  file->get_version() >=120002 && (file->get_extended_revision() >= 9 || file->get_extended_version() == 0 || file->get_extended_version() >= 13)) {
			file->rdwr_bool(lake);
			file->rdwr_bool(no_trees);
			file->rdwr_long(max_choose_route_steps );
		// otherwise the default values of the last one will be used
		}

		if ((file->get_version() > 120003 && (file->get_extended_version() == 0 || file->get_extended_revision() >= 19)) || file->get_extended_version() >= 13)
		{
			file->rdwr_bool(disable_make_way_public);
		}

		if(file->get_extended_version() >= 12)
		{
			file->rdwr_short(population_per_level);
			file->rdwr_short(visitor_demand_per_level);
			file->rdwr_short(jobs_per_level);
			file->rdwr_short(mail_per_level);
			file->rdwr_long(passenger_trips_per_month_hundredths);
			file->rdwr_long(mail_packets_per_month_hundredths);
			file->rdwr_short(max_onward_trips);
			file->rdwr_short(onward_trip_chance_percent);
			file->rdwr_short(commuting_trip_chance_percent);
			file->rdwr_long(base_meters_per_tile);
			file->rdwr_long(base_bits_per_month);
			file->rdwr_long(job_replenishment_per_hundredths_of_months);
			file->rdwr_long(random_mode_commuting);
			file->rdwr_long(random_mode_visiting);

			file->rdwr_longlong(forge_cost_road);
			file->rdwr_longlong(forge_cost_track);
			file->rdwr_longlong(forge_cost_water);
			file->rdwr_longlong(forge_cost_monorail);
			file->rdwr_longlong(forge_cost_maglev);
			file->rdwr_longlong(forge_cost_tram);
			file->rdwr_longlong(forge_cost_narrowgauge);
			file->rdwr_longlong(forge_cost_air);

			file->rdwr_short(parallel_ways_forge_cost_percentage_road);
			file->rdwr_short(parallel_ways_forge_cost_percentage_track);
			file->rdwr_short(parallel_ways_forge_cost_percentage_water);
			file->rdwr_short(parallel_ways_forge_cost_percentage_monorail);
			file->rdwr_short(parallel_ways_forge_cost_percentage_maglev);
			file->rdwr_short(parallel_ways_forge_cost_percentage_tram);
			file->rdwr_short(parallel_ways_forge_cost_percentage_narrowgauge);
			file->rdwr_short(parallel_ways_forge_cost_percentage_air);

			file->rdwr_long(max_diversion_tiles);
#ifdef SPECIAL_RESCUE_12_3
			if(file->is_saving())
			{
				file->rdwr_long(way_degradation_fraction);
				file->rdwr_long(way_wear_power_factor_road_type);
				file->rdwr_long(way_wear_power_factor_rail_type);
				file->rdwr_short(standard_axle_load);
				file->rdwr_long(citycar_way_wear_factor);
			}
#else
			file->rdwr_long(way_degradation_fraction);
			file->rdwr_long(way_wear_power_factor_road_type);
			file->rdwr_long(way_wear_power_factor_rail_type);
			file->rdwr_short(standard_axle_load);
			file->rdwr_long(citycar_way_wear_factor);
			if(file->get_extended_revision() >= 2 || file->get_extended_version() >= 13)
			{
				file->rdwr_long(sighting_distance_meters);
				sighting_distance_tiles = sighting_distance_meters / meters_per_tile;
				file->rdwr_long(assumed_curve_radius_45_degrees);
			}
			if(file->get_extended_revision() >= 3 || file->get_extended_version() >= 13)
			{
				file->rdwr_long(max_speed_drive_by_sight_kmh);
				max_speed_drive_by_sight = kmh_to_speed(max_speed_drive_by_sight_kmh);
			}
#endif
			if(file->get_extended_revision() >= 5 || file->get_extended_version() >= 13)
			{
				file->rdwr_short(global_force_factor_percent);
			}

			if(file->get_extended_revision() >= 6 || file->get_extended_version() >= 13)
			{
				file->rdwr_long(time_interval_seconds_to_clear);
				file->rdwr_long(time_interval_seconds_to_caution);
			}
			else
			{
				time_interval_seconds_to_clear = 600;
				time_interval_seconds_to_caution = 300;
			}
			if(file->get_extended_revision() >= 7 || file->get_extended_version() >= 13)
			{
				file->rdwr_long(town_road_speed_limit);
			}
			else
			{
				town_road_speed_limit = 50;
			}
		}
		else
		{
			// Calibrate old saved games with reference to their original passenger factor.
			passenger_trips_per_month_hundredths = (passenger_trips_per_month_hundredths * 16) / old_passenger_factor;
			mail_packets_per_month_hundredths = (mail_packets_per_month_hundredths * 16) / old_passenger_factor;
			way_degradation_fraction = 7;
		}

		if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 23)
		{
			file->rdwr_long(minimum_staffing_percentage_consumer_industry);
			file->rdwr_long(minimum_staffing_percentage_full_production_producer_industry);
		}
		else
		{
			minimum_staffing_percentage_consumer_industry = 66;
			minimum_staffing_percentage_full_production_producer_industry = 80;
		}

		if (file->get_extended_version() >= 13 || (file->get_extended_version() == 12 && file->get_extended_revision() >= 27))
		{
			file->rdwr_short(max_comfort_preference_percentage);
		}
		else
		{
			max_comfort_preference_percentage = 500;
		}

		if (file->get_extended_version() >= 13 && file->get_extended_revision() >= 3)
		{
			file->rdwr_bool(rural_industries_no_staff_shortage);
		}
		else if (file->is_loading())
		{
			rural_industries_no_staff_shortage = true;
		}

		if (file->get_extended_version() >= 14 && file->get_extended_revision() >= 16)
		{
			file->rdwr_long(auto_connect_industries_and_attractions_by_road);
		}
		else
		{
			auto_connect_industries_and_attractions_by_road = 4;
		}

		if (file->get_extended_version() >= 13 && file->get_extended_revision() >= 4)
		{
			file->rdwr_long(power_revenue_factor_percentage);
		}

		if (file->get_extended_version() >= 15 || (file->get_extended_version() >= 14 && file->get_extended_revision() >= 8))
		{
			file->rdwr_long(path_explorer_time_midpoint);
			file->rdwr_bool(save_path_explorer_data);
		}

		if (file->get_extended_version() >= 15 || (file->get_extended_version() >= 14 && file->get_extended_revision() >= 18))
		{
			file->rdwr_bool(show_future_vehicle_info);
		}

		if (file->get_extended_version() >= 15 || (file->get_extended_version() == 14 && file->get_extended_revision() >= 19))
		{
			if (file->get_extended_version() == 14 && file->get_extended_revision() < 20)
			{
				uint32 old_route_tiles = max(1, max_route_tiles_to_process_in_a_step / 256);
				file->rdwr_long(old_route_tiles);
				max_route_tiles_to_process_in_a_step = old_route_tiles * 256;
			}
			else
			{
				file->rdwr_long(max_route_tiles_to_process_in_a_step);
			}
		}

		if (file->get_extended_version() >= 15 || (file->get_extended_version() == 14 && file->get_extended_revision() >= 25))
		{
			file->rdwr_long(tolerance_modifier_percentage);
		}
		else if (file->is_loading())
		{
			tolerance_modifier_percentage = 100;
		}

		if (file->get_extended_version() >= 15 || (file->get_extended_version() == 14 && file->get_extended_revision() >= 27))
		{
			file->rdwr_bool(absolute_regions);
			if (file->is_saving())
			{
				uint32 count = regions.get_count();
				file->rdwr_long(count);

				FOR(vector_tpl<region_definition_t>, region, regions)
				{
					region.top_left.rdwr(file);
					region.bottom_right.rdwr(file);
					file->rdwr_string(region.name);
				}
			}
			else // Loading
			{
				uint32 count = 0;
				file->rdwr_long(count);

				regions.clear();
				for (uint32 i = 0; i < count; i++)
				{
					region_definition_t r;
					r.top_left.rdwr(file);
					r.bottom_right.rdwr(file);
					file->rdwr_string(r.name);
					regions.append(r);
				}
			}
		}

		if (file->get_extended_version() >= 15 || file->get_extended_version() == 14 && file->get_extended_revision() >= 30)
		{
			if (!env_t::server)
			{
				file->rdwr_long(industry_density_proportion_override);
			}
			else
			{
				uint32 idpo = industry_density_proportion_override;
				file->rdwr_long(idpo); // Do not load this value if running a server - allow this to be an override whatever the general override settings.
			}
		}
		else if (file->is_loading())
		{
			if (!env_t::server)
			{
				industry_density_proportion_override = 0;
			}
		}
	}



#ifdef DEBUG_SIMRAND_CALLS
	char buf[256];
	sprintf(buf,"Initial counter: %i; seed: %i", get_random_counter(), get_random_seed());
	dbg->message("settings_t::rdwr", buf);
#endif
}

// read the settings from this file
void settings_t::parse_simuconf(tabfile_t& simuconf, sint16& disp_width, sint16& disp_height, sint16 &fullscreen, std::string& objfilename)
{
	tabfileobj_t contents;

	simuconf.read(contents);

	// Meta-options.
	// Only the version in default_einstellungen is meaningful.  These determine whether savegames
	// are updated to the newest local settings.  They are ignored for clients in network games.
	// This is read many many times so always use the older version...
	// @author: neroden.
	progdir_overrides_savegame_settings = (contents.get_int("progdir_overrides_savegame_settings", progdir_overrides_savegame_settings) != 0);
	pak_overrides_savegame_settings = (contents.get_int("pak_overrides_savegame_settings", pak_overrides_savegame_settings) != 0);
	userdir_overrides_savegame_settings = (contents.get_int("userdir_overrides_savegame_settings", userdir_overrides_savegame_settings) != 0);

	// This needs to be first as other settings are based on this.
	// @author: jamespetts, neroden

	// This compatibility code is more complicated than it appears due to (a) roundoff error,
	// and (b) the fact that simuconf tabfiles are read *multiple times*.
	// First of all, 0 is clearly an invalid value, so use it as a flag.
	uint16 new_dpt = contents.get_int("distance_per_tile", 0);
	uint16 new_mpt = contents.get_int("meters_per_tile", 0);
	if (new_mpt) {
		set_meters_per_tile(new_mpt);
	} else if (new_dpt) {
		set_meters_per_tile(new_dpt * 10);
	} else {
		// Don't set it, leave it at the previous setting from a previous simuconf.tab, save file, etc
	}
	float32e8_t distance_per_tile(meters_per_tile, 1000);

	base_meters_per_tile = contents.get_int("base_meters_per_tile", base_meters_per_tile);
	base_bits_per_month = contents.get_int("base_bits_per_month", base_bits_per_month);
	job_replenishment_per_hundredths_of_months = contents.get_int("job_replenishment_per_hundredths_of_months", job_replenishment_per_hundredths_of_months);
	calc_job_replenishment_ticks();

		// special day/night colors
#if COLOUR_DEPTH != 0
	// special day/night colors
	for(  int i=0;  i<LIGHT_COUNT;  i++  ) {
		char str[256];
		sprintf( str, "special_color[%i]", i );
		int *c = contents.get_ints( str );
		if(  c[0]>=6  ) {
			// now update RGB values
			for(  int j=0;  j<3;  j++  ) {
				display_day_lights[i*3+j] = c[j+1];
			}
			for(  int j=0;  j<3;  j++  ) {
				display_night_lights[i*3+j] = c[j+4];
			}
		}
		delete [] c;
	}
#endif

	env_t::water_animation = contents.get_int("water_animation_ms", env_t::water_animation);
	env_t::ground_object_probability = contents.get_int("random_grounds_probability", env_t::ground_object_probability);
	env_t::moving_object_probability = contents.get_int("random_wildlife_probability", env_t::moving_object_probability);

	env_t::straight_way_without_control = contents.get_int("straight_way_without_control", env_t::straight_way_without_control) != 0;

	env_t::road_user_info = contents.get_int("pedes_and_car_info", env_t::road_user_info) != 0;
	env_t::tree_info = contents.get_int("tree_info", env_t::tree_info) != 0;
	env_t::ground_info = contents.get_int("ground_info", env_t::ground_info) != 0;
	env_t::townhall_info = contents.get_int("townhall_info", env_t::townhall_info) != 0;
	env_t::single_info = contents.get_int("only_single_info", env_t::single_info );

	env_t::compass_map_position = contents.get_int("compass_map_position", env_t::compass_map_position );
	env_t::compass_screen_position = contents.get_int("compass_screen_position", env_t::compass_screen_position );

	env_t::window_snap_distance = contents.get_int("window_snap_distance", env_t::window_snap_distance );
	env_t::window_buttons_right = contents.get_int("window_buttons_right", env_t::window_buttons_right );
	env_t::left_to_right_graphs = contents.get_int("left_to_right_graphs", env_t::left_to_right_graphs );
	env_t::window_frame_active = contents.get_int("window_frame_active", env_t::window_frame_active );

	env_t::second_open_closes_win = contents.get_int("second_open_closes_win", env_t::second_open_closes_win );
	env_t::remember_window_positions = contents.get_int("remember_window_positions", env_t::remember_window_positions );

	env_t::front_window_bar_color = contents.get_int("front_window_bar_color", env_t::front_window_bar_color );
	env_t::front_window_text_color = contents.get_int("front_window_text_color", env_t::front_window_text_color );
	env_t::bottom_window_bar_color = contents.get_int("bottom_window_bar_color", env_t::bottom_window_bar_color );
	env_t::bottom_window_text_color = contents.get_int("bottom_window_text_color", env_t::bottom_window_text_color );

	env_t::show_tooltips = contents.get_int("show_tooltips", env_t::show_tooltips );
	env_t::tooltip_color = contents.get_int("tooltip_background_color", env_t::tooltip_color );
	env_t::tooltip_textcolor = contents.get_int("tooltip_text_color", env_t::tooltip_textcolor );
	env_t::tooltip_delay = contents.get_int("tooltip_delay", env_t::tooltip_delay );
	env_t::tooltip_duration = contents.get_int("tooltip_duration", env_t::tooltip_duration );
	env_t::toolbar_max_width = contents.get_int("toolbar_max_width", env_t::toolbar_max_width );
	env_t::toolbar_max_height = contents.get_int("toolbar_max_height", env_t::toolbar_max_height );
	env_t::cursor_overlay_color = contents.get_int("cursor_overlay_color", env_t::cursor_overlay_color );

	// how to show the stuff outside the map
	env_t::background_color = contents.get_int("background_color", env_t::background_color );
	env_t::draw_earth_border = contents.get_int("draw_earth_border", env_t::draw_earth_border ) != 0;
	env_t::draw_outside_tile = contents.get_int("draw_outside_tile", env_t::draw_outside_tile ) != 0;

	// display stuff
	env_t::show_names = contents.get_int("show_names", env_t::show_names );
	env_t::show_month = contents.get_int("show_month", env_t::show_month );
	env_t::max_acceleration = contents.get_int("fast_forward", env_t::max_acceleration );
	env_t::fps = contents.get_int("frames_per_second",env_t::fps );
	env_t::num_threads = clamp( contents.get_int("threads", env_t::num_threads ), 1, MAX_THREADS );
	env_t::simple_drawing_default = contents.get_int("simple_drawing_tile_size",env_t::simple_drawing_default );
	env_t::simple_drawing_fast_forward = contents.get_int("simple_drawing_fast_forward",env_t::simple_drawing_fast_forward );
	env_t::visualize_schedule = contents.get_int("visualize_schedule",env_t::visualize_schedule ) != 0;
	env_t::show_vehicle_states = contents.get_int("show_vehicle_states",env_t::show_vehicle_states );

	env_t::show_delete_buttons = contents.get_int("show_delete_buttons",env_t::show_delete_buttons ) != 0;
	env_t::chat_window_transparency = contents.get_int("chat_transparency",env_t::chat_window_transparency );

	env_t::hide_keyboard = contents.get_int("hide_keyboard",env_t::hide_keyboard ) != 0;

	env_t::player_finance_display_account = contents.get_int("player_finance_display_account",env_t::player_finance_display_account ) != 0;

	int* offsets[8];
	offsets[0] = contents.get_ints("reverse_base_offset_south");
	offsets[1] = contents.get_ints("reverse_base_offset_west");
	offsets[2] = contents.get_ints("reverse_base_offset_southwest");
	offsets[3] = contents.get_ints("reverse_base_offset_southeast");
	offsets[4] = contents.get_ints("reverse_base_offset_north");
	offsets[5] = contents.get_ints("reverse_base_offset_east");
	offsets[6] = contents.get_ints("reverse_base_offset_northeast");
	offsets[7] = contents.get_ints("reverse_base_offset_northwest");

	for (uint32 i = 0; i < 8; i++)
	{
		if (offsets[i][0] >= 3)
		{
			for (uint32 j = 0; j < 3; j++)
			{
				env_t::reverse_base_offsets[i][j] = offsets[i][j+1];
			}
		}
		else
		{
			for (uint32 j = 0; j < 3; j++)
			{
				env_t::reverse_base_offsets[i][j] = 0;
			}
		}
	}

	// network stuff
	env_t::server_frames_ahead = contents.get_int("server_frames_ahead", env_t::server_frames_ahead );
	env_t::additional_client_frames_behind = contents.get_int("additional_client_frames_behind", env_t::additional_client_frames_behind);
	env_t::network_frames_per_step = contents.get_int("server_frames_per_step", env_t::network_frames_per_step );
	env_t::server_sync_steps_between_checks = contents.get_int("server_frames_between_checks", env_t::server_sync_steps_between_checks );
	env_t::pause_server_no_clients = contents.get_int("pause_server_no_clients", env_t::pause_server_no_clients );
	env_t::server_save_game_on_quit = contents.get_int("server_save_game_on_quit", env_t::server_save_game_on_quit );
	env_t::reload_and_save_on_quit = contents.get_int("reload_and_save_on_quit", env_t::reload_and_save_on_quit );

	env_t::server_announce = contents.get_int("announce_server", env_t::server_announce );
	env_t::server_announce = contents.get_int("server_announce", env_t::server_announce );
	env_t::server_announce_interval = contents.get_int("server_announce_intervall", env_t::server_announce_interval );
	env_t::server_announce_interval = contents.get_int("server_announce_interval", env_t::server_announce_interval );
	if (env_t::server_announce_interval < 60) {
		env_t::server_announce_interval = 60;
	}
	else if (env_t::server_announce_interval > 86400) {
		env_t::server_announce_interval = 86400;
	}
	if(  *contents.get("server_dns")  ) {
		env_t::server_dns = ltrim(contents.get("server_dns"));
	}
	if(  *contents.get("server_name")  ) {
		env_t::server_name = ltrim(contents.get("server_name"));
	}
	if(  *contents.get("server_comments")  ) {
		env_t::server_comments = ltrim(contents.get("server_comments"));
	}
	if(  *contents.get("server_email")  ) {
		env_t::server_email = ltrim(contents.get("server_email"));
	}
	if(  *contents.get("server_pakurl")  ) {
		env_t::server_pakurl = ltrim(contents.get("server_pakurl"));
	}
	if(  *contents.get("server_infurl")  ) {
		env_t::server_infurl = ltrim(contents.get("server_infurl"));
	}
	if(  *contents.get("server_admin_pw")  ) {
		env_t::server_admin_pw = ltrim(contents.get("server_admin_pw"));
	}
	if(  *contents.get("nickname")  ) {
		env_t::nickname = ltrim(contents.get("nickname"));
	}
	if(  *contents.get("server_motd_filename")  ) {
		env_t::server_motd_filename = ltrim(contents.get("server_motd_filename"));
	}

	// listen directive is a comma separated list of IP addresses to listen on
	if(  *contents.get("listen")  ) {
		env_t::listen.clear();
		std::string s = ltrim(contents.get("listen"));

		// Find index of first ',' copy from start of string to that position
		// Set start index to last position, then repeat
		// When ',' not found, copy remainder of string

		size_t start = 0;
		size_t end;

		end = s.find_first_of(",");
		env_t::listen.append_unique( ltrim( s.substr( start, end ).c_str() ) );
		while (  end != std::string::npos  ) {
			start = end;
			end = s.find_first_of( ",", start + 1 );
			env_t::listen.append_unique( ltrim( s.substr( start + 1, end - 1 - start ).c_str() ) );
		}
	}

	drive_on_left = contents.get_int("drive_left", drive_on_left );
	signals_on_left = contents.get_int("signals_on_left", signals_on_left );
	allow_underground_transformers = contents.get_int( "allow_underground_transformers", allow_underground_transformers )!=0;
	disable_make_way_public = contents.get_int("disable_make_way_public", disable_make_way_public) != 0;

	// up to ten rivers are possible
	for(  int i = 0;  i<10;  i++  ) {
		char name[32];
		sprintf( name, "river_type[%i]", i );
		const char *test = ltrim(contents.get(name) );
		if(*test) {
			const int add_river = i<env_t::river_types ? i : env_t::river_types;
			env_t::river_type[add_river] = test;
			if(  add_river==env_t::river_types  ) {
				env_t::river_types++;
			}
		}
	}

	// old syntax for single city road
	const char *str = ltrim(contents.get("city_road_type") );
	if(  str[0]  ) {
		num_city_roads = 1;
		tstrncpy(city_roads[0].name, str, lengthof(city_roads[0].name) );
		rtrim( city_roads[0].name );
		// default her: always available
		city_roads[0].intro = 1;
		city_roads[0].retire = NEVER;
	}

	// up to 16 city_roads are possible
	if(  *contents.get("city_road[0]")  ) {
		// renew them always when a table is encountered ...
		num_city_roads = 0;
		for(  int i = 0;  i<16;  i++  ) {
			char name[256];
			sprintf( name, "city_road[%i]", i );
			// format is "city_road[%i]=name_of_road,using from (year), using to (year)"
			const char *test = ltrim(contents.get(name) );
			if(*test) {
				const char *p = test;
				while(*p  &&  *p!=','  ) {
					p++;
				}
				tstrncpy( city_roads[num_city_roads].name, test, (unsigned)(p-test)+1 );
				// default her: intro/retire=0 -> set later to intro/retire of way-desc
				city_roads[num_city_roads].intro = 0;
				city_roads[num_city_roads].retire = 0;
				if(  *p==','  ) {
					++p;
					city_roads[num_city_roads].intro = atoi(p)*12;
					while(*p  &&  *p!=','  ) {
						p++;
					}
				}
				if(  *p==','  ) {
					city_roads[num_city_roads].retire = atoi(p+1)*12;
				}
				num_city_roads ++;
			}
		}
	}
	if (num_city_roads == 0) {
		// take fallback value: "city_road"
		tstrncpy(city_roads[0].name, "city_road", lengthof(city_roads[0].name) );
		// default her: always available
		city_roads[0].intro = 1;
		city_roads[0].retire = NEVER;
		num_city_roads = 1;
	}

	// intercity roads
	// old syntax for single intercity road
	str = ltrim(contents.get("intercity_road_type") );
	if(  str[0]  ) {
		num_intercity_roads = 1;
		tstrncpy(intercity_roads[0].name, str, lengthof(intercity_roads[0].name) );
		rtrim( intercity_roads[0].name );
		// default her: always available
		intercity_roads[0].intro = 1;
		intercity_roads[0].retire = NEVER;
	}

	// up to 16 intercity_roads are possible
	if(  *contents.get("intercity_road[0]")  ) {
		// renew them always when a table is encountered ...
		num_intercity_roads = 0;
		for(  int i = 0;  i<16;  i++  ) {
			char name[256];
			sprintf( name, "intercity_road[%i]", i );
			// format is "intercity_road[%i]=name_of_road,using from (year), using to (year)"
			const char *test = ltrim(contents.get(name) );
			if(*test) {
				const char *p = test;
				while(*p  &&  *p!=','  ) {
					p++;
				}
				tstrncpy( intercity_roads[num_intercity_roads].name, test, (unsigned)(p-test)+1 );
				// default her: intro/retire=0 -> set later to intro/retire of way-desc
				intercity_roads[num_intercity_roads].intro = 0;
				intercity_roads[num_intercity_roads].retire = 0;
				if(  *p==','  ) {
					++p;
					intercity_roads[num_intercity_roads].intro = atoi(p)*12;
					while(*p  &&  *p!=','  ) {
						p++;
					}
				}
				if(  *p==','  ) {
					intercity_roads[num_intercity_roads].retire = atoi(p+1)*12;
				}
				num_intercity_roads ++;
			}
		}
	}
	if (num_intercity_roads == 0) {
		// take fallback value: "asphalt_road"
		tstrncpy(intercity_roads[0].name, "asphalt_road", lengthof(intercity_roads[0].name) );
		// default her: always available
		intercity_roads[0].intro = 1;
		intercity_roads[0].retire = NEVER;
		num_intercity_roads = 1;
	}

	env_t::autosave = (contents.get_int("autosave", env_t::autosave) );

	max_route_steps = contents.get_int("max_route_steps", max_route_steps );
	max_choose_route_steps = contents.get_int("max_choose_route_steps", max_choose_route_steps );
	max_hops = contents.get_int("max_hops", max_hops );
	max_transfers = contents.get_int("max_transfers", max_transfers );

	special_building_distance = contents.get_int("special_building_distance", special_building_distance );
	industry_increase = contents.get_int("industry_increase_every", industry_increase );
	city_isolation_factor = contents.get_int("city_isolation_factor", city_isolation_factor );
	factory_arrival_periods = clamp( contents.get_int("factory_arrival_periods", factory_arrival_periods), 1, 16 );
	factory_enforce_demand = contents.get_int("factory_enforce_demand", factory_enforce_demand) != 0;
	factory_maximum_intransit_percentage  = contents.get_int("maximum_intransit_percentage", factory_maximum_intransit_percentage);

	//tourist_percentage = contents.get_int("tourist_percentage", tourist_percentage );
	// .. read twice: old and right spelling
	separate_halt_capacities = contents.get_int("seperate_halt_capacities", separate_halt_capacities ) != 0;
	separate_halt_capacities = contents.get_int("separate_halt_capacities", separate_halt_capacities ) != 0;

	//pay_for_total_distance = contents.get_int("pay_for_total_distance", pay_for_total_distance );
	avoid_overcrowding = contents.get_int("avoid_overcrowding", avoid_overcrowding )!=0;
	passenger_max_wait = contents.get_int("passenger_max_wait", passenger_max_wait);
	max_rerouting_interval_months = contents.get_int("max_rerouting_interval_months", max_rerouting_interval_months);

	// city stuff
	passenger_multiplier = contents.get_int("passenger_multiplier", passenger_multiplier );
	mail_multiplier = contents.get_int("mail_multiplier", mail_multiplier );
	goods_multiplier = contents.get_int("goods_multiplier", goods_multiplier );
	electricity_multiplier = contents.get_int("electricity_multiplier", electricity_multiplier );

	growthfactor_small = contents.get_int("growthfactor_villages", growthfactor_small );
	growthfactor_medium = contents.get_int("growthfactor_cities", growthfactor_medium );
	growthfactor_large = contents.get_int("growthfactor_capitals", growthfactor_large );

	random_pedestrians = contents.get_int("random_pedestrians", random_pedestrians ) != 0;
	show_pax = contents.get_int("stop_pedestrians", show_pax ) != 0;
	traffic_level = contents.get_int("citycar_level", traffic_level );
	stadtauto_duration = contents.get_int("default_citycar_life", stadtauto_duration );	// ten normal years
	allow_buying_obsolete_vehicles = contents.get_int("allow_buying_obsolete_vehicles", allow_buying_obsolete_vehicles );
	used_vehicle_reduction  = clamp( contents.get_int("used_vehicle_reduction", used_vehicle_reduction ), 0, 1000 );

	// starting money
	starting_money = contents.get_int64("starting_money", starting_money );
	/* up to ten blocks year, money, interpolation={0,1} are possible:
	 * starting_money[i]=y,m,int
	 * y .. year
	 * m .. money (in 1/100 Cr)
	 * int .. interpolation: 0 - no interpolation, !=0 linear interpolated
	 * (m) is the starting money for player start after (y), if (i)!=0, the starting money
	 * is linearly interpolated between (y) and the next greater year given in another entry.
	 * starting money for given year is:
	 */
	int j=0;
	for(  int i = 0;  i<10;  i++  ) {
		char name[32];
		sprintf( name, "starting_money[%i]", i );
		sint64 *test = contents.get_sint64s(name );
		if ((test[0]>1) && (test[0]<=3)) {
			// insert sorted by years
			int k=0;
			for (k=0; k<i; k++) {
				if (startingmoneyperyear[k].year > test[1]) {
					for (int l=j; l>=k; l--)
						memcpy( &startingmoneyperyear[l+1], &startingmoneyperyear[l], sizeof(yearmoney) );
					break;
				}
			}
			startingmoneyperyear[k].year = (sint16)test[1];
			startingmoneyperyear[k].money = test[2];
			if (test[0]==3) {
				startingmoneyperyear[k].interpol = test[3]!=0;
			}
			else {
				startingmoneyperyear[k].interpol = false;
			}
//			printf("smpy[%d] year=%d money=%lld\n",k,startingmoneyperyear[k].year,startingmoneyperyear[k].money);
			j++;
		}
		else {
			// invalid entry
		}
		delete [] test;
	}
	// at least one found => use this now!
	if(  j>0  &&  startingmoneyperyear[0].money>0  ) {
		starting_money = 0;
		// fill remaining entries
		for(  int i=j+1; i<10; i++  ) {
			startingmoneyperyear[i].year = 0;
			startingmoneyperyear[i].money = 0;
			startingmoneyperyear[i].interpol = 0;
		}
	}

	// player stuff
	remove_dummy_player_months = contents.get_int("remove_dummy_player_months", remove_dummy_player_months );
	// .. read twice: old and right spelling
	unprotect_abandoned_player_months = contents.get_int("unprotect_abondoned_player_months", unprotect_abandoned_player_months );
	unprotect_abandoned_player_months = contents.get_int("unprotect_abandoned_player_months", unprotect_abandoned_player_months );
	default_player_color_random = contents.get_int("random_player_colors", default_player_color_random ) != 0;
	for(  int i = 0;  i<MAX_PLAYER_COUNT;  i++  ) {
		char name[32];
		sprintf( name, "player_color[%i]", i );
		const char *command = contents.get(name);
		int c1, c2;
		if(  sscanf( command, "%i,%i", &c1, &c2 )==2  ) {
			default_player_color[i][0] = c1;
			default_player_color[i][1] = c2;
		}
	}
	default_ai_construction_speed = env_t::default_ai_construction_speed = contents.get_int("ai_construction_speed", env_t::default_ai_construction_speed );

	sint64 new_maintenance_building = contents.get_int64("maintenance_building", -1);
	if (new_maintenance_building > 0) {
		maint_building = new_maintenance_building * distance_per_tile;
	}

	numbered_stations = contents.get_int("numbered_stations", numbered_stations );
	station_coverage_size = contents.get_int("station_coverage", station_coverage_size );
	station_coverage_size_factories = contents.get_int("station_coverage_factories", station_coverage_size_factories );

	// time stuff
	bits_per_month = contents.get_int("bits_per_month", bits_per_month );
	calc_job_replenishment_ticks();
	use_timeline = contents.get_int("use_timeline", use_timeline );
	starting_year = contents.get_int("starting_year", starting_year );
	starting_month = contents.get_int("starting_month", starting_month+1)-1;

	env_t::new_height_map_conversion = contents.get_int("new_height_map_conversion", env_t::new_height_map_conversion );

	river_number = contents.get_int("river_number", river_number );
	min_river_length = contents.get_int("river_min_length", min_river_length );
	max_river_length = contents.get_int("river_max_length", max_river_length );

	// forest stuff (now part of simuconf.tab)
	forest_base_size = contents.get_int("forest_base_size", forest_base_size );
	forest_map_size_divisor = contents.get_int("forest_map_size_divisor", forest_map_size_divisor );
	forest_count_divisor = contents.get_int("forest_count_divisor", forest_count_divisor );
	forest_inverse_spare_tree_density = contents.get_int("forest_inverse_spare_tree_density", forest_inverse_spare_tree_density );
	max_no_of_trees_on_square = contents.get_int("max_no_of_trees_on_square", max_no_of_trees_on_square );
	tree_climates = contents.get_int("tree_climates", tree_climates );
	no_tree_climates = contents.get_int("no_tree_climates", no_tree_climates );
	no_trees = contents.get_int("no_trees", no_trees );
	lake = !contents.get_int("no_lakes", !lake );

	// these are pak specific; the diagonal length affect travelling time (is game critical)
	pak_diagonal_multiplier = contents.get_int("diagonal_multiplier", pak_diagonal_multiplier );
	// the height in z-direction will only cause pixel errors but not a different behaviour
	env_t::pak_tile_height_step = contents.get_int("tile_height", env_t::pak_tile_height_step );
	// new height for old slopes after conversion - 1=single height, 2=double height
	// Must be only overwrite when reading from pak dir ...
	env_t::pak_height_conversion_factor = contents.get_int("height_conversion_factor", env_t::pak_height_conversion_factor );

	// minimum clearance under under bridges: 1 or 2? (HACK: value only zero during loading of pak set config)
	const int bounds = way_height_clearance != 0 ? 1 : 0;
	way_height_clearance  = contents.get_int("way_height_clearance", way_height_clearance );
	if(  way_height_clearance > 2  &&  way_height_clearance < bounds  ) {
		sint8 new_whc = clamp( way_height_clearance, bounds, 2 );
		dbg->warning( "settings_t::parse_simuconf()", "Illegal way_height_clearance of %i set to %i", way_height_clearance, new_whc );
		way_height_clearance = new_whc;
	}

	min_factory_spacing = contents.get_int("factory_spacing", min_factory_spacing );
	min_factory_spacing = contents.get_int("min_factory_spacing", min_factory_spacing );
	max_factory_spacing = contents.get_int("max_factory_spacing", max_factory_spacing );
	max_factory_spacing_percentage = contents.get_int("max_factory_spacing_percentage", max_factory_spacing_percentage );
	crossconnect_factories = contents.get_int("crossconnect_factories", crossconnect_factories ) != 0;
	crossconnect_factor = contents.get_int("crossconnect_factories_percentage", crossconnect_factor );
	electric_promille = contents.get_int("electric_promille", electric_promille );

	just_in_time = contents.get_int("just_in_time", just_in_time);
	beginner_price_factor = contents.get_int("beginner_price_factor", beginner_price_factor ); /* this manipulates the good prices in beginner mode */
	beginner_mode = contents.get_int("first_beginner", beginner_mode ); /* start in beginner mode */

	way_toll_runningcost_percentage = contents.get_int("toll_runningcost_percentage", way_toll_runningcost_percentage );
	way_toll_waycost_percentage = contents.get_int("toll_waycost_percentage", way_toll_waycost_percentage );
	way_toll_revenue_percentage = contents.get_int("toll_revenue_percentage", way_toll_revenue_percentage );
	seaport_toll_revenue_percentage = contents.get_int("seaport_toll_revenue_percentage", seaport_toll_revenue_percentage );
	airport_toll_revenue_percentage = contents.get_int("airport_toll_revenue_percentage", airport_toll_revenue_percentage );

	/* now the cost section */
	// Account for multiple loading of conflicting simuconf.tab files correctly.
	// Assume that negative numbers in the simuconf.tab file are invalid cost options (zero might be valid).
	// Annoyingly, the cst_ numbers are stored negative; assume positive numbers are invalid there...

	// Stations.  (Overridden by specific prices in pak files.)
	sint64 new_cost_multiply_dock = contents.get_int64("cost_multiply_dock", -1);
	if (new_cost_multiply_dock > 0) {
		cst_multiply_dock = new_cost_multiply_dock * -100 * distance_per_tile;
	}
	sint64 new_cost_multiply_station = contents.get_int64("cost_multiply_station", -1);
	if (new_cost_multiply_station > 0) {
		cst_multiply_station = new_cost_multiply_station * -100 * distance_per_tile;
	}
	sint64 new_cost_multiply_roadstop = contents.get_int64("cost_multiply_roadstop", -1);
	if (new_cost_multiply_roadstop > 0) {
		cst_multiply_roadstop = new_cost_multiply_roadstop * -100 * distance_per_tile;
	}
	sint64 new_cost_multiply_airterminal = contents.get_int64("cost_multiply_airterminal", -1);
	if (new_cost_multiply_airterminal > 0) {
		cst_multiply_airterminal = new_cost_multiply_airterminal * -100 * distance_per_tile;
	}
	// "mail" is auxiliary station buildings
	sint64 new_cost_multiply_post = contents.get_int64("cost_multiply_post", -1);
	if (new_cost_multiply_post > 0) {
		cst_multiply_post = new_cost_multiply_post * -100 * distance_per_tile;
	}

	// Depots & HQ are a bit simpler because not adjusted for distance per tile (not distance based).
	//   It should be possible to override this in .dat files, but it isn't
	cst_multiply_headquarter = contents.get_int64("cost_multiply_headquarter", cst_multiply_headquarter/(-100) ) * -100;
	cst_depot_air = contents.get_int64("cost_depot_air", cst_depot_air/(-100) ) * -100;
	cst_depot_rail = contents.get_int64("cost_depot_rail", cst_depot_rail/(-100) ) * -100;
	cst_depot_road = contents.get_int64("cost_depot_road", cst_depot_road/(-100) ) * -100;
	cst_depot_ship = contents.get_int64("cost_depot_ship", cst_depot_ship/(-100) ) * -100;

	// Set slope or alter it the "cheaper" way.
	// This *should* be adjusted for distance per tile, because it's actually distance-based.
	// But we weren't adjusting it before extended version 12.
	// We do not attempt to correct saved games as this was part of the "game balance" involved
	// with that game.  A save game can be changed using the override options.
	sint64 new_cost_alter_land = contents.get_int64("cost_alter_land", -1);
	if (new_cost_alter_land > 0) {
		cst_alter_land = new_cost_alter_land * -100 * distance_per_tile;
	}
	sint64 new_cost_set_slope = contents.get_int64("cost_set_slope", -1);
	if (new_cost_set_slope > 0) {
		cst_set_slope = new_cost_set_slope * -100 * distance_per_tile;
	}
	// Remove trees.  Probably distance based (if we're clearing a long area).
	sint64 new_cost_remove_tree = contents.get_int64("cost_remove_tree", -1);
	if (new_cost_remove_tree > 0) {
		cst_remove_tree = new_cost_remove_tree * -100 * distance_per_tile;
	}
	// Purchase land (often a house).  Distance-based, adjust for distance_per_tile.
	sint64 new_cost_buy_land = contents.get_int64("cost_buy_land", -1);
	if (new_cost_buy_land > 0) {
		cst_buy_land = new_cost_buy_land * -100 * distance_per_tile;
	}
	// Delete house or field.  Both are definitely distance based.
	// (You're usually trying to drive a railway line through a field.)
	// Fields were not adjusted for distance before version 12.
	// We do not attempt to correct saved games as this was part of the "game balance" involved
	// with that game.  A save game can be changed using the override options.
	sint64 new_cost_multiply_remove_haus = contents.get_int64("cost_multiply_remove_haus", -1);
	if (new_cost_multiply_remove_haus > 0) {
		cst_multiply_remove_haus = new_cost_multiply_remove_haus * -100 * distance_per_tile;
	}
	sint64 new_cost_multiply_remove_field = contents.get_int64("cost_multiply_remove_field", -1);
	if (new_cost_multiply_remove_field > 0) {
		cst_multiply_remove_field = new_cost_multiply_remove_field * -100 * distance_per_tile;
	}

	// Found city or industry chain.  Not distance based.
	cst_found_city = contents.get_int64("cost_found_city", cst_found_city/(-100) ) * -100;
	cst_multiply_found_industry = contents.get_int64("cost_multiply_found_industry", cst_multiply_found_industry/(-100) ) * -100;

	// Transformers.  Not distance based.
	//   It should be possible to override this in .dat files, but it isn't
	cst_transformer = contents.get_int64("cost_transformer", cst_transformer/(-100) ) * -100;
	cst_maintain_transformer = contents.get_int64("cost_maintain_transformer", cst_maintain_transformer/(-100) ) * -100;

	cst_make_public_months = contents.get_int64("cost_make_public_months", cst_make_public_months);

	/* now the way builder */
	way_count_straight = contents.get_int("way_straight", way_count_straight );
	way_count_curve = contents.get_int("way_curve", way_count_curve );
	way_count_double_curve = contents.get_int("way_double_curve", way_count_double_curve );
	way_count_90_curve = contents.get_int("way_90_curve", way_count_90_curve );
	way_count_slope = contents.get_int("way_slope", way_count_slope );
	way_count_tunnel = contents.get_int("way_tunnel", way_count_tunnel );
	way_max_bridge_len = contents.get_int("way_max_bridge_len", way_max_bridge_len );
	way_count_leaving_road = contents.get_int("way_leaving_road", way_count_leaving_road );

	// Revenue calibration settings
	// @author: jamespetts
	tolerable_comfort_short = contents.get_int("tolerable_comfort_short", tolerable_comfort_short);
	tolerable_comfort_long = contents.get_int("tolerable_comfort_long", tolerable_comfort_long);
	tolerable_comfort_short_minutes = contents.get_int("tolerable_comfort_short_minutes", tolerable_comfort_short_minutes);
	tolerable_comfort_long_minutes = contents.get_int("tolerable_comfort_long_minutes", tolerable_comfort_long_minutes);
	tolerable_comfort_median_short = contents.get_int("tolerable_comfort_median_short", tolerable_comfort_median_short);
	tolerable_comfort_median_median = contents.get_int("tolerable_comfort_median_median", tolerable_comfort_median_median);
	tolerable_comfort_median_long = contents.get_int("tolerable_comfort_median_long", tolerable_comfort_median_long);
	tolerable_comfort_median_short_minutes = contents.get_int("tolerable_comfort_median_short_minutes", tolerable_comfort_median_short_minutes);
	tolerable_comfort_median_median_minutes = contents.get_int("tolerable_comfort_median_median_minutes", tolerable_comfort_median_median_minutes);
	tolerable_comfort_median_long_minutes = contents.get_int("tolerable_comfort_median_long_minutes", tolerable_comfort_median_long_minutes);
	max_luxury_bonus_differential = contents.get_int("max_luxury_bonus_differential", max_luxury_bonus_differential);
	max_discomfort_penalty_differential = contents.get_int("max_discomfort_penalty_differential", max_discomfort_penalty_differential);
	max_luxury_bonus_percent = contents.get_int("max_luxury_bonus_percent", max_luxury_bonus_percent);
	max_discomfort_penalty_percent = contents.get_int("max_discomfort_penalty_percent", max_discomfort_penalty_percent);
	cache_comfort_tables();

	catering_min_minutes = contents.get_int("catering_min_minutes", catering_min_minutes);
	catering_level1_minutes = contents.get_int("catering_level1_minutes", catering_level1_minutes);
	catering_level1_max_revenue = contents.get_int("catering_level1_max_revenue", catering_level1_max_revenue);
	catering_level2_minutes = contents.get_int("catering_level2_minutes", catering_level2_minutes);
	catering_level2_max_revenue = contents.get_int("catering_level2_max_revenue", catering_level2_max_revenue);
	catering_level3_minutes = contents.get_int("catering_level3_minutes", catering_level3_minutes);
	catering_level3_max_revenue = contents.get_int("catering_level3_max_revenue", catering_level3_max_revenue);
	catering_level4_minutes = contents.get_int("catering_level4_minutes", catering_level4_minutes);
	catering_level4_max_revenue = contents.get_int("catering_level4_max_revenue", catering_level4_max_revenue);
	catering_level5_minutes = contents.get_int("catering_level5_minutes", catering_level5_minutes);
	catering_level5_max_revenue = contents.get_int("catering_level5_max_revenue", catering_level5_max_revenue);
	cache_catering_revenues();

	tpo_min_minutes = contents.get_int("tpo_min_minutes", tpo_min_minutes);
	tpo_revenue = contents.get_int("tpo_revenue", tpo_revenue);

	// Obsolete vehicles' running cost increase
	obsolete_running_cost_increase_percent = contents.get_int("obsolete_running_cost_increase_percent", obsolete_running_cost_increase_percent);
	obsolete_running_cost_increase_phase_years = contents.get_int("obsolete_running_cost_increase_phase_years", obsolete_running_cost_increase_phase_years);

	// Passenger routing settings
	passenger_routing_packet_size = contents.get_int("passenger_routing_packet_size", passenger_routing_packet_size);
	if(passenger_routing_packet_size < 1)
	{
		passenger_routing_packet_size = 7;
	}

	min_alternative_destinations_visiting = contents.get_int("min_alternative_destinations_visiting", min_alternative_destinations_visiting);
	min_alternative_destinations_commuting = contents.get_int("min_alternative_destinations_commuting", min_alternative_destinations_commuting);

	// Subtract the minima from the maxima because these are added again when the random numbers are generated
	const uint16 old_max_alternative_destinations = contents.get_int("max_alternative_destinations", max_alternative_destinations_visiting);
	max_alternative_destinations_visiting = contents.get_int("max_alternative_destinations_visiting", old_max_alternative_destinations) - min_alternative_destinations_visiting;
	max_alternative_destinations_commuting = contents.get_int("max_alternative_destinations_commuting", max_alternative_destinations_commuting) - min_alternative_destinations_commuting;

	max_alternative_destinations_per_job_millionths = contents.get_int("max_alternative_destinations_per_job_millionths", max_alternative_destinations_per_job_millionths);
	max_alternative_destinations_per_visitor_demand_millionths = contents.get_int("max_alternative_destinations_per_visitor_demand_millionths", max_alternative_destinations_per_visitor_demand_millionths);

	min_alternative_destinations_per_job_millionths = contents.get_int("min_alternative_destinations_per_job_millionths", min_alternative_destinations_per_job_millionths);
	min_alternative_destinations_per_visitor_demand_millionths = contents.get_int("min_alternative_destinations_per_visitor_demand_millionths", min_alternative_destinations_per_visitor_demand_millionths);

	always_prefer_car_percent = contents.get_int("always_prefer_car_percent", always_prefer_car_percent);
	congestion_density_factor = contents.get_int("congestion_density_factor", congestion_density_factor);

	// Cornering settings
	corner_force_divider[waytype_t(road_wt)] = contents.get_int("corner_force_divider_road", corner_force_divider[waytype_t(road_wt)]);
	corner_force_divider[waytype_t(track_wt)] = contents.get_int("corner_force_divider_track", corner_force_divider[waytype_t(track_wt)]);
	corner_force_divider[waytype_t(tram_wt)] = contents.get_int("corner_force_divider_tram", corner_force_divider[waytype_t(tram_wt)]);
	corner_force_divider[waytype_t(monorail_wt)] = contents.get_int("corner_force_divider_monorail", corner_force_divider[waytype_t(monorail_wt)]);
	corner_force_divider[waytype_t(maglev_wt)] = contents.get_int("corner_force_divider_maglev", corner_force_divider[waytype_t(maglev_wt)]);
	corner_force_divider[waytype_t(narrowgauge_wt)] = contents.get_int("corner_force_divider_narrowgauge", corner_force_divider[waytype_t(narrowgauge_wt)]);

	curve_friction_factor[waytype_t(road_wt)] = contents.get_int("curve_friction_factor_road", curve_friction_factor[waytype_t(road_wt)]);
	curve_friction_factor[waytype_t(track_wt)] = contents.get_int("curve_friction_factor_track", curve_friction_factor[waytype_t(track_wt)]);
	curve_friction_factor[waytype_t(tram_wt)] = contents.get_int("curve_friction_factor_tram", curve_friction_factor[waytype_t(tram_wt)]);
	curve_friction_factor[waytype_t(monorail_wt)] = contents.get_int("curve_friction_factor_monorail", curve_friction_factor[waytype_t(monorail_wt)]);
	curve_friction_factor[waytype_t(maglev_wt)] = contents.get_int("curve_friction_factor_maglev", curve_friction_factor[waytype_t(maglev_wt)]);
	curve_friction_factor[waytype_t(narrowgauge_wt)] = contents.get_int("curve_friction_factor_narrowgauge", curve_friction_factor[waytype_t(narrowgauge_wt)]);

	tilting_min_radius_effect = contents.get_int("tilting_min_radius_effect", tilting_min_radius_effect);

	// Factory settings
	factory_max_years_obsolete = contents.get_int("max_years_obsolete", factory_max_years_obsolete);

	// @author: jamespetts
	// Insolvency and debt settings
	interest_rate_percent = contents.get_int("interest_rate_percent", interest_rate_percent);
	// Check for misspelled version
	allow_insolvency = contents.get_int("allow_bankruptsy", allow_insolvency);
	// Check for deprecated version
	allow_insolvency = contents.get_int("allow_bankruptcy", allow_insolvency);
	allow_insolvency = contents.get_int("allow_insolvency", allow_insolvency);
	// Check for misspelled version
	allow_purchases_when_insolvent = contents.get_int("allow_purhcases_when_insolvent", allow_purchases_when_insolvent);
	allow_purchases_when_insolvent = contents.get_int("allow_purchases_when_insolvent", allow_purchases_when_insolvent);

	// Reversing settings
	// @author: jamespetts
	unit_reverse_time = contents.get_int("unit_reverse_time", unit_reverse_time);
	hauled_reverse_time = contents.get_int("hauled_reverse_time", hauled_reverse_time);
	turntable_reverse_time = contents.get_int("turntable_reverse_time", turntable_reverse_time);
	road_reverse_time = contents.get_int("road_reverse_time", road_reverse_time);

	unit_reverse_time_seconds = contents.get_int("unit_reverse_time_seconds", unit_reverse_time_seconds);
	hauled_reverse_time_seconds = contents.get_int("hauled_reverse_time_seconds", hauled_reverse_time_seconds);
	turntable_reverse_time_seconds = contents.get_int("turntable_reverse_time_seconds", turntable_reverse_time_seconds);
	road_reverse_time_seconds = contents.get_int("road_reverse_time_seconds", road_reverse_time_seconds);

	// Global power factor
	// @author: jamespetts
	global_power_factor_percent = contents.get_int("global_power_factor_percent", global_power_factor_percent);
	global_force_factor_percent = contents.get_int("global_force_factor_percent", global_force_factor_percent);

	// How and whether weight limits are enforced.
	// @author: jamespetts
	enforce_weight_limits = contents.get_int("enforce_weight_limits", enforce_weight_limits);

	allow_airports_without_control_towers = contents.get_int("allow_airports_without_control_towers", allow_airports_without_control_towers);

	// Multiply by 10 because journey times are measured in tenths of minutes.
	//@author: jamespetts
	const uint32 min_visiting_tolerance_minutes = contents.get_int("min_visiting_tolerance", (min_visiting_tolerance / 10u));
	min_visiting_tolerance = min_visiting_tolerance_minutes * 10u;
	const uint32 range_commuting_tolerance_minutes = contents.get_int("range_commuting_tolerance", (range_commuting_tolerance / 10u));
	range_commuting_tolerance = range_commuting_tolerance_minutes * 10u;
	const uint32 min_commuting_tolerance_minutes = contents.get_int("min_commuting_tolerance", (min_commuting_tolerance/ 10u));
	min_commuting_tolerance = min_commuting_tolerance_minutes * 10u;
	const uint32 range_visiting_tolerance_minutes = contents.get_int("range_visiting_tolerance", (range_visiting_tolerance / 10u));
	range_visiting_tolerance = range_visiting_tolerance_minutes * 10u;

	quick_city_growth = (bool)(contents.get_int("quick_city_growth", quick_city_growth));

	allow_routing_on_foot = (bool)(contents.get_int("allow_routing_on_foot", allow_routing_on_foot));

	min_wait_airport = contents.get_int("min_wait_airport", min_wait_airport) * 10; // Stored as 10ths of minutes

	toll_free_public_roads = (bool)contents.get_int("toll_free_public_roads", toll_free_public_roads);

	private_car_toll_per_km = contents.get_int("private_car_toll_per_km", private_car_toll_per_km);

	towns_adopt_player_roads = (bool)contents.get_int("towns_adopt_player_roads", towns_adopt_player_roads);

	max_elevated_way_building_level = (uint8)contents.get_int("max_elevated_way_building_level", max_elevated_way_building_level);

	tolerance_modifier_percentage = contents.get_int("tolerance_modifier_percentage", tolerance_modifier_percentage);

	industry_density_proportion_override = contents.get_int("industry_density_proportion_override", industry_density_proportion_override);

	assume_everywhere_connected_by_road = (bool)(contents.get_int("assume_everywhere_connected_by_road", assume_everywhere_connected_by_road));

	allow_making_public = (bool)(contents.get_int("allow_making_public", allow_making_public));

	reroute_check_interval_steps = contents.get_int("reroute_check_interval_steps", reroute_check_interval_steps);

	walking_speed = contents.get_int("walking_speed", walking_speed);

	random_mode_commuting = contents.get_int("random_mode_commuting", random_mode_commuting);
	random_mode_visiting = contents.get_int("random_mode_visiting", random_mode_visiting);

	for(uint8 i = road_wt; i <= air_wt; i ++)
	{
		std::string buf;
		switch(i)
		{
		case road_wt:
			buf = "default_increase_maintenance_after_years_road";
			break;
		case track_wt:
			buf = "default_increase_maintenance_after_years_rail";
			break;
		case water_wt:
			buf = "default_increase_maintenance_after_years_water";
			break;
		case monorail_wt:
			buf = "default_increase_maintenance_after_years_monorail";
			break;
		case maglev_wt:
			buf = "default_increase_maintenance_after_years_maglev";
			break;
		case tram_wt:
			buf = "default_increase_maintenance_after_years_tram";
			break;
		case narrowgauge_wt:
			buf = "default_increase_maintenance_after_years_narrowgauge";
			break;
		case air_wt:
			buf = "default_increase_maintenance_after_years_air";
			break;
		default:
			buf = "default_increase_maintenance_after_years_other";
		}
		default_increase_maintenance_after_years[i] = contents.get_int(buf.c_str(), default_increase_maintenance_after_years[i]);
	}

	city_threshold_size  = contents.get_int("city_threshold_size", city_threshold_size);
	capital_threshold_size  = contents.get_int("capital_threshold_size", capital_threshold_size);
	max_small_city_size  = contents.get_int("max_small_city_size", max_small_city_size);
	max_city_size  = contents.get_int("max_city_size", max_city_size);
	capital_threshold_percentage = contents.get_int("capial_threshold_percentage", capital_threshold_percentage);
	city_threshold_percentage = contents.get_int("city_threshold_percentage", city_threshold_percentage);

	spacing_shift_mode = contents.get_int("spacing_shift_mode", spacing_shift_mode);
	spacing_shift_divisor = contents.get_int("spacing_shift_divisor", spacing_shift_divisor);

	population_per_level = contents.get_int("population_per_level", population_per_level);
	visitor_demand_per_level = contents.get_int("visitor_demand_per_level", visitor_demand_per_level);
	jobs_per_level = contents.get_int("jobs_per_level", jobs_per_level);
	mail_per_level = contents.get_int("mail_per_level", mail_per_level);

	passenger_trips_per_month_hundredths = contents.get_int("passenger_trips_per_month_hundredths", passenger_trips_per_month_hundredths);
	mail_packets_per_month_hundredths = contents.get_int("mail_packets_per_month_hundredths", mail_packets_per_month_hundredths);

	max_onward_trips = contents.get_int("max_onward_trips", max_onward_trips);
	onward_trip_chance_percent = contents.get_int("onward_trip_chance_percent", onward_trip_chance_percent);
	commuting_trip_chance_percent = contents.get_int("commuting_trip_chance_percent", commuting_trip_chance_percent);

	forge_cost_road = contents.get_int("forge_cost_road", forge_cost_road);
	forge_cost_track = contents.get_int("forge_cost_track", forge_cost_track);
	forge_cost_water = contents.get_int("forge_cost_water", forge_cost_water);
	forge_cost_monorail = contents.get_int("forge_cost_monorail", forge_cost_monorail);
	forge_cost_maglev = contents.get_int("forge_cost_maglev", forge_cost_maglev);
	forge_cost_tram = contents.get_int("forge_cost_tram", forge_cost_tram);
	forge_cost_narrowgauge = contents.get_int("forge_cost_narrowgauge", forge_cost_narrowgauge);
	forge_cost_air = contents.get_int("forge_cost_air", forge_cost_air);

	parallel_ways_forge_cost_percentage_road = contents.get_int("parallel_ways_forge_cost_percentage_road", parallel_ways_forge_cost_percentage_road);
	parallel_ways_forge_cost_percentage_track = contents.get_int("parallel_ways_forge_cost_percentage_track", parallel_ways_forge_cost_percentage_track);
	parallel_ways_forge_cost_percentage_water = contents.get_int("parallel_ways_forge_cost_percentage_water", parallel_ways_forge_cost_percentage_water);
	parallel_ways_forge_cost_percentage_monorail = contents.get_int("parallel_ways_forge_cost_percentage_monorail", parallel_ways_forge_cost_percentage_monorail);
	parallel_ways_forge_cost_percentage_maglev = contents.get_int("parallel_ways_forge_cost_percentage_maglev", parallel_ways_forge_cost_percentage_maglev);
	parallel_ways_forge_cost_percentage_tram = contents.get_int("parallel_ways_forge_cost_percentage_tram", parallel_ways_forge_cost_percentage_tram);
	parallel_ways_forge_cost_percentage_narrowgauge = contents.get_int("parallel_ways_forge_cost_percentage_narrowgauge", parallel_ways_forge_cost_percentage_narrowgauge);
	parallel_ways_forge_cost_percentage_air = contents.get_int("parallel_ways_forge_cost_percentage_air", parallel_ways_forge_cost_percentage_air);

	max_diversion_tiles = contents.get_int("max_diversion_tiles", max_diversion_tiles);

	way_degradation_fraction = contents.get_int("way_degradation_fraction", way_degradation_fraction);

	way_wear_power_factor_road_type = contents.get_int("way_wear_power_factor_road_type", way_wear_power_factor_road_type);
	way_wear_power_factor_rail_type = contents.get_int("way_wear_power_factor_rail_type", way_wear_power_factor_rail_type);
	standard_axle_load = contents.get_int("standard_axle_load", standard_axle_load);
	citycar_way_wear_factor = contents.get_int("citycar_way_wear_factor", citycar_way_wear_factor);

	sighting_distance_meters = contents.get_int("sighting_distance_meters", sighting_distance_meters);
	sighting_distance_tiles = sighting_distance_meters / meters_per_tile;

	assumed_curve_radius_45_degrees = contents.get_int("assumed_curve_radius_45_degrees", assumed_curve_radius_45_degrees);

	max_speed_drive_by_sight_kmh = contents.get_int("max_speed_drive_by_sight_kmh", max_speed_drive_by_sight_kmh);
	max_speed_drive_by_sight = kmh_to_speed(max_speed_drive_by_sight_kmh);

	time_interval_seconds_to_clear = contents.get_int("time_interval_seconds_to_clear", time_interval_seconds_to_clear);
	time_interval_seconds_to_caution = contents.get_int("time_interval_seconds_to_caution", time_interval_seconds_to_caution);

	town_road_speed_limit = contents.get_int("town_road_speed_limit", town_road_speed_limit);

	minimum_staffing_percentage_consumer_industry = contents.get_int("minimum_staffing_percentage_consumer_industry", minimum_staffing_percentage_consumer_industry);
	minimum_staffing_percentage_full_production_producer_industry = contents.get_int("minimum_staffing_percentage_full_production_producer_industry", minimum_staffing_percentage_full_production_producer_industry);

	max_comfort_preference_percentage = contents.get_int("max_comfort_preference_percentage", max_comfort_preference_percentage);

	rural_industries_no_staff_shortage = contents.get_int("rural_industries_no_staff_shortage", rural_industries_no_staff_shortage);
	auto_connect_industries_and_attractions_by_road = contents.get_int("auto_connect_industries_and_attractions_by_road", auto_connect_industries_and_attractions_by_road);

	path_explorer_time_midpoint = contents.get_int("path_explorer_time_midpoint", path_explorer_time_midpoint);
	save_path_explorer_data = contents.get_int("save_path_explorer_data", save_path_explorer_data);

	show_future_vehicle_info = contents.get_int("show_future_vehicle_information", show_future_vehicle_info);

	uint32 max_routes_to_process_in_a_step = contents.get_int("max_routes_to_process_in_a_step", 0);
	const uint32 old_max_route_tiles_extrapolated = max_routes_to_process_in_a_step * 1024;
	const uint32 max_route_tiles_default = old_max_route_tiles_extrapolated ? old_max_route_tiles_extrapolated : max_route_tiles_to_process_in_a_step;
	max_route_tiles_to_process_in_a_step = contents.get_int("max_route_tiles_to_process_in_a_step", max_route_tiles_default);

	// OK, this is a bit complex.  We are at risk of loading the same livery schemes repeatedly, which
	// gives duplicate livery schemes and utter confusion.
	// On the other hand, we are also at risk of wiping out our livery schemes with blank space.
	// So, *if* this file has livery schemes in it, we *replace* all previous livery schemes.
	// This does not allow for "addon livery schemes" but at least it works for the usual cases.
	// We could do better if we actually matched schemes up by index number.
	const char* first_scheme_name = contents.get("livery_scheme[0]");
	if (first_scheme_name[0] != '\0') {
		// This file has livery schemes.  Replace all previous.
		livery_schemes.clear();
		for(int i = 0; i < 65336; i ++)
		{
			char name[128] ;
			sprintf( name, "livery_scheme[%i]", i );
			const char* scheme_name = ltrim(contents.get(name));
			if(scheme_name[0] == '\0')
			{
				break;
			}

			sprintf( name, "retire_year[%i]", i );
			uint16 retire = contents.get_int(name, DEFAULT_RETIRE_DATE) * 12;

			sprintf( name, "retire_month[%i]", i );
			retire += contents.get_int(name, 1) - 1;

			livery_scheme_t* scheme = new livery_scheme_t(scheme_name, retire);

			bool has_liveries = false;
			for(int j = 0; j < 65536; j ++)
			{
				char livery[128];
				sprintf(livery, "livery[%i][%i]", i, j);
				const char* liv_name = ltrim(contents.get(livery));
				if(liv_name[0] == '\0')
				{
					break;
				}

				has_liveries = true;
				sprintf(livery, "intro_year[%i][%i]", i, j);
				uint16 intro = contents.get_int(livery, DEFAULT_INTRO_DATE) * 12;

				sprintf(livery, "intro_month[%i][%i]", i, j);
				intro += contents.get_int(livery, 1) - 1;

				scheme->add_livery(liv_name, intro);
			}
			if(has_liveries)
			{
				livery_schemes.append(scheme);
			}
			else
			{
				delete scheme;
			}
		}
	}

	// Read region data
	regions.clear();
	absolute_regions = false;
	for (int i = 0; i < 255; i++) // NOTE: We can define up to 254 regions. The number 255 is reserved for indicating no region in some contexts.
	{
		char name[128];
		sprintf(name, "region_name[%i]", i);

		const char* region_name = ltrim(contents.get(name));
		if (region_name[0] == '\0')
		{
			break;
		}

		char upper_left[128];
		sprintf(upper_left, "region_upper_left_percent[%i]", i);

		char lower_right[128];
		sprintf(lower_right, "region_lower_right_percent[%i]", i);

		int* ul = contents.get_ints(upper_left);
		int* lr = contents.get_ints(lower_right);

		uint32 x_percent = 0;
		uint32 y_percent = 0;
		region_definition_t r;

		if ((ul[0] & 1) == 1)
		{
			dbg->message("void settings_t::parse_simuconf(", "Ill formed line in config/simuconf.tab.\nWill use default value. Format is region_upper_left[percent]=x,y");
			break;
		}

		for (int i = 1; i < ul[0]; i += 2)
		{
			x_percent = (uint32)ul[i];
			y_percent = (uint32)ul[i+1];
		}

		uint32 x = x_percent > 0 ? ((uint32)size_x * x_percent) / 100u : 0;
		uint32 y = y_percent > 0 ? ((uint32)size_y * y_percent) / 100u : 0;

		r.top_left = koord(x, y);

		if ((lr[0] & 1) == 1)
		{
			dbg->message("void settings_t::parse_simuconf(", "Ill formed line in config/simuconf.tab.\nWill use default value. Format is region_upper_left[percent]=x,y");
			break;
		}
		for (int i = 1; i < lr[0]; i += 2)
		{
			x_percent = (uint32)lr[i];
			y_percent = (uint32)lr[i + 1];
		}

		x = x_percent > 0 ? ((uint32)size_x * x_percent) / 100u : 0;
		y = y_percent > 0 ? ((uint32)size_y * y_percent) / 100u : 0;

		r.bottom_right = koord(x, y);

		delete[] ul;
		delete[] lr;

		sprintf(upper_left, "region_upper_left[%i]", i);
		sprintf(lower_right, "region_lower_right[%i]", i);

		// Hard coded values override percentages where specified
		if (contents.get_int(upper_left, 65536) < 65536 && contents.get_int(lower_right, 65536) < 65536)
		{
			absolute_regions = true;
			ul = contents.get_ints(upper_left);
			lr = contents.get_ints(lower_right);

			if ((ul[0] & 1) == 1)
			{
				dbg->message("void settings_t::parse_simuconf(", "Ill formed line in config/simuconf.tab.\nWill use default value. Format is region_upper_left[percent]=x,y");
				break;
			}
			for (int i = 1; i < ul[0]; i += 2)
			{
				x = (uint32)ul[i];
				y = (uint32)ul[i + 1];
			}

			r.top_left = koord(x, y);

			if ((lr[0] & 1) == 1)
			{
				dbg->message("void settings_t::parse_simuconf(", "Ill formed line in config/simuconf.tab.\nWill use default value. Format is region_upper_left[percent]=x,y");
				break;
			}
			for (int i = 1; i < lr[0]; i += 2)
			{
				x = (uint32)lr[i];
				y = (uint32)lr[i + 1];
			}

			r.bottom_right = koord(x, y);

			delete[] ul;
			delete[] lr;
		}

		r.name = region_name;

		regions.append(r);
	}

	/*
	 * Selection of savegame format through inifile
	 */
	str = contents.get("saveformat" );
	while (*str == ' ') str++;
	if (strcmp(str, "binary") == 0) {
		loadsave_t::set_savemode(loadsave_t::binary );
	}
	else if(strcmp(str, "zipped") == 0) {
		loadsave_t::set_savemode(loadsave_t::zipped );
	}
	else if(strcmp(str, "xml") == 0) {
		loadsave_t::set_savemode(loadsave_t::xml );
	}
	else if(strcmp(str, "xml_zipped") == 0) {
		loadsave_t::set_savemode(loadsave_t::xml_zipped );
	}
	else if(strcmp(str, "bzip2") == 0) {
		loadsave_t::set_savemode(loadsave_t::bzip2 );
	}
	else if(strcmp(str, "xml_bzip2") == 0) {
		loadsave_t::set_savemode(loadsave_t::xml_bzip2 );
	}

	str = contents.get("autosaveformat" );
	while (*str == ' ') str++;
	if (strcmp(str, "binary") == 0) {
		loadsave_t::set_autosavemode(loadsave_t::binary );
	}
	else if(strcmp(str, "zipped") == 0) {
		loadsave_t::set_autosavemode(loadsave_t::zipped );
	}
	else if(strcmp(str, "xml") == 0) {
		loadsave_t::set_autosavemode(loadsave_t::xml );
	}
	else if(strcmp(str, "xml_zipped") == 0) {
		loadsave_t::set_autosavemode(loadsave_t::xml_zipped );
	}
	else if(strcmp(str, "bzip2") == 0) {
		loadsave_t::set_autosavemode(loadsave_t::bzip2 );
	}
	else if(strcmp(str, "xml_bzip2") == 0) {
		loadsave_t::set_autosavemode(loadsave_t::xml_bzip2 );
	}

	/*
	 * Default resolution
	 */
	disp_width = contents.get_int("display_width", disp_width );
	disp_height = contents.get_int("display_height", disp_height );
	fullscreen = contents.get_int("fullscreen", fullscreen );

	with_private_paks = contents.get_int("with_private_paks", with_private_paks)!=0;

	// Default pak file path
	objfilename = ltrim(contents.get_string("pak_file_path", ""));

	printf("Reading simuconf.tab successful!\n" );

	simuconf.close( );
}


int settings_t::get_name_language_id() const
{
	int lang = -1;
	if(  env_t::networkmode  ) {
		lang = translator::get_language( language_code_names );
	}
	if(  lang == -1  ) {
		lang = translator::get_language();
	}
	return lang;
}

void settings_t::set_groesse(sint32 x, sint32 y, bool preserve_regions)
{
	sint32 old_x = size_x;
	sint32 old_y = size_y;

	size_x = x;
	size_y = y;
	if (!preserve_regions)
	{
		reset_regions(old_x, old_y);
	}
}

void settings_t::set_size_x(sint32 g)
{
	sint32 old_x = size_x;
	size_x = g;

	reset_regions(old_x, size_y);
}

void settings_t::set_size_y(sint32 g)
{
	sint32 old_y = size_y;
	size_y = g;

	reset_regions(size_x, old_y);
}

void settings_t::reset_regions(sint32 old_x, sint32 old_y)
{
	if (absolute_regions || (env_t::networkmode && !env_t::server))
	{
		return;
	}

	// Necessary when changing the map size unless regions are specified in absolute
	// rather than relative terms in ther relevant simuconf.tab
	vector_tpl<region_definition_t> temp_regions;
	FOR(vector_tpl<region_definition_t>, region, regions)
	{
		sint32 old_percent_x = (region.top_left.x * 100u) / old_x;
		sint32 old_percent_y = (region.top_left.y * 100u) / old_y;
		region.top_left.x = (size_x * old_percent_x) / 100u;
		region.top_left.y = (size_y * old_percent_y) / 100u;

		old_percent_x = (region.bottom_right.x * 100u) / old_x;
		old_percent_y = (region.bottom_right.y * 100u) / old_y;
		region.bottom_right.x = (size_x * old_percent_x) / 100u;
		region.bottom_right.y = (size_y * old_percent_y) / 100u;
		temp_regions.append(region);
	}

	regions.clear();
	FOR(vector_tpl<region_definition_t>, region, temp_regions)
	{
		regions.append(region);
	}
}

void settings_t::rotate_regions(sint16 y_size)
{
	vector_tpl<region_definition_t> temp_regions;
	FOR(vector_tpl<region_definition_t>, region, regions)
	{
		region.top_left.rotate90(y_size);
		region.bottom_right.rotate90(y_size);
	}

	regions.clear();
	FOR(vector_tpl<region_definition_t>, region, temp_regions)
	{
		regions.append(region);
	}
}


sint64 settings_t::get_starting_money(sint16 const year) const
{
	if(  starting_money>0  ) {
		return starting_money;
	}

	// search entry with startingmoneyperyear[i].year > year
	int i;
	for(  i=0;  i<10;  i++  ) {
		if(startingmoneyperyear[i].year!=0) {
			if (startingmoneyperyear[i].year>year) {
				break;
			}
		}
		else {
			// year is behind the latest given date
			return startingmoneyperyear[i>0 ? i-1 : 0].money;
		}
	}
	if (i==0) {
		// too early: use first entry
		return startingmoneyperyear[0].money;
	}
	else {
		// now: startingmoneyperyear[i-1].year <= year < startingmoneyperyear[i].year
		if (startingmoneyperyear[i-1].interpol) {
			// linear interpolation
			return startingmoneyperyear[i-1].money +
				(startingmoneyperyear[i].money-startingmoneyperyear[i-1].money)
				* (year-startingmoneyperyear[i-1].year) /(startingmoneyperyear[i].year-startingmoneyperyear[i-1].year );
		}
		else {
			// no interpolation
			return startingmoneyperyear[i-1].money;
		}

	}
}


/**
 * returns newest way-desc for road_timeline_t arrays
 * @param road_timeline_t must be an array with at least num_roads elements, no range checks!
 */
static const way_desc_t *get_timeline_road_type( uint16 year, uint16 num_roads, road_timeline_t* roads)
{
	const way_desc_t *desc = NULL;
	const way_desc_t *test;
	for(  int i=0;  i<num_roads;  i++  ) {
		test = way_builder_t::get_desc( roads[i].name, 0 );
		if(  test  )
		{
			if(  roads[i].intro==0  ) {
				// fill in real intro date
				roads[i].intro = test->get_intro_year_month( );
			}
			if(  roads[i].retire==0  ) {
				// fill in real retire date
				roads[i].retire = test->get_retire_year_month( );
				if(  roads[i].retire==0  ) {
					roads[i].retire = NEVER;
				}
			}
			// find newest available
			if(year == 0 || (year>=roads[i].intro && year<roads[i].retire))
			{
				if(desc==0 || desc->get_intro_year_month()<test->get_intro_year_month())
				{
					desc = test;
				}
			}
		}
	}
	return desc;
}


way_desc_t const* settings_t::get_city_road_type(uint16 const year)
{
	return get_timeline_road_type(year, num_city_roads, city_roads);
}


way_desc_t const* settings_t::get_intercity_road_type(uint16 const year)
{
	return get_timeline_road_type(year, num_intercity_roads, intercity_roads);
}


void settings_t::copy_city_road(settings_t const& other)
{
	num_city_roads = other.num_city_roads;
	for(  int i=0;  i<10;  i++  ) {
		city_roads[i] = other.city_roads[i];
	}
}


// returns default player colors for new players
void settings_t::set_default_player_color(player_t* const player) const
{
	karte_ptr_t welt;
	COLOR_VAL color1 = default_player_color[player->get_player_nr()][0];
	if(  color1 == 255  ) {
		if(  default_player_color_random  ) {
			// build a vector with all colors
			minivec_tpl<uint8>all_colors1(28);
			for(  uint8 i=0;  i<28;  i++  ) {
				all_colors1.append(i);
			}
			// remove all used colors
			for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				player_t *test_sp = welt->get_player(i);
				if(  test_sp  &&  player!=test_sp  ) {
					uint8 rem = 1<<(player->get_player_color1()/8);
					if(  all_colors1.is_contained(rem)  ) {
						all_colors1.remove( rem );
					}
				}
				else if(  default_player_color[i][0]!=255  ) {
					uint8 rem = default_player_color[i][0];
					if(  all_colors1.is_contained(rem)  ) {
						all_colors1.remove( rem );
					}
				}
			}
			// now choose a random empty color
			color1 = pick_any(all_colors1);
		}
		else {
			color1 = player->get_player_nr();
		}
	}

	COLOR_VAL color2 = default_player_color[player->get_player_nr()][1];
	if(  color2 == 255  ) {
		if(  default_player_color_random  ) {
			// build a vector with all colors
			minivec_tpl<uint8>all_colors2(28);
			for(  uint8 i=0;  i<28;  i++  ) {
				all_colors2.append(i);
			}
			// remove color1
			all_colors2.remove( color1/8 );
			// remove all used colors
			for(  uint8 i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				player_t *test_sp = welt->get_player(i);
				if(  test_sp  &&  player!=test_sp  ) {
					uint8 rem = 1<<(player->get_player_color2()/8);
					if(  all_colors2.is_contained(rem)  ) {
						all_colors2.remove( rem );
					}
				}
				else if(  default_player_color[i][1]!=255  ) {
					uint8 rem = default_player_color[i][1];
					if(  all_colors2.is_contained(rem)  ) {
						all_colors2.remove( rem );
					}
				}
			}
			// now choose a random empty color
			color2 = pick_any(all_colors2);
		}
		else {
			color2 = player->get_player_nr() + 3;
		}
	}

	player->set_player_color_no_message( color1*8, color2*8 );
}

void settings_t::set_allow_routing_on_foot(bool value)
{
	allow_routing_on_foot = value;
#ifdef MULTI_THREAD
	world()->await_path_explorer();
#endif
	path_explorer_t::refresh_category(0);
}

void settings_t::set_meters_per_tile(uint16 value)
{
	meters_per_tile = value;
	steps_per_km = (1000 * VEHICLE_STEPS_PER_TILE) / meters_per_tile;
	simtime_factor = float32e8_t(meters_per_tile, 1000);
	steps_per_meter = float32e8_t(VEHICLE_STEPS_PER_TILE, meters_per_tile);
	meters_per_step = float32e8_t(meters_per_tile, VEHICLE_STEPS_PER_TILE);

	// As simspeed2ms = meters_per_yard / seconds_per_tick
	// seconds_per_tick = meters_per_step / yards_per_step / simspeed2ms
	seconds_per_tick = meters_per_step / ( (1<<YARDS_PER_VEHICLE_STEP_SHIFT) * simspeed2ms);
}

void settings_t::set_scale()
{
	if(unit_reverse_time_seconds < 65535)
	{
		unit_reverse_time = (uint32)seconds_to_ticks(unit_reverse_time_seconds, meters_per_tile);
	}

	if(hauled_reverse_time_seconds < 65535)
	{
		hauled_reverse_time = (uint32)seconds_to_ticks(hauled_reverse_time_seconds, meters_per_tile);
	}

	if(turntable_reverse_time_seconds < 65535)
	{
		turntable_reverse_time = (uint32)seconds_to_ticks(turntable_reverse_time_seconds, meters_per_tile);
	}

	if (road_reverse_time_seconds < 65535)
	{
		road_reverse_time = (uint32)seconds_to_ticks(road_reverse_time_seconds, meters_per_tile);
	}
}


/**
 * Reload the linear interpolation tables for catering from the settings.
 * @author neroden
 */
void settings_t::cache_catering_revenues() {
	// There is one catering revenue table for each catering level.
	// Catering levels go up to 5.
	//
	// We *do* keep a table for catering level 0; this avoids some funny math.
	// (The catering level 0 table has only the one entry.)
	// The memory overhead is small enough that it's worth doing this "cleanly".
	//
	// This is an expensive operation but is normally done once per game -- unless
	// someone is tweaking the settings at runtime using the dialog boxes.
	//
	// PLEASE NOTE.  Each table is indexed in *tenths of minutes*,
	// and returns revenues in 1/4096ths of simcents.
	//
	// FIXME: The infamous division by 3 is incorporated here too.
	for (int i = 0; i<= 5; i++) {
		// Clear table, and reset table size to "correct" value (we know how large it needs to be)
		catering_revenues[i].clear(i + 1);
		// Why don't we use a fall-through case statement (you ask)?
		// Because the way the interpolation tables are written, that would involve
		// lots of copying memory to relocate entries.  This enters data from left to right.
		catering_revenues[i].insert(catering_min_minutes * 10, 0);
		if (i >= 1) {
			catering_revenues[i].insert(catering_level1_minutes * 10, (sint64)catering_level1_max_revenue * 4096ll / 3ll);
		}
		if (i >= 2) {
			catering_revenues[i].insert(catering_level2_minutes * 10, (sint64)catering_level2_max_revenue * 4096ll / 3ll);
		}
		if (i >= 3) {
			catering_revenues[i].insert(catering_level3_minutes * 10, (sint64)catering_level3_max_revenue * 4096ll / 3ll);
		}
		if (i >= 4) {
			catering_revenues[i].insert(catering_level4_minutes * 10, (sint64)catering_level4_max_revenue * 4096ll / 3ll);
		}
		if (i >= 5) {
			catering_revenues[i].insert(catering_level5_minutes * 10, (sint64)catering_level5_max_revenue * 4096ll / 3ll);
		}
	}
	// And the TPO revenues.  This is actually a *piecewise constant* table but we can implement it
	// using the linear tables pretty easily...
	// FIXME: The infamous division by 3 is incorporated here too.
	tpo_revenues.clear(2);
	tpo_revenues.insert(tpo_min_minutes * 10 - 1, 0);
	tpo_revenues.insert(tpo_min_minutes * 10, (sint64)tpo_revenue * 4096ll / 3ll);
}


/**
 * Reload the linear interpolation tables for comfort from the settings.
 * @author neroden
 */
void settings_t::cache_comfort_tables() {
	// Tolerable comfort table is indexed in TENTHS of minutes
	tolerable_comfort.clear(5);
	tolerable_comfort.insert(tolerable_comfort_short_minutes * 10, tolerable_comfort_short);
	tolerable_comfort.insert(tolerable_comfort_median_short_minutes * 10, tolerable_comfort_median_short);
	tolerable_comfort.insert(tolerable_comfort_median_median_minutes * 10, tolerable_comfort_median_median);
	tolerable_comfort.insert(tolerable_comfort_median_long_minutes * 10, tolerable_comfort_median_long);
	tolerable_comfort.insert(tolerable_comfort_long_minutes * 10, tolerable_comfort_long);

	// These tables define the bonus or penalty for differing from the tolerable comfort.
	// It is a percentage (yeah, two digits, again)
	// As such this table is indexed by (comfort - tolerable_comfort).  It is signed.
	base_comfort_revenue.clear(3);
	base_comfort_revenue.insert( - (sint16) max_discomfort_penalty_differential, - (sint16) max_discomfort_penalty_percent );
	base_comfort_revenue.insert( 0, 0 );
	base_comfort_revenue.insert( (sint16) max_luxury_bonus_differential, (sint16) max_luxury_bonus_percent );

	// Luxury & discomfort have less of an effect for shorter time periods.
	// This gives the "derating" percentage.  (Yes, it's rounded to two digits.  No, I don't know why.)
	// Again, it's indexed by TENTHS of minutes.
	comfort_derating.clear(2);
	comfort_derating.insert(tolerable_comfort_short_minutes * 10, 20 );
	comfort_derating.insert(tolerable_comfort_median_long_minutes * 10, 100 );

	// The inverse table: given a comfort rating, what's the tolerable journey length?
	// This gives results in SECONDS
	// It is used for display purposes only
	max_tolerable_journey.clear(7);
	// We have to do some finicky tricks at the beginning and end since it isn't constant...
	// There is no tolerable journey below tolerable_comfort_short...
	max_tolerable_journey.insert(tolerable_comfort_short - 1, 0);
	max_tolerable_journey.insert(tolerable_comfort_short, tolerable_comfort_short_minutes * 60);
	max_tolerable_journey.insert(tolerable_comfort_median_short, tolerable_comfort_median_short_minutes * 60);
	max_tolerable_journey.insert(tolerable_comfort_median_median, tolerable_comfort_median_median_minutes * 60);
	max_tolerable_journey.insert(tolerable_comfort_median_long, tolerable_comfort_median_long_minutes * 60);
	max_tolerable_journey.insert(tolerable_comfort_long - 1, tolerable_comfort_long_minutes * 60);
	// Tricky bits at the end.  We fudge the second-to-last section in order to get the final section right.
	// Max tolerable journey is basically infinity above tolerable_comfort_long.
	// Nickname for the largest legal number of journey tenths:
	uint32 infinite_tenths = 65534;
	uint32 infinite_seconds = infinite_tenths * 6;
	max_tolerable_journey.insert(tolerable_comfort_long, infinite_seconds);
}

// Returns *scaled* values.
sint64 settings_t::get_forge_cost(waytype_t wt) const
{
	switch(wt)
	{
	default:
	case road_wt:
		return (forge_cost_road * (sint64)meters_per_tile) / 1000ll;

	case track_wt:
		return (forge_cost_track * (sint64)meters_per_tile) / 1000ll;

	case water_wt:
		return (forge_cost_water * (sint64)meters_per_tile) / 1000ll;

	case monorail_wt:
		return (forge_cost_monorail * (sint64)meters_per_tile) / 1000ll;

	case maglev_wt:
		return (forge_cost_maglev * (sint64)meters_per_tile) / 1000ll;

	case tram_wt:
		return (forge_cost_tram * (sint64)meters_per_tile) / 1000ll;

	case narrowgauge_wt:
		return (forge_cost_narrowgauge * (sint64)meters_per_tile) / 1000ll;

	case air_wt:
		return (forge_cost_air * (sint64)meters_per_tile) / 1000ll;
	};
}

sint64 settings_t::get_parallel_ways_forge_cost_percentage(waytype_t wt) const
{
	switch(wt)
	{
	default:
	case road_wt:
		return parallel_ways_forge_cost_percentage_road;

	case track_wt:
		return parallel_ways_forge_cost_percentage_track;

	case water_wt:
		return parallel_ways_forge_cost_percentage_water;

	case monorail_wt:
		return parallel_ways_forge_cost_percentage_monorail;

	case maglev_wt:
		return parallel_ways_forge_cost_percentage_maglev;

	case tram_wt:
		return parallel_ways_forge_cost_percentage_tram;

	case narrowgauge_wt:
		return parallel_ways_forge_cost_percentage_narrowgauge;

	case air_wt:
		return parallel_ways_forge_cost_percentage_air;
	};
}

void settings_t::calc_job_replenishment_ticks()
{
	job_replenishment_ticks = ((1LL << bits_per_month) * (sint64)get_job_replenishment_per_hundredths_of_months()) / 100ll;
}
