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
#include "../bauer/wegbauer.h"
#include "../descriptor/way_desc.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "loadsave.h"
#include "tabfile.h"
#include "translator.h"

#include "../tpl/minivec_tpl.h"


#define NEVER 0xFFFFU


settings_t::settings_t() :
	filename(""),
	midi_command(""),
	heightfield("")
{
	size_x = 256;
	size_y = 256;

	map_number = 33;

	/* new setting since version 0.85.01
	 * @author prissi
	 */
	factory_count = 12;
	tourist_attractions = 16;

	city_count = 16;
	mean_citizen_count = 1600;

	station_coverage_size = 2;

	traffic_level = 5;

	show_pax = true;

	// default maximum length of convoi
	max_rail_convoi_length = 24;
	max_road_convoi_length = 4;
	max_ship_convoi_length = 4;
	max_air_convoi_length = 1;

	world_maximum_height = 32;
	world_minimum_height = -12;

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

	lake = true;	// if set lakes will be added to map

	// some settings more
	allow_player_change = true;
	use_timeline = 2;
	starting_year = 1930;
	starting_month = 0;
	bits_per_month = 20;

	beginner_mode = false;
	beginner_price_factor = 1500;

	rotation = 0;

	origin_x = origin_y = 0;

	// passenger manipulation factor (=16 about old value)
	passenger_factor = 16;

	// town growth factors
	passenger_multiplier = 40;
	mail_multiplier = 20;
	goods_multiplier = 20;
	electricity_multiplier = 0;

	// Also there are size dependent factors (0 causes crash !)
	growthfactor_small = 400;
	growthfactor_medium = 200;
	growthfactor_large = 100;

	minimum_city_distance = 16;
	industry_increase = 2000;

	special_building_distance = 3;

	factory_worker_percentage = 33;
	tourist_percentage = 16;
	for(  int i=0; i<10; i++  ) {
		locality_factor_per_year[i].year = 0;
		locality_factor_per_year[i].factor = 0;
	}
	locality_factor_per_year[0].factor = 100;

	factory_worker_radius = 77;
	// try to have at least a single town connected to a factory
	factory_worker_minimum_towns = 1;
	// not more than four towns should supply to a factory
	factory_worker_maximum_towns = 4;

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

	just_in_time = env_t::just_in_time;

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
	no_routing_over_overcrowding = false;

	bonus_basefactor = 125;

	/* multiplier for steps on diagonal:
	 * 1024: TT-like, factor 2, vehicle will be too long and too fast
	 * 724: correct one, factor sqrt(2)
	 */
	pak_diagonal_multiplier = 724;

	// read default from env_t
	// should be set in simmain.cc (taken from pak-set simuconf.tab
	way_height_clearance = env_t::default_settings.get_way_height_clearance();
	if (way_height_clearance < 0  ||  way_height_clearance >2) {
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
	allow_merge_distant_halt = 2;
	cst_multiply_merge_halt=-50000;
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

	// this will pay for distance to next change station
	pay_for_total_distance = TO_PREVIOUS;

	avoid_overcrowding = false;

	allow_buying_obsolete_vehicles = true;

	// default: load also private extensions of the pak file
	with_private_paks = true;

	used_vehicle_reduction = 0;

	// some network thing to keep client in sync
	random_counter = 0;	// will be set when actually saving
	frames_per_second = 20;
	frames_per_step = 4;
	server_frames_ahead = 4;
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

	if(file->is_version_less(86, 0)) {
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
		mean_citizen_count = 1600;
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
		beginner_mode = false;
		rotation = 0;
	}
	else {
		// newer versions
		file->rdwr_long(size_x );
		file->rdwr_long(map_number );

		// industries
		file->rdwr_long(factory_count );
		if(file->is_version_less(99, 18)) {
			uint32 dummy;	// was city chains
			file->rdwr_long(dummy );
		}
		else {
			file->rdwr_long( electric_promille );
		}
		file->rdwr_long(tourist_attractions );

		// now towns
		file->rdwr_long(mean_citizen_count );
		file->rdwr_long(city_count );

		// rest
		if(file->is_version_less(101, 0)) {
			uint32 dummy;	// was scroll dir
			file->rdwr_long(dummy );
		}
		file->rdwr_long(traffic_level );
		file->rdwr_long(show_pax );
		sint32 dummy = groundwater;
		file->rdwr_long(dummy );
		if(file->is_version_less(99, 5)) {
			groundwater = (sint16)(dummy/16);
		}
		else {
			groundwater = (sint16)dummy;
		}
		file->rdwr_double(max_mountain_height );
		file->rdwr_double(map_roughness );

		if(file->is_version_atleast(86, 3)) {
			dummy = station_coverage_size;
			file->rdwr_long(dummy );
			station_coverage_size = (uint16)dummy;
		}

		if(file->is_version_atleast(86, 6)) {
			// handle also size on y direction
			file->rdwr_long(size_y );
		}
		else {
			size_y = size_x;
		}

		if(file->is_version_atleast(86, 11)) {
			// some more settings
			file->rdwr_byte(allow_player_change );
			file->rdwr_byte(use_timeline );
			file->rdwr_short(starting_year );
		}
		else {
			allow_player_change = 1;
			use_timeline = 1;
			starting_year = 1930;
		}

		if(file->is_version_atleast(88, 5)) {
			file->rdwr_short(bits_per_month );
		}
		else {
			bits_per_month = 18;
		}

		if(file->is_version_atleast(89, 3)) {
			file->rdwr_bool(beginner_mode );
		}
		else {
			beginner_mode = false;
		}
		if(  file->is_version_atleast(120, 1)  ){
			file->rdwr_byte( just_in_time );
		}
		else if(  file->is_version_atleast(89, 4)  ) {
			bool compat = just_in_time > 0;
			file->rdwr_bool( compat );
			just_in_time = 0;
			if(  compat  ) {
				just_in_time = env_t::just_in_time ? env_t::just_in_time : 1;
			}
		}
		// rotation of the map with respect to the original value
		if(file->is_version_atleast(99, 15)) {
			file->rdwr_byte(rotation );
		}
		else {
			rotation = 0;
		}

		// clear the name when loading ...
		if(file->is_loading()) {
			filename = "";
		}

		// climate borders
		if(file->is_version_atleast(91, 0)) {
			if(  file->is_version_less(120, 6)  ) {
				climate_borders[arctic_climate] -= groundwater;
			}
			for(  int i=0;  i<8;  i++ ) {
				file->rdwr_short(climate_borders[i] );
			}
			if(  file->is_version_less(120, 6)  ) {
				climate_borders[arctic_climate] += groundwater;
			}
			file->rdwr_short(winter_snowline );
		}

		if(  file->is_loading()  &&  file->is_version_less(112, 7)  ) {
			groundwater *= env_t::pak_height_conversion_factor;
			for(  int i = 0;  i < 8;  i++  ) {
				climate_borders[i] *= env_t::pak_height_conversion_factor;
			}
			winter_snowline *= env_t::pak_height_conversion_factor;
			way_height_clearance = env_t::pak_height_conversion_factor;
		}

		// since vehicle will need realignment afterwards!
		if(file->is_version_less(99, 19)) {
			vehicle_base_t::set_diagonal_multiplier( pak_diagonal_multiplier, 1024 );
		}
		else {
			uint16 old_multiplier = pak_diagonal_multiplier;
			file->rdwr_short( old_multiplier );
			vehicle_base_t::set_diagonal_multiplier( pak_diagonal_multiplier, old_multiplier );
			// since vehicle will need realignment afterwards!
		}

		if(file->is_version_atleast(101, 0)) {
			// game mechanics
			file->rdwr_short(origin_x );
			file->rdwr_short(origin_y );

			file->rdwr_long(passenger_factor );

			// town grow stuff
			if(file->is_version_atleast(102, 2)) {
				file->rdwr_long(passenger_multiplier );
				file->rdwr_long(mail_multiplier );
				file->rdwr_long(goods_multiplier );
				file->rdwr_long(electricity_multiplier );
				file->rdwr_long(growthfactor_small );
				file->rdwr_long(growthfactor_medium );
				file->rdwr_long(growthfactor_large );
				file->rdwr_short(factory_worker_percentage );
				file->rdwr_short(tourist_percentage );
				file->rdwr_short(factory_worker_radius );
			}

			file->rdwr_long(electric_promille );

			file->rdwr_short(min_factory_spacing );
			file->rdwr_bool(crossconnect_factories );
			file->rdwr_short(crossconnect_factor );

			file->rdwr_bool(random_pedestrians );
			file->rdwr_long(stadtauto_duration );

			file->rdwr_bool(numbered_stations );
			if(  file->is_version_less(102, 3)  ) {
				if(  file->is_loading()  ) {
					num_city_roads = 1;
					city_roads[0].intro = 0;
					city_roads[0].retire = 0;
					// intercity roads were not saved in old savegames
					num_intercity_roads = 0;
				}
				file->rdwr_str(city_roads[0].name, lengthof(city_roads[0].name) );
			}
			else {
				// several roads ...
				file->rdwr_short(num_city_roads );
				if(  num_city_roads>=10  ) {
					dbg->fatal("settings_t::rdwr()", "Too many (%i) city roads!", num_city_roads);
				}
				for(  int i=0;  i<num_city_roads;  i++  ) {
					file->rdwr_str(city_roads[i].name, lengthof(city_roads[i].name) );
					file->rdwr_short(city_roads[i].intro );
					file->rdwr_short(city_roads[i].retire );
				}
				// several intercity roads ...
				file->rdwr_short(num_intercity_roads );
				if(  num_intercity_roads>=10  ) {
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
			for(  int i=0;  i<15;  i++  ) {
				file->rdwr_bool(player_active[i] );
				file->rdwr_byte(player_type[i] );
				if(  file->is_version_less(102, 3)  ) {
					char dummy[2] = { 0, 0 };
					file->rdwr_str(dummy, lengthof(dummy) );
				}
			}

			// cost section ...
			file->rdwr_bool(freeplay );
			if(  file->is_version_atleast(102, 3)  ) {
				file->rdwr_longlong(starting_money );
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
			if(  file->is_version_less(102, 2)  ) {
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

			if(  file->is_version_atleast(120, 3)  ) {
				file->rdwr_longlong(cst_make_public_months);
			}

			if(  file->is_version_atleast(120, 9)  ) {
				file->rdwr_longlong(cst_multiply_merge_halt);
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

		if(file->is_version_atleast(101, 1)) {
			file->rdwr_bool( separate_halt_capacities );
			file->rdwr_byte( pay_for_total_distance );

			file->rdwr_short(starting_month );

			file->rdwr_short( river_number );
			file->rdwr_short( min_river_length );
			file->rdwr_short( max_river_length );
		}

		if(file->is_version_atleast(102, 1)) {
			file->rdwr_bool( avoid_overcrowding );
		}
		if(file->is_version_atleast(102, 2)) {
			file->rdwr_bool( no_routing_over_overcrowding );
			file->rdwr_bool( with_private_paks );
		}
		if(file->is_version_atleast(102, 3)) {
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
			file->rdwr_bool( allow_buying_obsolete_vehicles );
			file->rdwr_long( factory_worker_minimum_towns );
			file->rdwr_long( factory_worker_maximum_towns );
			// forest stuff
			file->rdwr_byte( forest_base_size );
			file->rdwr_byte( forest_map_size_divisor );
			file->rdwr_byte( forest_count_divisor );
			file->rdwr_short( forest_inverse_spare_tree_density );
			file->rdwr_byte( max_no_of_trees_on_square );
			file->rdwr_short( tree_climates );
			file->rdwr_short( no_tree_climates );
			file->rdwr_bool( no_trees );
			file->rdwr_long( minimum_city_distance );
			file->rdwr_long( industry_increase );
		}
		if(  file->is_version_atleast(110, 0)  ) {
			if(  !env_t::networkmode  ||  env_t::server  ) {
				server_frames_ahead = env_t::server_frames_ahead;
			}
			file->rdwr_long( server_frames_ahead );
			if(  !env_t::networkmode  ||  env_t::server  ) {
				server_frames_ahead = env_t::server_frames_ahead;
			}
			file->rdwr_short( used_vehicle_reduction );
		}

		if(  file->is_version_atleast(110, 1)  ) {
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

		if(  file->is_version_atleast(110, 5)  ) {
			file->rdwr_short(factory_arrival_periods);
			file->rdwr_bool(factory_enforce_demand);
		}

		if(  file->is_version_atleast(110, 7)  ) {
			for(  int i=0;  i<10;  i++  ) {
				file->rdwr_short(locality_factor_per_year[i].year );
				file->rdwr_long(locality_factor_per_year[i].factor );
			}
			file->rdwr_bool( drive_on_left );
			file->rdwr_bool( signals_on_left );
			file->rdwr_long( way_toll_runningcost_percentage );
			file->rdwr_long( way_toll_waycost_percentage );
		}

		if(  file->is_version_atleast(111, 2)  ) {
			file->rdwr_long( bonus_basefactor );
		}
		else if(  file->is_loading()  ) {
			bonus_basefactor = 125;
		}

		if(  file->is_version_atleast(111, 4)  ) {
			file->rdwr_bool( allow_underground_transformers );
		}

		if(  file->is_version_atleast(111, 5)  ) {
			file->rdwr_short( special_building_distance );
		}

		if(  file->is_version_atleast(112, 1)  ) {
			file->rdwr_short( factory_maximum_intransit_percentage );
		}

		if(  file->is_version_atleast(112, 2)  ) {
			file->rdwr_short( remove_dummy_player_months );
			file->rdwr_short( unprotect_abandoned_player_months );
		}

		if(  file->is_version_atleast(112, 3)  ) {
			file->rdwr_short( max_factory_spacing );
			file->rdwr_short( max_factory_spacing_percentage );
		}
		if(  file->is_version_atleast(112, 8)  ) {
			file->rdwr_longlong( cst_alter_climate );
			file->rdwr_byte( way_height_clearance );
		}
		if(  file->is_version_atleast(120, 2)  ) {
			file->rdwr_long( default_ai_construction_speed );
		}
		else if(  file->is_loading()  ) {
			default_ai_construction_speed = env_t::default_ai_construction_speed;
		}
		if(  file->is_version_atleast(120, 2) ) {
			file->rdwr_bool(lake);
			file->rdwr_bool(no_trees);
			file->rdwr_long( max_choose_route_steps );
		}
		if(  file->is_version_atleast(120, 4)  ) {
			file->rdwr_bool(disable_make_way_public);
		}
		if(  file->is_version_atleast(120, 6)  ) {
			file->rdwr_byte(max_rail_convoi_length);
			file->rdwr_byte(max_road_convoi_length);
			file->rdwr_byte(max_ship_convoi_length);
			file->rdwr_byte(max_air_convoi_length);
		}
		if(  file->is_version_atleast(120, 7) ) {
			file->rdwr_byte(world_maximum_height);
			file->rdwr_byte(world_minimum_height);
		}
		if(  file->is_version_atleast(120, 9)  ) {
			file->rdwr_long(allow_merge_distant_halt);
		}
		// otherwise the default values of the last one will be used
	}
}


// read the settings from this file
void settings_t::parse_simuconf(tabfile_t& simuconf, sint16& disp_width, sint16& disp_height, sint16 &fullscreen, std::string& objfilename)
{
	tabfileobj_t contents;

	simuconf.read(contents );

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

	//check for fontname, must be a valid name!
	const char *fname = contents.get_string( "fontname", env_t::fontname.c_str() );
	if(  FILE *f=fopen(fname,"r")  ) {
		fclose(f);
		env_t::fontname = fname;
	}

	env_t::midi_command = contents.get_string( "midi_command", env_t::midi_command.c_str());
	env_t::water_animation = contents.get_int("water_animation_ms", env_t::water_animation );
	env_t::ground_object_probability = contents.get_int("random_grounds_probability", env_t::ground_object_probability );
	env_t::moving_object_probability = contents.get_int("random_wildlife_probability", env_t::moving_object_probability );

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

	env_t::show_tooltips = contents.get_int("show_tooltips", env_t::show_tooltips );
	env_t::tooltip_delay = contents.get_int("tooltip_delay", env_t::tooltip_delay );
	env_t::tooltip_duration = contents.get_int("tooltip_duration", env_t::tooltip_duration );
	env_t::toolbar_max_width = contents.get_int("toolbar_max_width", env_t::toolbar_max_width );
	env_t::toolbar_max_height = contents.get_int("toolbar_max_height", env_t::toolbar_max_height );

	// how to show the stuff outside the map
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
	env_t::follow_convoi_underground = contents.get_int("follow_convoi_underground",env_t::follow_convoi_underground );

	env_t::hide_rail_return_ticket = contents.get_int("hide_rail_return_ticket",env_t::hide_rail_return_ticket ) != 0;
	env_t::show_delete_buttons = contents.get_int("show_delete_buttons",env_t::show_delete_buttons ) != 0;
	env_t::chat_window_transparency = contents.get_int("chat_transparency",env_t::chat_window_transparency );

	env_t::hide_keyboard = contents.get_int("hide_keyboard",env_t::hide_keyboard ) != 0;

	env_t::player_finance_display_account = contents.get_int("player_finance_display_account",env_t::player_finance_display_account ) != 0;

	// network stuff
	env_t::server_frames_ahead = contents.get_int("server_frames_ahead", env_t::server_frames_ahead );
	env_t::additional_client_frames_behind = contents.get_int("additional_client_frames_behind", env_t::additional_client_frames_behind);
	env_t::network_frames_per_step = contents.get_int("server_frames_per_step", env_t::network_frames_per_step );
	env_t::server_sync_steps_between_checks = contents.get_int("server_frames_between_checks", env_t::server_sync_steps_between_checks );
	env_t::pause_server_no_clients = contents.get_int("pause_server_no_clients", env_t::pause_server_no_clients );
	env_t::server_save_game_on_quit = contents.get_int("server_save_game_on_quit", env_t::server_save_game_on_quit );
	env_t::reload_and_save_on_quit = contents.get_int("reload_and_save_on_quit", env_t::reload_and_save_on_quit );

	env_t::server_announce = contents.get_int("announce_server", env_t::server_announce );
	if (!env_t::server) {
		env_t::server_port = contents.get_int("server_port", env_t::server_port);
	}
	else {
		env_t::server_port = env_t::server;
	}
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
	if(  *contents.get("server_altdns")  ) {
		env_t::server_alt_dns = ltrim(contents.get("server_altdns"));
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

	// new: up to ten city_roads are possible
	if(  *contents.get("city_road[0]")  ) {
		// renew them always when a table is encountered ...
		num_city_roads = 0;
		for(  int i = 0;  i<10;  i++  ) {
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

	// new: up to ten intercity_roads are possible
	if(  *contents.get("intercity_road[0]")  ) {
		// renew them always when a table is encountered ...
		num_intercity_roads = 0;
		for(  int i = 0;  i<10;  i++  ) {
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

	// routing stuff
	max_route_steps = contents.get_int("max_route_steps", max_route_steps );
	max_choose_route_steps = contents.get_int("max_choose_route_steps", max_choose_route_steps );
	max_hops = contents.get_int("max_hops", max_hops );
	max_transfers = contents.get_int("max_transfers", max_transfers );
	bonus_basefactor = contents.get_int("bonus_basefactor", bonus_basefactor );

	special_building_distance = contents.get_int("special_building_distance", special_building_distance );
	minimum_city_distance = contents.get_int("minimum_city_distance", minimum_city_distance );
	industry_increase = contents.get_int("industry_increase_every", industry_increase );
	passenger_factor = contents.get_int("passenger_factor", passenger_factor ); /* this can manipulate the passenger generation */
	factory_worker_percentage = contents.get_int("factory_worker_percentage", factory_worker_percentage );
	factory_worker_radius = contents.get_int("factory_worker_radius", factory_worker_radius );
	factory_worker_minimum_towns = contents.get_int("factory_worker_minimum_towns", factory_worker_minimum_towns );
	factory_worker_maximum_towns = contents.get_int("factory_worker_maximum_towns", factory_worker_maximum_towns );
	factory_arrival_periods = clamp( contents.get_int("factory_arrival_periods", factory_arrival_periods), 1, 16 );
	factory_enforce_demand = contents.get_int("factory_enforce_demand", factory_enforce_demand) != 0;
	factory_maximum_intransit_percentage  = contents.get_int("maximum_intransit_percentage", factory_maximum_intransit_percentage);

	tourist_percentage = contents.get_int("tourist_percentage", tourist_percentage );
	// .. read twice: old and right spelling
	separate_halt_capacities = contents.get_int("seperate_halt_capacities", separate_halt_capacities ) != 0;
	separate_halt_capacities = contents.get_int("separate_halt_capacities", separate_halt_capacities ) != 0;

	pay_for_total_distance = contents.get_int("pay_for_total_distance", pay_for_total_distance );
	avoid_overcrowding = contents.get_int("avoid_overcrowding", avoid_overcrowding )!=0;
	no_routing_over_overcrowding = contents.get_int("no_routing_over_overcrowded", no_routing_over_overcrowding )!=0;

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
	traffic_level = contents.get_int("citycar_level", traffic_level );	// ten normal years
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
		sint64 *test = contents.get_sint64s( name );
		if(  (test[0]>1) && (test[0]<=3)  ) {
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

	/* up to ten blocks year, locality_factor={0...2000000000}
	 * locality_factor[i]=y,l
	 * y .. year
	 * l .. factor, the larger the more widespread
	 */
	j = 0;
	for(  int i = 0;  i<10;  i++  ) {
		char name[256];
		sprintf( name, "locality_factor[%i]", i );
		sint64 *test = contents.get_sint64s( name );
		// two arguments, and then factor natural number
		if(  test[0]==2  ) {
			if(  test[2]<=0  ) {
				dbg->error("Parameter in simuconf.tab wrong!","locality_factor second value must be larger than zero!" );
			}
			else {
				// insert sorted by years
				int k=0;
				for(  k=0; k<i; k++  ) {
					if(  locality_factor_per_year[k].year > test[1]  ) {
						for (int l=j; l>=k; l--)
							memcpy( &locality_factor_per_year[l+1], &locality_factor_per_year[l], sizeof(yearly_locality_factor_t) );
						break;
					}
				}
				locality_factor_per_year[k].year = (sint16)test[1];
				locality_factor_per_year[k].factor = (uint32)test[2];
				j++;
			}
		}
		else {
			// invalid entry
		}
		delete [] test;
	}
	// add default, if nothing found
	if(  j==0  &&  locality_factor_per_year[0].factor==0  ) {
		locality_factor_per_year[0].year = 0;
		locality_factor_per_year[0].factor = 100;
		j++;
	}
	// fill remaining entries
	while(  j>0  &&  j<9  ) {
		j++;
		locality_factor_per_year[j].year = 0;
		locality_factor_per_year[j].factor = 0;
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

	maint_building = contents.get_int("maintenance_building", maint_building );

	numbered_stations = contents.get_int("numbered_stations", numbered_stations );
	station_coverage_size = contents.get_int("station_coverage", station_coverage_size );

	// time stuff
	bits_per_month = contents.get_int("bits_per_month", bits_per_month );
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
	int bounds = (way_height_clearance!=0);
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

	env_t::just_in_time = (uint8)contents.get_int("just_in_time", env_t::just_in_time);
	if( env_t::just_in_time > 2 ) {
		env_t::just_in_time = 2; // Range restriction.
	}
	just_in_time = env_t::just_in_time;
	beginner_price_factor = contents.get_int("beginner_price_factor", beginner_price_factor ); /* this manipulates the good prices in beginner mode */
	beginner_mode = contents.get_int("first_beginner", beginner_mode ); /* start in beginner mode */

	way_toll_runningcost_percentage = contents.get_int("toll_runningcost_percentage", way_toll_runningcost_percentage );
	way_toll_waycost_percentage = contents.get_int("toll_waycost_percentage", way_toll_waycost_percentage );

	/* now the cost section */
	cst_multiply_dock = contents.get_int64("cost_multiply_dock", cst_multiply_dock/(-100) ) * -100;
	cst_multiply_station = contents.get_int64("cost_multiply_station", cst_multiply_station/(-100) ) * -100;
	cst_multiply_roadstop = contents.get_int64("cost_multiply_roadstop", cst_multiply_roadstop/(-100) ) * -100;
	cst_multiply_airterminal = contents.get_int64("cost_multiply_airterminal", cst_multiply_airterminal/(-100) ) * -100;
	cst_multiply_post = contents.get_int64("cost_multiply_post", cst_multiply_post/(-100) ) * -100;
	cst_multiply_headquarter = contents.get_int64("cost_multiply_headquarter", cst_multiply_headquarter/(-100) ) * -100;
	cst_depot_air = contents.get_int64("cost_depot_air", cst_depot_air/(-100) ) * -100;
	cst_depot_rail = contents.get_int64("cost_depot_rail", cst_depot_rail/(-100) ) * -100;
	cst_depot_road = contents.get_int64("cost_depot_road", cst_depot_road/(-100) ) * -100;
	cst_depot_ship = contents.get_int64("cost_depot_ship", cst_depot_ship/(-100) ) * -100;

	allow_merge_distant_halt = contents.get_int("allow_merge_distant_halt", allow_merge_distant_halt);
	cst_multiply_merge_halt = contents.get_int64("cost_multiply_merge_halt", cst_multiply_merge_halt/(-100) ) * -100;

	// alter landscape
	cst_buy_land = contents.get_int64("cost_buy_land", cst_buy_land/(-100) ) * -100;
	cst_alter_land = contents.get_int64("cost_alter_land", cst_alter_land/(-100) ) * -100;
	cst_set_slope = contents.get_int64("cost_set_slope", cst_set_slope/(-100) ) * -100;
	cst_alter_climate = contents.get_int64("cost_alter_climate", cst_alter_climate/(-100) ) * -100;
	cst_found_city = contents.get_int64("cost_found_city", cst_found_city/(-100) ) * -100;
	cst_multiply_found_industry = contents.get_int64("cost_multiply_found_industry", cst_multiply_found_industry/(-100) ) * -100;
	cst_remove_tree = contents.get_int64("cost_remove_tree", cst_remove_tree/(-100) ) * -100;
	cst_multiply_remove_haus = contents.get_int64("cost_multiply_remove_haus", cst_multiply_remove_haus/(-100) ) * -100;
	cst_multiply_remove_field = contents.get_int64("cost_multiply_remove_field", cst_multiply_remove_field/(-100) ) * -100;
	// powerlines
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

	max_rail_convoi_length = contents.get_int("max_rail_convoi_length",max_rail_convoi_length);
	max_road_convoi_length = contents.get_int("max_road_convoi_length",max_road_convoi_length);
	max_ship_convoi_length = contents.get_int("max_ship_convoi_length",max_ship_convoi_length);
	max_air_convoi_length = contents.get_int("max_air_convoi_length",max_air_convoi_length);

	world_maximum_height = contents.get_int("world_maximum_height",world_maximum_height);
	world_minimum_height = contents.get_int("world_minimum_height",world_minimum_height);
	if(  world_minimum_height>=world_maximum_height  ) {
		world_minimum_height = world_maximum_height-1;
	}

	// Default pak file path
	objfilename = ltrim(contents.get_string("pak_file_path", "" ) );

	printf("Reading simuconf.tab successful!\n" );
}

// colour stuff can only be parsed when the graphic system has already started
void settings_t::parse_colours(tabfile_t& simuconf)
{
	tabfileobj_t contents;

	simuconf.read( contents );

	env_t::default_window_title_color = contents.get_color("default_window_title_color", env_t::default_window_title_color, &env_t::default_window_title_color_rgb );
	env_t::front_window_text_color = contents.get_color("front_window_text_color", env_t::front_window_text_color, &env_t::front_window_text_color_rgb );
	env_t::bottom_window_text_color = contents.get_color("bottom_window_text_color", env_t::bottom_window_text_color, &env_t::bottom_window_text_color_rgb );

	env_t::bottom_window_darkness = contents.get_int("env_t::bottom_window_darkness", env_t::bottom_window_darkness);

	env_t::tooltip_color = contents.get_color("tooltip_background_color", env_t::tooltip_color, &env_t::tooltip_color_rgb );
	env_t::tooltip_textcolor = contents.get_color("tooltip_text_color", env_t::tooltip_textcolor, &env_t::tooltip_textcolor_rgb );
	env_t::cursor_overlay_color = contents.get_color("cursor_overlay_color", env_t::cursor_overlay_color, &env_t::cursor_overlay_color_rgb );

	env_t::background_color = contents.get_color("background_color", env_t::background_color, &env_t::background_color_rgb );
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


uint32 settings_t::get_locality_factor(sint16 const year) const
{
	int i;
	for(  i=0;  i<10;  i++  ) {
		if(  locality_factor_per_year[i].year!=0  ) {
			if(  locality_factor_per_year[i].year > year  ) {
				break;
			}
		}
		else {
			// year is behind the latest given date
			return locality_factor_per_year[max(i-1,0)].factor;
		}
	}
	if(  i==0  ) {
		// too early: use first entry
		return locality_factor_per_year[0].factor;
	}
	else {
#if 0
		// linear interpolation
		return locality_factor_per_year[i-1].factor +
			(locality_factor_per_year[i].factor-locality_factor_per_year[i-1].factor)
			* (year-locality_factor_per_year[i-1].year) /(locality_factor_per_year[i].year-locality_factor_per_year[i-1].year );
#else
		// exponential evaluation
		sint32 diff = (locality_factor_per_year[i].factor-locality_factor_per_year[i-1].factor);
		double a = exp((year-locality_factor_per_year[i-1].year)/20.0);
		double b = exp((locality_factor_per_year[i].year-locality_factor_per_year[i-1].year)/20.0);
		return locality_factor_per_year[i-1].factor + (sint32)((diff*(a-1.0))/(b-1.0) + 0.5);
#endif
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
		if(  test  ) {
			// return first available for no timeline
			if(  year==0  ) {
				return test;
			}
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
			// find newest available ...
			if(  year>=roads[i].intro  &&  year<roads[i].retire  ) {
				if(  desc==0  ||  desc->get_intro_year_month()<test->get_intro_year_month()  ) {
					desc = test;
				}
			}
		}
	}
	return desc;
}


way_desc_t const* settings_t::get_city_road_type(uint16 const year)
{
	return get_timeline_road_type(year, num_city_roads, city_roads );
}


way_desc_t const* settings_t::get_intercity_road_type(uint16 const year)
{
	return get_timeline_road_type(year, num_intercity_roads, intercity_roads );
}


void settings_t::copy_city_road(settings_t const& other)
{
	num_city_roads = other.num_city_roads;
	for(  int i=0;  i<10;  i++  ) {
		city_roads[i] = other.city_roads[i];
	}
}


void settings_t::set_default_player_color(uint8 player_nr, uint8 color1, uint8 color2)
{
	if (player_nr < MAX_PLAYER_COUNT) {
		default_player_color[player_nr][0] = color1 < 28 ? color1 : 255;
		default_player_color[player_nr][1] = color2 < 28 ? color2 : 255;
	}
}


// returns default player colors for new players
void settings_t::set_player_color_to_default(player_t* const player) const
{
	karte_ptr_t welt;
	uint8 color1 = default_player_color[player->get_player_nr()][0];
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

	uint8 color2 = default_player_color[player->get_player_nr()][1];
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

	player->set_player_color( color1*8, color2*8 );
}
