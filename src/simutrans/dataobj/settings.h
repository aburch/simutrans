/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SETTINGS_H
#define DATAOBJ_SETTINGS_H


#include <string>

#include "../simtypes.h"
#include "../simconst.h"

#include "ribi.h"

class player_t;
class loadsave_t;
class tabfile_t;
class way_desc_t;

struct road_timeline_t
{
	char name[64];
	uint16 intro;
	uint16 retire;
};


/**
 * Game settings
 *
 * @note The default values from this class should be kept in sync with the
 *       default values from the default simuconf.tab located in simutrans/config/
 *       (same as for env_t)
 */
class settings_t
{
	// these are the only classes, that are allowed to modify elements from settings_t
	// for all remaining special cases there are the set_...() routines
	friend class settings_general_stats_t;
	friend class settings_routing_stats_t;
	friend class settings_economy_stats_t;
	friend class settings_costs_stats_t;
	friend class settings_frame_t;
	friend class settings_climates_stats_t;
	friend class climate_gui_t;
	friend class welt_gui_t;

public:
	typedef enum {
		HEIGHT_BASED = 0,
		HUMIDITY_BASED,
		MAP_BASED
	} climate_generate_t;

private:
	sint32 size_x     = 256;
	sint32 size_y     = 256;
	sint32 map_number =  33;

	sint32 tourist_attractions =  16;
	sint32 factory_count       =  12;
	sint32 electric_promille   = 330;

	sint32 city_count         =   16;
	sint32 mean_citizen_count = 1600;

	// town growth factors
	sint32 passenger_multiplier   = 40;
	sint32 mail_multiplier        = 20;
	sint32 goods_multiplier       = 20;
	sint32 electricity_multiplier =  0;

	// Also there are size dependent factors (0=no growth)
	sint32 growthfactor_small  = 400;
	sint32 growthfactor_medium = 200;
	sint32 growthfactor_large  = 100;

	sint16 special_building_distance =    3; // distance between attraction to factory or other special buildings
	uint32 minimum_city_distance     =   16;
	uint32 industry_increase         = 2000; // add new factory (chain) when city grows to x * 2^n

	// percentage of routing
	sint16 factory_worker_percentage = 33;
	sint16 tourist_percentage        = 16;

	// higher number: passengers are more evenly distributed around the map
	struct yearly_locality_factor_t
	{
		sint16 year;
		uint32 factor;
	};
	yearly_locality_factor_t locality_factor_per_year[10];

	// radius for factories
	sint16 factory_worker_radius        = 77;
	sint32 factory_worker_minimum_towns =  1; ///< try to have at least a single town connected to a factory ...
	sint32 factory_worker_maximum_towns =  4; ///< ... but not more than 4

	// number of periods for averaging the amount of arrived pax/mail at factories
	uint16 factory_arrival_periods = 4;

	// whether factory pax/mail demands are enforced
	bool factory_enforce_demand = true;

	uint16 station_coverage_size = 2;

	// the maximum length of each convoi
	uint8 max_rail_convoi_length = 24;
	uint8 max_road_convoi_length =  4;
	uint8 max_ship_convoi_length =  4;
	uint8 max_air_convoi_length  =  1;

	/// At which level buildings generate traffic?
	sint32 traffic_level = 5;

	/**
	 * the maximum and minimum allowed world height.
	 */
	sint8 world_maximum_height =  32;
	sint8 world_minimum_height = -12;

	/// @{
	/// @ref set_default_climates
	climate_generate_t climate_generator = climate_generate_t::HEIGHT_BASED;
	sint16 groundwater = -2;
	sint16 winter_snowline = 7; ///< Not mediterranean
	sint16 climate_borders[MAX_CLIMATES][2];
	sint8 climate_temperature_borders[5];
	sint8 tropic_humidity = 75;
	sint8 desert_humidity = 65;

	ribi_t::ribi wind_direction = ribi_t::west; ///< Wind is coming from this direction. Must be single! (N/W/S/E)

	sint8 patch_size_percentage; // average size of a climate patch, if there are overlapping climates

	sint8 moisture; // how much increase of moisture per tile
	sint8 moisture_water; // how much increase of moisture per water tile
	/// @}

	double max_mountain_height = 160.0;  // can be 0-160.0
	double map_roughness       =   0.6;  // can be 0-1

	// river stuff
	sint16 river_number     =  16;
	sint16 min_river_length =  16;
	sint16 max_river_length = 256;

	// forest stuff
	uint8 forest_base_size                   = 36; // Minimal size of forests - map independent
	uint8 forest_map_size_divisor            = 38; // The smaller it is, the larger are individual forests
	uint8 forest_count_divisor               = 16; // The smaller it is, the more forest are generated
	uint16 forest_inverse_spare_tree_density =  5; // Determines how often are spare trees going to be planted (1 per x tiles on avg)
	uint8 max_no_of_trees_on_square          =  3; // 2 - minimal usable, 3 good, 5 very nice looking
	uint16 tree_climates                     =  0; // bit set, if this climate is to be covered with trees entirely
	uint16 no_tree_climates                  =  0; // bit set, if this climate is to be void of random trees

public:
	enum tree_distribution_t
	{
		TREE_DIST_NONE     = 0,
		TREE_DIST_RANDOM   = 1,
		TREE_DIST_RAINFALL = 2,
		TREE_DIST_COUNT
	};

	// server message (if any)
	std::string motd = "";

private:
	uint16 tree_distribution = tree_distribution_t::TREE_DIST_RANDOM;

	sint8 lake_height = 8; // relative to sea (groundwater) height

	// game mechanics
	uint8 allow_player_change = true;
	uint8 use_timeline        =    2;
	sint16 starting_year      = 1930;
	sint16 starting_month     =    0;
	sint16 bits_per_month     =   20;

	std::string filename = "";

	bool beginner_mode = false;
	sint32 beginner_price_factor = 1500; // 1500 = +50%

	/* Industry supply model used.
	 * 0 : Classic (no flow control?)
	 * 1 : JIT Classic (maximum transit and storage limited)
	 * 2 : JIT Version 2 (demand buffers with better consumption model)
	 */
	uint8 just_in_time = 1; // Overwritten by the value from env_t

	// default 0, will be incremented after each 90 degree rotation until 4
	uint8 rotation = 0;

	sint16 origin_x = 0, origin_y = 0;

	// passenger manipulation factor (=16 about old value)
	sint32 passenger_factor = 16;

	sint16 min_factory_spacing = 6;
	sint16 max_factory_spacing = 40;
	sint16 max_factory_spacing_percentage = 0;

	/*no goods will put in route, when stored>gemax_storage and goods_in_transit*maximum_intransit_percentage/100>max_storage  */
	uint16 factory_maximum_intransit_percentage = 0;

	// crossconnect all factories (like OTTD and similar games)
	// These two values are overwritten if OTTD_LIKE is defined
	bool   crossconnect_factories = false;
	sint16 crossconnect_factor    =    33;

	sint32 stadtauto_duration = false;

	bool freeplay = false;

	sint64 starting_money = 20000000;

	struct yearmoney
	{
		sint16 year;
		sint64 money;
		bool interpol;
	};
	yearmoney startingmoneyperyear[10];

	uint16 num_city_roads = 0;
	road_timeline_t city_roads[10];
	uint16 num_intercity_roads = 0;
	road_timeline_t intercity_roads[10];

	// pairs of year,speed
	uint16 city_road_speed_limit_num = 0;
	uint16 city_road_speed_limit[20];

	/**
	 * Use numbering for stations?
	 */
	bool numbered_stations = false;

	/* maximum number of steps for breath search */
	sint32 max_route_steps = 1000000;

	// maximum length for route search at signs/signals
	sint32 max_choose_route_steps = 200;

	// max steps for good routing
	sint32 max_hops = 2000;

	/* maximum number of steps for breath search */
	sint32 max_transfers = 9;

	/* multiplier for steps on diagonal:
	 * 1024: TT-like, factor 2, vehicle will be too long and too fast
	 * 724: correct one, factor sqrt(2)
	 */
	uint16 pak_diagonal_multiplier = 724;

	// names of the stations ...
	char language_code_names[4];

	// true, if the different capacities (passengers/mail/freight) are counted separately
	bool separate_halt_capacities = false;

	/**
	 * payment is only for the distance that got shorter between target and start
	 * three modes:
	 * 0 = pay for travelled manhattan distance
	 * 1 = pay for distance difference to next transfer stop
	 * 2 = pay for distance to destination
	 * 0 allows chaeting, but the income with 1 or two are much lower
	 */
	uint8 pay_for_total_distance = TO_PREVIOUS;

	/* if set, goods will avoid being routed over overcrowded stops */
	bool avoid_overcrowding = false;

	/* if set, goods will not routed over overcrowded stations but rather try detours (if possible) */
	bool no_routing_over_overcrowding = false;

	// lowest possible income with speedbonus (1000=1) default 125
	sint32 bonus_basefactor = 125;

	// true, if this pak should be used with extensions (default=false)
	bool with_private_paks = false;

	/// what is the minimum clearance required under bridges
	/// read default from env_t, should be set in simmain.cc (taken from pak-set simuconf.tab)
	uint8 way_height_clearance = 2;

	// if true, you can buy obsolete stuff
	bool allow_buying_obsolete_vehicles = true;
	// vehicle value is decrease by this factor/1000 when a vehicle leaves the depot
	sint16 used_vehicle_reduction = 0;

	// some network things to keep client in sync
	uint32 random_counter      =  0; // will be set when actually saving
	uint32 frames_per_second   = 20; // only used in network mode
	uint32 frames_per_step     =  4; // for network play
	uint32 server_frames_ahead =  4; // for network play

	bool drive_on_left   = false;
	bool signals_on_left = false;

	// fraction of running costs charged for going on other players way
	sint32 way_toll_runningcost_percentage = 0;
	sint32 way_toll_waycost_percentage     = 0;

	// true if transformers are allowed to built underground
	bool allow_underground_transformers = true;

	// true if companies can make ways public
	bool disable_make_way_public = false;

	// only for trains. If true, trains stop at the position designated in the schdule..
	bool stop_halt_as_scheduled = false;

	// the big cost section
public:
	sint32 maint_building = 5000; // normal building

	sint64 cst_multiply_dock        =  -50000;
	sint64 cst_multiply_station     =  -60000;
	sint64 cst_multiply_roadstop    =  -40000;
	sint64 cst_multiply_airterminal = -300000;
	sint64 cst_multiply_post        =  -30000;
	sint64 cst_multiply_headquarter = -100000;
	sint64 cst_depot_rail           = -100000;
	sint64 cst_depot_road           = -130000;
	sint64 cst_depot_ship           = -250000;
	sint64 cst_depot_air            = -500000;

	// cost to merge station
	uint32 allow_merge_distant_halt = 2;
	sint64 cst_multiply_merge_halt  = -50000;

	// alter landscape
	sint64 cst_buy_land                =     -10000;
	sint64 cst_alter_land              =    -100000;
	sint64 cst_alter_climate           =    -100000;
	sint64 cst_set_slope               =    -250000;
	sint64 cst_found_city              = -500000000;
	sint64 cst_multiply_found_industry =   -2000000;
	sint64 cst_remove_tree             =     -10000;
	sint64 cst_multiply_remove_haus    =    -100000;
	sint64 cst_multiply_remove_field   =    -500000;
	sint64 cst_transformer             =    -250000;
	sint64 cst_maintain_transformer    =      -2000;

	// maintainance cost in months to make something public
	sint64 cst_make_public_months = 60;

	// costs for the way searcher
	sint32 way_count_straight        =    1; // cost on existing way
	sint32 way_count_curve           =    5; // diagonal curve
	sint32 way_count_double_curve    =   10;
	sint32 way_count_90_curve        =   30;
	sint32 way_count_slope           =   20;
	sint32 way_count_tunnel          =   16;
	sint32 way_count_no_way          =    3; // slightly prefer existing ways
	sint32 way_count_avoid_crossings =    8; // prefer less system crossings
	sint32 way_count_leaving_way     =   50;
	sint32 way_count_maximum         = 2000; // limit for allowed ways (can be set lower to avoid covering the whole map with two clicks)
	uint32 way_max_bridge_len        =   15;

	// 0 = empty, otherwise some value from simplay
	uint8 player_type[MAX_PLAYER_COUNT];

	// how fast new AI will build something; value is copied from env_t
	uint32 default_ai_construction_speed = 8000;

	// player color suggestions for new games
	bool default_player_color_random = false;
	uint8 default_player_color[MAX_PLAYER_COUNT][2];

	// remove dummy companies and remove password from abandoned companies
	uint16 remove_dummy_player_months        = 6;
	uint16 unprotect_abandoned_player_months = 0;

public:
	/**
	 * If map is read from a heightfield, this is the name of the heightfield.
	 * Set to empty string in order to avoid loading.
	 */
	std::string heightfield = "";

	settings_t();

	void rdwr(loadsave_t *file);

	void copy_city_road(settings_t const& other);

	// init from this file ...
	void parse_simuconf(tabfile_t& simuconf, sint16& disp_width, sint16& disp_height, sint16& fullscreen, bool set_pak);

	// init without screen parameters ...
	void parse_simuconf(tabfile_t& simuconf) {
		sint16 idummy = 0;
		std::string sdummy;

		parse_simuconf(simuconf, idummy, idummy, idummy, false);
	}

	void parse_colours(tabfile_t& simuconf);

	void set_size_x(sint32 g) {size_x=g;}
	void set_size_y(sint32 g) {size_y=g;}
	void set_size(sint32 x, sint32 y) {size_x = x; size_y=y;}
	sint32 get_size_x() const {return size_x;}
	sint32 get_size_y() const {return size_y;}

	sint32 get_map_number() const {return map_number;}

	void set_factory_count(sint32 d) { factory_count=d; }
	sint32 get_factory_count() const {return factory_count;}

	sint32 get_electric_promille() const {return electric_promille;}

	void set_tourist_attractions( sint32 n ) { tourist_attractions = n; }
	sint32 get_tourist_attractions() const {return tourist_attractions;}

	void set_city_count(sint32 n) {city_count=n;}
	sint32 get_city_count() const {return city_count;}

	void set_mean_citizen_count( sint32 n ) {mean_citizen_count = n;}
	sint32 get_mean_citizen_count() const {return mean_citizen_count;}

	void set_traffic_level(sint32 l) {traffic_level=l;}
	sint32 get_traffic_level() const {return traffic_level;}

	sint8 get_maximumheight() const { return world_maximum_height; }
	sint8 get_minimumheight() const { return world_minimum_height; }

	sint8 get_groundwater() const {return (sint8)groundwater;}

	double get_max_mountain_height() const {return max_mountain_height;}

	double get_map_roughness() const {return map_roughness;}

	uint16 get_station_coverage() const {return station_coverage_size;}

	uint8 get_max_rail_convoi_length() const {return max_rail_convoi_length;}
	uint8 get_max_road_convoi_length() const {return max_road_convoi_length;}
	uint8 get_max_ship_convoi_length() const {return max_ship_convoi_length;}
	uint8 get_max_air_convoi_length() const {return max_air_convoi_length;}

	void set_allow_player_change(char n) {allow_player_change=n;}
	uint8 get_allow_player_change() const {return allow_player_change;}

	void set_use_timeline(char n) {use_timeline=n;}
	uint8 get_use_timeline() const {return use_timeline;}

	void set_starting_year( sint16 n ) { starting_year = n; }
	sint16 get_starting_year() const {return starting_year;}

	void set_starting_month( sint16 n ) { starting_month = n; }
	sint16 get_starting_month() const {return starting_month;}

	sint16 get_bits_per_month() const {return bits_per_month;}

	void set_filename(const char *n) {filename=n;}
	const char* get_filename() const { return filename.c_str(); }

	bool get_beginner_mode() const {return beginner_mode;}

	void set_just_in_time(uint8 b) { just_in_time = b; }
	uint8 get_just_in_time() const {return just_in_time;}

	void rotate90() {
		rotation = (rotation+1)&3;
		set_size( size_y, size_x );
		wind_direction = ribi_t::rotate90(wind_direction);
	}
	uint8 get_rotation() const { return rotation; }

	void set_origin_x(sint16 x) { origin_x = x; }
	void set_origin_y(sint16 y) { origin_y = y; }
	sint16 get_origin_x() const { return origin_x; }
	sint16 get_origin_y() const { return origin_y; }

	bool is_freeplay() const { return freeplay; }
	void set_freeplay( bool f ) { freeplay = f; }

	sint32 get_max_route_steps() const { return max_route_steps; }
	sint32 get_max_choose_route_steps() const { return max_choose_route_steps; }
	sint32 get_max_hops() const { return max_hops; }
	sint32 get_max_transfers() const { return max_transfers; }

	sint64 get_starting_money(sint16 year) const;

	sint16 get_special_building_distance() const { return special_building_distance; }

	sint16 get_min_factory_spacing() const { return min_factory_spacing; }
	sint16 get_max_factory_spacing() const { return max_factory_spacing; }
	sint16 get_max_factory_spacing_percent() const { return max_factory_spacing_percentage; }
	sint16 get_crossconnect_factor() const { return crossconnect_factor; }
	bool is_crossconnect_factories() const { return crossconnect_factories; }

	bool get_numbered_stations() const { return numbered_stations; }

	sint32 get_stadtauto_duration() const { return stadtauto_duration; }

	sint32 get_beginner_price_factor() const { return beginner_price_factor; }

	const way_desc_t *get_city_road_type( uint16 year );
	const way_desc_t *get_intercity_road_type( uint16 year );
	uint16 get_city_road_speed_limit(uint16 year);

	void set_pak_diagonal_multiplier(uint16 n) { pak_diagonal_multiplier = n; }
	uint16 get_pak_diagonal_multiplier() const { return pak_diagonal_multiplier; }

	int get_name_language_id() const;
	void set_name_language_iso( const char *iso ) {
		language_code_names[0] = iso[0];
		language_code_names[1] = iso[1];
		language_code_names[2] = 0;
	}

	void set_player_type(uint8 i, uint8 t) { player_type[i] = t; }
	uint8 get_player_type(uint8 i) const { return player_type[i]; }

	bool is_separate_halt_capacities() const { return separate_halt_capacities ; }

	// allowed modes are 0,1,2
	enum {
		TO_PREVIOUS = 0,
		TO_TRANSFER,
		TO_DESTINATION
	};
	uint8 get_pay_for_total_distance_mode() const { return pay_for_total_distance ; }

	// do not take people to overcrowded destinations
	bool is_avoid_overcrowding() const { return avoid_overcrowding; }

	// do not allow routes over overcrowded destinations
	bool is_no_routing_over_overcrowding() const { return no_routing_over_overcrowding; }

	sint16 get_river_number() const { return river_number; }
	sint16 get_min_river_length() const { return min_river_length; }
	sint16 get_max_river_length() const { return max_river_length; }

	// true, if this pak should be used with extensions (default)
	void set_with_private_paks(bool b ) { with_private_paks = b; }
	bool get_with_private_paks() const { return with_private_paks; }

	sint32 get_passenger_factor() const { return passenger_factor; }

	// town growth stuff
	sint32 get_passenger_multiplier() const { return passenger_multiplier; }
	sint32 get_mail_multiplier() const { return mail_multiplier; }
	sint32 get_goods_multiplier() const { return goods_multiplier; }
	sint32 get_electricity_multiplier() const { return electricity_multiplier; }

	// Also there are size dependent factors (0=no growth)
	sint32 get_growthfactor_small() const { return growthfactor_small; }
	sint32 get_growthfactor_medium() const { return growthfactor_medium; }
	sint32 get_growthfactor_large() const { return growthfactor_large; }

	// percentage of passengers for different kinds of trips
	sint16 get_factory_worker_percentage() const { return factory_worker_percentage; }
	sint16 get_tourist_percentage() const { return tourist_percentage; }

	// radius from factories to get workers from towns (usually set to 77 but 1/8 of map size may be meaningful too)
	uint16 get_factory_worker_radius() const { return factory_worker_radius; }

	// any factory will be connected to at least this number of next cities
	uint32 get_factory_worker_minimum_towns() const { return factory_worker_minimum_towns; }
	void set_factory_worker_minimum_towns(uint32 n) { factory_worker_minimum_towns = n; }

	// any factory will be connected to not more than this number of next cities
	uint32 get_factory_worker_maximum_towns() const { return factory_worker_maximum_towns; }
	void set_factory_worker_maximum_towns(uint32 n) { factory_worker_maximum_towns = n; }

	// number of periods for averaging the amount of arrived pax/mail at factories
	uint16 get_factory_arrival_periods() const { return factory_arrival_periods; }

	// whether factory pax/mail demands are enforced
	bool get_factory_enforce_demand() const { return factory_enforce_demand; }

	uint16 get_factory_maximum_intransit_percentage() const { return factory_maximum_intransit_percentage; }

	uint32 get_locality_factor(sint16 year) const;

	// disallow using obsolete vehicles in depot
	bool get_allow_buying_obsolete_vehicles() const { return allow_buying_obsolete_vehicles; }

	// forest stuff
	uint8 get_forest_base_size() const { return forest_base_size; }
	uint8 get_forest_map_size_divisor() const { return forest_map_size_divisor; }
	uint8 get_forest_count_divisor() const { return forest_count_divisor; }
	uint16 get_forest_inverse_spare_tree_density() const { return forest_inverse_spare_tree_density; }
	uint8 get_max_no_of_trees_on_square() const { return max_no_of_trees_on_square; }
	uint16 get_tree_climates() const { return tree_climates; }
	uint16 get_no_tree_climates() const { return no_tree_climates; }
	tree_distribution_t get_tree_distribution() const { return (tree_distribution_t)tree_distribution; }
	void set_tree_distribution(tree_distribution_t value) { tree_distribution = value; }

	void set_default_climates();
	sint8 get_climate_borders( sint8 climate, sint8 start_end ) const { return (sint8)climate_borders[climate][start_end]; }

	sint8 get_tropic_humidity() const { return tropic_humidity; }
	sint8 get_desert_humidity() const { return desert_humidity; }
	sint8 get_climate_temperature_borders( sint8 border ) const { return climate_temperature_borders[border]; }

	climate_generate_t get_climate_generator() const { return climate_generator; }
	void set_climate_generator(climate_generate_t generator) { climate_generator = generator; }

	sint8 get_moisture() const { return moisture;  }
	sint8 get_moisture_water() const { return moisture_water;  }
	sint16 get_winter_snowline() const { return winter_snowline; }

	sint8 get_lakeheight() const { return lake_height; }
	void set_lakeheight(sint8 h) { lake_height = h; }

	/// Wind is coming from this direction
	ribi_t::ribi get_wind_dir() const { return wind_direction;  }
	ribi_t::ribi get_approach_dir() const { return wind_direction | ribi_t::rotate90(wind_direction);  }

	sint8 get_patch_size_percentage() const { return patch_size_percentage; }

	uint32 get_industry_increase_every() const { return industry_increase; }
	void set_industry_increase_every( uint32 n ) { industry_increase = n; }
	uint32 get_minimum_city_distance() const { return minimum_city_distance; }

	sint16 get_used_vehicle_reduction() const { return used_vehicle_reduction; }

	void set_player_color_to_default( player_t *player ) const;
	void set_default_player_color(uint8 player_nr, uint8 color1, uint8 color2);

	// usually only used in network mode => no need to set them!
	uint32 get_random_counter() const { return random_counter; }
	uint32 get_frames_per_second() const { return frames_per_second; }
	uint32 get_frames_per_step() const { return frames_per_step; }
	uint32 get_server_frames_ahead() const { return server_frames_ahead; }

	bool is_drive_left() const { return drive_on_left; }
	bool is_signals_left() const { return signals_on_left; }

	sint32 get_way_toll_runningcost_percentage() const { return way_toll_runningcost_percentage; }
	sint32 get_way_toll_waycost_percentage() const { return way_toll_waycost_percentage; }

	sint32 get_bonus_basefactor() const { return bonus_basefactor; }

	bool get_allow_underground_transformers() const { return allow_underground_transformers; }
	bool get_disable_make_way_public() const { return disable_make_way_public; }

	uint32 get_allow_merge_distant_halt() const { return allow_merge_distant_halt; }

	uint16 get_remove_dummy_player_months() const { return remove_dummy_player_months; }
	uint16 get_unprotect_abandoned_player_months() const { return unprotect_abandoned_player_months; }

	uint8 get_way_height_clearance() const { return way_height_clearance; }
	void set_way_height_clearance( uint8 n ) { way_height_clearance = n; }

	uint32 get_default_ai_construction_speed() const { return default_ai_construction_speed; }
	void set_default_ai_construction_speed( uint32 n ) { default_ai_construction_speed = n; }

	bool get_stop_halt_as_scheduled() const { return stop_halt_as_scheduled; }
	void set_stop_halt_as_scheduled(bool b) { stop_halt_as_scheduled = b; }

	// some settigns are not to be saved in the global settings
	void reset_after_global_settings_reload();

	sint64 get_make_public_months() const { return cst_make_public_months; }
};

#endif
