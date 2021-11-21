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
 */
class settings_t
{
	// these are the only classes, that are allowed to modify elements from settings_t
	// for all remaining special cases there are the set_...() routines
	friend class settings_general_stats_t;
	friend class settings_routing_stats_t;
	friend class settings_economy_stats_t;
	friend class settings_costs_stats_t;
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
	sint32 size_x, size_y;
	sint32 map_number;

	/* new setting since version 0.85.01
	 */
	sint32 factory_count;
	sint32 electric_promille;
	sint32 tourist_attractions;

	sint32 city_count;
	sint32 mean_citizen_count;

	// town growth factors
	sint32 passenger_multiplier;
	sint32 mail_multiplier;
	sint32 goods_multiplier;
	sint32 electricity_multiplier;

	// Also there are size dependent factors (0=no growth)
	sint32 growthfactor_small;
	sint32 growthfactor_medium;
	sint32 growthfactor_large;

	sint16 special_building_distance; // distance between attraction to factory or other special buildings
	uint32 minimum_city_distance;
	uint32 industry_increase;

	// percentage of routing
	sint16 factory_worker_percentage;
	sint16 tourist_percentage;

	// higher number: passengers are more evenly distributed around the map
	struct yearly_locality_factor_t
	{
		sint16 year;
		uint32 factor;
	};
	yearly_locality_factor_t locality_factor_per_year[10];

	// radius for factories
	sint16 factory_worker_radius;
	sint32 factory_worker_minimum_towns;
	sint32 factory_worker_maximum_towns;

	// number of periods for averaging the amount of arrived pax/mail at factories
	uint16 factory_arrival_periods;

	// whether factory pax/mail demands are enforced
	bool factory_enforce_demand;

	uint16 station_coverage_size;

	// the maximum length of each convoi
	uint8 max_rail_convoi_length;
	uint8 max_road_convoi_length;
	uint8 max_ship_convoi_length;
	uint8 max_air_convoi_length;

	/**
	 * At which level buildings generate traffic?
	 */
	sint32 traffic_level;

	/**
	 * Should pedestrians be displayed?
	 */
	sint32 show_pax;

	/**
	 * the maximum and minimum allowed world height.
	 */
	sint8 world_maximum_height;
	sint8 world_minimum_height;

	 /**
	 * waterlevel, climate borders, lowest snow in winter
	 */
	climate_generate_t climate_generator;
	sint16 groundwater;
	sint16 winter_snowline;
	sint16 climate_borders[MAX_CLIMATES][2];
	sint8 climate_temperature_borders[5];
	sint8 tropic_humidity;
	sint8 desert_humidity;

	ribi_t::ribi wind_direction; ///< Wind is coming from this direction. Must be single! (N/W/S/E)

	sint8 patch_size_percentage; // average size of a climate patch, if there are overlapping climates

	sint8 moisture; // how much increase of moisture per tile
	sint8 moisture_water; // how much increase of moisture per water tile

	double max_mountain_height;
	double map_roughness;

	// river stuff
	sint16 river_number;
	sint16 min_river_length;
	sint16 max_river_length;

	// forest stuff
	uint8 forest_base_size;
	uint8 forest_map_size_divisor;
	uint8 forest_count_divisor;
	uint16 forest_inverse_spare_tree_density;
	uint8 max_no_of_trees_on_square;
	uint16 tree_climates;
	uint16 no_tree_climates;

public:
	enum tree_distribution_t
	{
		TREE_DIST_NONE     = 0,
		TREE_DIST_RANDOM   = 1,
		TREE_DIST_RAINFALL = 2,
		TREE_DIST_COUNT
	};
private:
	uint16 tree_distribution;

	sint8 lake_height; //relative to sea height

	// game mechanics
	uint8 allow_player_change;
	uint8 use_timeline;
	sint16 starting_year;
	sint16 starting_month;
	sint16 bits_per_month;

	std::string filename;

	bool beginner_mode;
	sint32 beginner_price_factor;

	/* Industry supply model used.
	 * 0 : Classic (no flow control?)
	 * 1 : JIT Classic (maximum transit and storage limited)
	 * 2 : JIT Version 2 (demand buffers with better consumption model)
	 */
	uint8 just_in_time;

	// default 0, will be incremented after each 90 degree rotation until 4
	uint8 rotation;

	sint16 origin_x, origin_y;

	sint32 passenger_factor;

	sint16 min_factory_spacing;
	sint16 max_factory_spacing;
	sint16 max_factory_spacing_percentage;

	/*no goods will put in route, when stored>gemax_storage and goods_in_transit*maximum_intransit_percentage/100>max_storage  */
	uint16 factory_maximum_intransit_percentage;

	/* crossconnect all factories (like OTTD and similar games) */
	bool crossconnect_factories;

	/* crossconnect all factories (like OTTD and similar games) */
	sint16 crossconnect_factor;

	/**
	* Generate random pedestrians in the cities?
	*/
	bool random_pedestrians;

	sint32 stadtauto_duration;

	bool freeplay;

	sint64 starting_money;

	struct yearmoney
	{
		sint16 year;
		sint64 money;
		bool interpol;
	};
	yearmoney startingmoneyperyear[10];

	uint16 num_city_roads;
	road_timeline_t city_roads[10];
	uint16 num_intercity_roads;
	road_timeline_t intercity_roads[10];

	/**
	 * Use numbering for stations?
	 */
	bool numbered_stations;

	/* maximum number of steps for breath search */
	sint32 max_route_steps;

	// maximum length for route search at signs/signals
	sint32 max_choose_route_steps;

	// max steps for good routing
	sint32 max_hops;

	/* maximum number of steps for breath search */
	sint32 max_transfers;

	/* multiplier for steps on diagonal:
	 * 1024: TT-like, factor 2, vehicle will be too long and too fast
	 * 724: correct one, factor sqrt(2)
	 */
	uint16 pak_diagonal_multiplier;

	// names of the stations ...
	char language_code_names[4];

	// true, if the different capacities (passengers/mail/freight) are counted separately
	bool separate_halt_capacities;

	/**
	 * payment is only for the distance that got shorter between target and start
	 * three modes:
	 * 0 = pay for travelled manhattan distance
	 * 1 = pay for distance difference to next transfer stop
	 * 2 = pay for distance to destination
	 * 0 allows chaeting, but the income with 1 or two are much lower
	 */
	uint8 pay_for_total_distance;

	/* if set, goods will avoid being routed over overcrowded stops */
	bool avoid_overcrowding;

	/* if set, goods will not routed over overcrowded stations but rather try detours (if possible) */
	bool no_routing_over_overcrowding;

	// lowest possible income with speedbonus (1000=1) default 125
	sint32 bonus_basefactor;

	// true, if this pak should be used with extensions (default)
	bool with_private_paks;

	/// what is the minimum clearance required under bridges
	uint8 way_height_clearance;

	// if true, you can buy obsolete stuff
	bool allow_buying_obsolete_vehicles;
	// vehicle value is decrease by this factor/1000 when a vehicle leaved the depot
	sint16 used_vehicle_reduction;

	uint32 random_counter;
	uint32 frames_per_second; // only used in network mode ...
	uint32 frames_per_step;
	uint32 server_frames_ahead;

	bool drive_on_left;
	bool signals_on_left;

	// fraction of running costs charged for going on other players way
	sint32 way_toll_runningcost_percentage;
	sint32 way_toll_waycost_percentage;

	// true if transformers are allowed to built underground
	bool allow_underground_transformers;

	// true if companies can make ways public
	bool disable_make_way_public;

	// only for trains. If true, trains stop at the position designated in the schdule..
	bool stop_halt_as_scheduled;

public:
	/* the big cost section */
	sint32 maint_building; // normal building

	sint64 cst_multiply_dock;
	sint64 cst_multiply_station;
	sint64 cst_multiply_roadstop;
	sint64 cst_multiply_airterminal;
	sint64 cst_multiply_post;
	sint64 cst_multiply_headquarter;
	sint64 cst_depot_rail;
	sint64 cst_depot_road;
	sint64 cst_depot_ship;
	sint64 cst_depot_air;

	// cost to merge station
	uint32 allow_merge_distant_halt;
	sint64 cst_multiply_merge_halt;

	// alter landscape
	sint64 cst_buy_land;
	sint64 cst_alter_land;
	sint64 cst_alter_climate;
	sint64 cst_set_slope;
	sint64 cst_found_city;
	sint64 cst_multiply_found_industry;
	sint64 cst_remove_tree;
	sint64 cst_multiply_remove_haus;
	sint64 cst_multiply_remove_field;
	sint64 cst_transformer;
	sint64 cst_maintain_transformer;

	// maintainance cost in months to make something public
	sint64 cst_make_public_months;

	// costs for the way searcher
	sint32 way_count_straight;
	sint32 way_count_curve;
	sint32 way_count_double_curve;
	sint32 way_count_90_curve;
	sint32 way_count_slope;
	sint32 way_count_tunnel;
	uint32 way_max_bridge_len;
	sint32 way_count_leaving_road;

	// 0 = empty, otherwise some value from simplay
	uint8 player_type[MAX_PLAYER_COUNT];

	// how fast new AI will built something
	uint32 default_ai_construction_speed;

	// player color suggestions for new games
	bool default_player_color_random;
	uint8 default_player_color[MAX_PLAYER_COUNT][2];

	// remove dummy companies and remove password from abandoned companies
	uint16 remove_dummy_player_months;
	uint16 unprotect_abandoned_player_months;

public:
	/**
	 * If map is read from a heightfield, this is the name of the heightfield.
	 * Set to empty string in order to avoid loading.
	 */
	std::string heightfield;

	settings_t();

	void rdwr(loadsave_t *file);

	void copy_city_road(settings_t const& other);

	// init from this file ...
	void parse_simuconf(tabfile_t& simuconf, sint16& disp_width, sint16& disp_height, sint16& fullscreen, std::string& objfilename);

	// init without screen parameters ...
	void parse_simuconf(tabfile_t& simuconf) {
		sint16 idummy = 0;
		std::string sdummy;

		parse_simuconf(simuconf, idummy, idummy, idummy, sdummy);
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

	void set_show_pax(bool yesno) {show_pax=yesno;}
	bool get_show_pax() const {return show_pax != 0;}

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

	bool get_random_pedestrians() const { return random_pedestrians; }
	void set_random_pedestrians( bool f ) { random_pedestrians = f; }

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
	void set_with_private_paks(bool b ) {with_private_paks = b;}
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
};

#endif
