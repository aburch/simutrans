/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SETTINGS_H
#define DATAOBJ_SETTINGS_H


#include <string>
#include "../simtypes.h"
#include "../simconst.h"
#include "../simunits.h"
#include "livery_scheme.h"
#include "../tpl/piecewise_linear_tpl.h" // for various revenue tables
#include "../dataobj/koord.h"

/**
 * Game settings
 *
 * Hj. Malthaner
 *
 * April 2000
 */

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
struct region_definition_t
{
	std::string name;
	koord top_left = koord::invalid;
	koord bottom_right = koord::invalid;
};

template <class T>
class vector_with_ptr_ownership_tpl : public vector_tpl<T*>
{
public:
	vector_with_ptr_ownership_tpl(uint32 size = 0) :
		vector_tpl<T*>(size) {}

	vector_with_ptr_ownership_tpl( vector_with_ptr_ownership_tpl const& src ) :
		vector_tpl<T*>( src.get_count() ) {
		ITERATE( src, i ) {
			this->append( new T( *src[i] ) );
		}
	}

	vector_with_ptr_ownership_tpl& operator=( vector_with_ptr_ownership_tpl const& other ) {
		vector_with_ptr_ownership_tpl tmp(other);
		swap( static_cast<vector_tpl<T*>&>(tmp), static_cast<vector_tpl<T*>&>(*this) );
		return *this;
	}

	void clear() {
		clear_ptr_vector(*this);
	}

	~vector_with_ptr_ownership_tpl() {
		clear();
	}
};

class settings_t
{
	// these are the only classes, that are allowed to modify elements from settings_t
	// for all remaining special cases there are the set_...() routines
	friend class settings_general_stats_t;
	friend class settings_routing_stats_t;
	friend class settings_economy_stats_t;
	friend class settings_costs_stats_t;
	friend class settings_climates_stats_t;
	friend class settings_extended_general_stats_t;
	friend class settings_extended_revenue_stats_t;
	friend class climate_gui_t;
	friend class welt_gui_t;

private:
	/**
	* Many settings can be set by a pak, a local config file, and also by a saved game.
	* The saved game will override the pak settings normally,
	* ...which is very undesirable when debugging a pak.
	*
	* If progdir_overrides_savegame_settings is true, this will prefer the "progdir" settings.
	* If pak_overrides_savegame_settings is true, this will prefer the pak settings.
	* If userder_overrides_savegame_settings is true, this will prefer the pak settings.
	*
	* If several are set, they are read in the order progdir, pak, userdir.
	*/
	bool progdir_overrides_savegame_settings;
	bool pak_overrides_savegame_settings;
	bool userdir_overrides_savegame_settings;

	sint32 size_x, size_y;
	sint32 map_number;

	/* new setting since version 0.85.01
	 * @author prissi
	 */
	sint32 factory_count;
	sint32 electric_promille;
	sint32 tourist_attractions;

	sint32 city_count;
	sint32 mean_einwohnerzahl;

	// town growth factors
	sint32 passenger_multiplier;
	sint32 mail_multiplier;
	sint32 goods_multiplier;
	sint32 electricity_multiplier;

	// Also there are size dependent factors (0=no growth)
	sint32 growthfactor_small;
	sint32 growthfactor_medium;
	sint32 growthfactor_large;

	sint16 special_building_distance;	// distance between attraction to factory or other special buildings
	uint32 industry_increase;
	uint32 city_isolation_factor;

	// Knightly : number of periods for averaging the amount of arrived pax/mail at factories
	uint16 factory_arrival_periods;

	// Knightly : whether factory pax/mail demands are enforced
	bool factory_enforce_demand;

	uint16 station_coverage_size;
	// The coverage circle for factories - allows this to be smaller than for passengers/mail.
	uint16 station_coverage_size_factories;

	/**
	 * At which level buildings generate traffic?
	 */
	sint32 traffic_level;

	/**
	 * Should pedestrians be displayed?
	 */
	sint32 show_pax;

	 /**
	 * waterlevel, climate borders, lowest snow in winter
	 */

	sint16 groundwater;
	sint16 climate_borders[MAX_CLIMATES];
	sint16 winter_snowline;

	double max_mountain_height;                  //01-Dec-01        Markus Weber    Added
	double map_roughness;                        //01-Dec-01        Markus Weber    Added

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
	bool no_trees;

	bool lake;

	// game mechanics
	uint8 allow_player_change;
	uint8 use_timeline;
	sint16 starting_year;
	sint16 starting_month;
	sint16 bits_per_month;

	std::string filename;

	bool beginner_mode;
	sint32 beginner_price_factor;

	uint8 just_in_time;

	// default 0, will be incremented after each 90 degree rotation until 4
	uint8 rotation;

	sint16 origin_x, origin_y;

	sint16 min_factory_spacing;
	sint16 max_factory_spacing;
	sint16 max_factory_spacing_percentage;

	/*no goods will put in route, when stored>gemax_storage and goods_in_transit*maximum_intransit_percentage/100>max_storage  */
	uint16 factory_maximum_intransit_percentage;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	bool crossconnect_factories;

	/* prissi: crossconnect all factories (like OTTD and similar games) */
	sint16 crossconnect_factor;

	/**
	* Generate random pedestrians in the cities?
	*
	* @author Hj. Malthaner
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
	road_timeline_t city_roads[16];
	uint16 num_intercity_roads;
	road_timeline_t intercity_roads[16];

	/**
	 * Use numbering for stations?
	 *
	 * @author Hj. Malthaner
	 */
	bool numbered_stations;

	/* prissi: maximum number of steps for breath search */
	sint32 max_route_steps;

	// maximum length for route search at signs/signals
	sint32 max_choose_route_steps;

	// max steps for good routing
	sint32 max_hops;

	/* prissi: maximum number of steps for breath search */
	sint32 max_transfers;

	/**
	 * multiplier for steps on diagonal:
	 * 1024: TT-like, factor 2, vehicle will be too long and too fast
	 * 724: correct one, factor sqrt(2)
	 */
	uint16 pak_diagonal_multiplier;

	// names of the stations ...
	char language_code_names[4];

	// true, if the different capacities (passengers/mail/freight) are counted separately
	bool separate_halt_capacities;

	/// The set of livery schemes
	vector_with_ptr_ownership_tpl<class livery_scheme_t> livery_schemes;

	/// This is automatically set if the simuconf.tab region specifications are in absolute
	/// rather than percentage terms. Stops regions from being modified when the map is resized
	/// or a new map generated. Does not affect saved games.
	bool absolute_regions = false;

public:
	/// The set of all regions
	vector_tpl<region_definition_t> regions;

private:

	// Whether passengers might walk between stops en route.
	// @author: jamespetts, August 2011
	bool allow_routing_on_foot;

	/**
	 * If true, players will not be charged
	 * for using roads owned by the public
	 * service player. Other way types are
	 * not affected.
	 * @author: jamespetts, October 2011
	 */
	bool toll_free_public_roads;

	uint8 max_elevated_way_building_level;

	/**
	 * If players allow access to their roads
	 * by private cars, they will pay this
	 * much of a toll for every tile
	 */
	sint64 private_car_toll_per_km;

	/**
	 * If this is enabled, player roads
	 * built within city limits will on
	 * building be classed as unowned so
	 * that other players may use/modify
	 * them. This will not affect other
	 * ways built outside city limits
	 * but which subsequently fall within
	 * them.
	 */
	bool towns_adopt_player_roads;

	/**
	 * This is the number of steps between each call
	 * of the path explorer full refresh method. This
	 * value determines how frequently that the pathing
	 * information in the game is refreshed. 8192 ~
	 * 2h in 250m/tile or ~ 1h in 125m/tile.
	 * NOTE: This value will revert to the default
	 * of 8192 on every save/load until major version 11.
	 */
	uint32 reroute_check_interval_steps;

	/**
	 * The speed at which pedestrians walk in km/h.
	 * Used in journey time calculations.
	 * NOTE: The straight line distance is used
	 * with this speed.
	 */
	uint8 walking_speed;

	/**
	* These two settings define the random number generator
	* mode for the journey time tolerances of commuting and
	* visiting passengers respectively, i.e., whether the
	* random numbers are normalised, and, if normalised, whether
	* and how much they should be skewed
	*/
	uint32 random_mode_commuting;
	uint32 random_mode_visiting;

	/**
	* The number of private car routes tiles in a city to process
	* in a single step. The higher the number, the more quickly
	* that the private car routes update; the lower the number,
	* the faster the performance. Reduce this number if momentary
	* unresponsiveness be noticed frequently.
	*/
	uint32 max_route_tiles_to_process_in_a_step = 1024;

	/**
	* This modifies the base journey time tolerance for passenger
	* trips to allow more fine grained control of the journey time
	* tolernace algorithm. If this be set to zero, then the per
	* building adjustment of journey time tolerance will be disabled.
	* This only affects visiting passengers.
	*/
	uint32 tolerance_modifier_percentage = 100;

	/**
	* Setting this allows overriding of the calculated industry density
	* proportion to (1) allow players the ability to control industry growth
	* after map editing; and (2) correct errors/balance issues in server games.
	* Setting this to 0, its default setting, will mean that it has no effect.
	*/
	uint32 industry_density_proportion_override = 0;

public:
	//Cornering settings
	//@author: jamespetts

	//The array index corresponds
	//to the waytype index.

	sint32 corner_force_divider[10];

	uint8 curve_friction_factor[10];

	sint32 tilting_min_radius_effect;

	/* if set, goods will avoid being routed over overcrowded stops */
	bool avoid_overcrowding;

	/* if set, goods will not routed over overcrowded stations but rather try detours (if possible) */
	bool no_routing_over_overcrowding;

	// The longest time that a passenger is
	// prepared to wait for transport.
	// @author: jamespetts
	uint32 passenger_max_wait;

	uint8 max_rerouting_interval_months;

public:

	uint16 meters_per_tile;

	uint32 base_meters_per_tile;
	uint32 base_bits_per_month;
	uint32 job_replenishment_per_hundredths_of_months;

	// We need it often(every vehicle_base_t::do_drive call), so we cache it.
	uint32 steps_per_km;

private:
	// The public version of these is exposed via tables below --neroden
	uint8 tolerable_comfort_short;
	uint8 tolerable_comfort_median_short;
	uint8 tolerable_comfort_median_median;
	uint8 tolerable_comfort_median_long;
	uint8 tolerable_comfort_long;
	uint16 tolerable_comfort_short_minutes;
	uint16 tolerable_comfort_median_short_minutes;
	uint16 tolerable_comfort_median_median_minutes;
	uint16 tolerable_comfort_median_long_minutes;
	uint16 tolerable_comfort_long_minutes;

	uint8 max_luxury_bonus_differential;
	uint8 max_discomfort_penalty_differential;
	uint16 max_luxury_bonus_percent;
	uint16 max_discomfort_penalty_percent;

	uint16 catering_min_minutes;
	uint16 catering_level1_minutes;
	uint16 catering_level1_max_revenue;
	uint16 catering_level2_minutes;
	uint16 catering_level2_max_revenue;
	uint16 catering_level3_minutes;
	uint16 catering_level3_max_revenue;
	uint16 catering_level4_minutes;
	uint16 catering_level4_max_revenue;
	uint16 catering_level5_minutes;
	uint16 catering_level5_max_revenue;

	uint16 tpo_min_minutes;
	uint16 tpo_revenue;

	// The number of passenger trips per game month per unit of population (adjusted based on the adjusted monthly figure)
	uint32 passenger_trips_per_month_hundredths;
	// The number of packets of mail generated per month per unit of population (adjusted based on the adjusted monthly figure)
	uint32 mail_packets_per_month_hundredths;

	// The maximum number of onward trips permissible for passengers that do make an onward trip.
	uint16 max_onward_trips;

	// The distribution_weight (in %) that any given packet of passengers will make at least one onward trip.
	uint16 onward_trip_chance_percent;

	// The distribution_weight (in %) that any given packet of passengers will be making a commuting, rather than visiting, trip.
	uint16 commuting_trip_chance_percent;

	// This is used to store the adjusted value for the number of ticks that must elapse before a single job is replenished.
	sint64 job_replenishment_ticks;

public:
	// @author: neroden
	// Linear interpolation tables for various things
	// First argument is "in" type, second is "out" type
	// Third argument is intermediate computation type

	// The tolerable comfort table. (tenths of minutes to comfort)
	piecewise_linear_tpl<uint16, sint16, uint32> tolerable_comfort;
	// The max tolerable journey table (comfort to seconds)
	piecewise_linear_tpl<uint8, uint32, uint64> max_tolerable_journey;
	// The base comfort revenue table (comfort - tolerable comfort to percentage)
	piecewise_linear_tpl<sint16, sint16, sint32> base_comfort_revenue;
	// The comfort derating table (tenths of minutes to percentage)
	piecewise_linear_tpl<uint16, uint8, uint32> comfort_derating;

	// @author: neroden
	// Tables 0 through 5 for catering revenue.
	// One for each level -- so there are 6 of them total.
	// Dontcha hate C array declaration style?
	piecewise_linear_tpl<uint16, sint64, uint32> catering_revenues[6];

	// Single table for TPO revenues.
	piecewise_linear_tpl<uint16, sint64, uint32> tpo_revenues;


	//@author: jamespetts
	// Obsolete vehicle maintenance cost increases
	uint16 obsolete_running_cost_increase_percent;
	uint16 obsolete_running_cost_increase_phase_years;

	// @author: jamespetts
	// Private car settings
	uint8 always_prefer_car_percent;
	uint8 congestion_density_factor;

	//@author: jamespetts
	// Passenger routing settings
	uint8 passenger_routing_packet_size;

	uint16 max_alternative_destinations_visiting;
	uint16 max_alternative_destinations_commuting;

	uint16 min_alternative_destinations_visiting;
	uint16 min_alternative_destinations_commuting;

	uint32 max_alternative_destinations_per_visitor_demand_millionths;
	uint32 max_alternative_destinations_per_job_millionths;

	uint32 min_alternative_destinations_per_visitor_demand_millionths;
	uint32 min_alternative_destinations_per_job_millionths;

	//@author: jamespetts
	// Factory retirement settings
	uint16 factory_max_years_obsolete;

	//@author: jamespetts
	// Insolvency and debt settings
	uint8 interest_rate_percent;
	bool allow_insolvency;
	bool allow_purchases_when_insolvent;

	// Reversing settings
	//@author: jamespetts
	uint32 unit_reverse_time;
	uint32 hauled_reverse_time;
	uint32 turntable_reverse_time;
	uint32 road_reverse_time;

	uint16 unit_reverse_time_seconds;
	uint16 hauled_reverse_time_seconds;
	uint16 turntable_reverse_time_seconds;
	uint16 road_reverse_time_seconds;

	//@author: jamespetts
	uint16 global_power_factor_percent;
	uint16 global_force_factor_percent;

	// Whether and how weight limits are enforced
	// @author: jamespetts
	uint8 enforce_weight_limits;

	bool allow_airports_without_control_towers;

	/**
	 * The shortest time that passengers/goods can
	 * wait at an airport before boarding an aircraft.
	 * Waiting times are higher at airports because of
	 * the need to check-in, undergo security checks,
	 * etc.
	 * @author: jamespetts, August 2011
	 */
	uint16 min_wait_airport;

private:

	// true, if this pak should be used with extensions (default)
	bool with_private_paks;

public:

	// The ranges for the journey time tolerance for passengers.
	// @author: jamespetts
	uint32 range_commuting_tolerance;
	uint32 min_commuting_tolerance;
	uint32 min_visiting_tolerance;
	uint32 range_visiting_tolerance;

private:
	/// what is the minimum clearance required under bridges
	sint8 way_height_clearance;

	// 1 = allow purchase of all out of production vehicles, including obsolete vehicles 2 = allow purchase of out of produciton vehicles that are not obsolete only
	uint8 allow_buying_obsolete_vehicles;
	// vehicle value is decrease by this factor/1000 when a vehicle leaved the depot
	sint16 used_vehicle_reduction;

	uint32 random_counter;
	uint32 frames_per_second;	// only used in network mode ...
	uint32 frames_per_step;
	uint32 server_frames_ahead;

	bool drive_on_left;
	bool signals_on_left;

	// The fraction of running costs (etc.) charged for going on other players' ways
	// and using their air and sea ports.
	sint32 way_toll_runningcost_percentage;
	sint32 way_toll_waycost_percentage;
	sint32 way_toll_revenue_percentage;
	sint32 seaport_toll_revenue_percentage;
	sint32 airport_toll_revenue_percentage;

	// Whether non-public players are allowed to make stops and ways public.
	bool allow_making_public;
#ifndef NETTOOL
	float32e8_t simtime_factor;
	float32e8_t meters_per_step;
	float32e8_t steps_per_meter;
	float32e8_t seconds_per_tick;
#endif

	// true if transformers are allowed to built underground
	bool allow_underground_transformers;

	// true if companies can make ways public
	bool disable_make_way_public;

public:
	/* the big cost section */
	sint32 maint_building;	// normal building

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

	// true if active
	bool player_active[MAX_PLAYER_COUNT];
	// 0 = empty, otherwise some value from simplay
	uint8 player_type[MAX_PLAYER_COUNT];

	// If true, the old (faster) method of
	// city growth is used. If false (default),
	// the new, more accurate, method of city
	// growth is used.
	bool quick_city_growth;

	// The new (8.0) system for private cars checking
	// whether their destination is reachable can have
	// an adverse effect on performance. Allow it to
	// be disabled.
	bool assume_everywhere_connected_by_road;

	uint16 default_increase_maintenance_after_years[17];

	uint32 city_threshold_size;
	uint32 capital_threshold_size;
	uint32 max_small_city_size;
	uint32 max_city_size;

	uint8 capital_threshold_percentage;
	uint8 city_threshold_percentage;

	// The factor percentage of power revenue
	// default: 100
	// @author: Phystam
	uint32 power_revenue_factor_percentage;

	// how fast new AI will built something
	// Only used in Standard
	uint32 default_ai_construction_speed;

	// player color suggestions for new games
	bool default_player_color_random;
	uint8 default_player_color[MAX_PLAYER_COUNT][2];

	enum spacing_shift_mode_t { SPACING_SHIFT_DISABLED = 0, SPACING_SHIFT_PER_LINE, SPACING_SHIFT_PER_STOP};
	uint8 spacing_shift_mode;
	sint16 spacing_shift_divisor;

	// remove dummy companies and remove password from abandoned companies
	uint16 remove_dummy_player_months;
	uint16 unprotect_abandoned_player_months;

	uint16 population_per_level;
	uint16 visitor_demand_per_level;
	uint16 jobs_per_level;
	uint16 mail_per_level;

	// Forge cost for ways (array of way types)
	sint64 forge_cost_road;
	sint64 forge_cost_track;
	sint64 forge_cost_water;
	sint64 forge_cost_monorail;
	sint64 forge_cost_maglev;
	sint64 forge_cost_tram;
	sint64 forge_cost_narrowgauge;
	sint64 forge_cost_air;

	uint16 parallel_ways_forge_cost_percentage_road;
	uint16 parallel_ways_forge_cost_percentage_track;
	uint16 parallel_ways_forge_cost_percentage_water;
	uint16 parallel_ways_forge_cost_percentage_monorail;
	uint16 parallel_ways_forge_cost_percentage_maglev;
	uint16 parallel_ways_forge_cost_percentage_tram;
	uint16 parallel_ways_forge_cost_percentage_narrowgauge;
	uint16 parallel_ways_forge_cost_percentage_air;

	uint32 max_diversion_tiles;

	uint32 way_degradation_fraction;

	uint32 way_wear_power_factor_road_type;
	uint32 way_wear_power_factor_rail_type;
	uint16 standard_axle_load;
	uint32 citycar_way_wear_factor;

	uint32 sighting_distance_meters;
	uint16 sighting_distance_tiles;

	uint32 assumed_curve_radius_45_degrees;

	sint32 max_speed_drive_by_sight_kmh;
	sint32 max_speed_drive_by_sight;

	uint32 time_interval_seconds_to_clear;
	uint32 time_interval_seconds_to_caution;

	uint32 town_road_speed_limit;

	uint32 minimum_staffing_percentage_consumer_industry;
	uint32 minimum_staffing_percentage_full_production_producer_industry;

	uint16 max_comfort_preference_percentage;

	bool rural_industries_no_staff_shortage;
	uint32 auto_connect_industries_and_attractions_by_road;

	uint32 path_explorer_time_midpoint;
	bool save_path_explorer_data;

	// Whether players can know in advance the vehicle production end date and upgrade availability date
	// If false, only information up to one year ahead
	bool show_future_vehicle_info;

	/**
	 * If map is read from a heightfield, this is the name of the heightfield.
	 * Set to empty string in order to avoid loading.
	 * @author Hj. Malthaner
	 */
	std::string heightfield;

	settings_t();

	bool get_progdir_overrides_savegame_settings() {return progdir_overrides_savegame_settings;}
	bool get_pak_overrides_savegame_settings() {return pak_overrides_savegame_settings;}
	bool get_userdir_overrides_savegame_settings() {return userdir_overrides_savegame_settings;}

	void rdwr(loadsave_t *file);

	void copy_city_road(settings_t const& other);

	// init form this file ...
	void parse_simuconf( tabfile_t &simuconf, sint16 &disp_width, sint16 &disp_height, sint16 &fullscreen, std::string &objfilename );

	void set_size_x(sint32 g);
	void set_size_y(sint32 g);
	void set_groesse(sint32 x, sint32 y, bool preserve_regions = false);
	sint32 get_size_x() const {return size_x;}
	sint32 get_size_y() const {return size_y;}

	void reset_regions(sint32 old_x, sint32 old_y);
	void rotate_regions(sint16 y_size);

	sint32 get_map_number() const {return map_number;}

	void set_factory_count(sint32 d) { factory_count=d; }
	sint32 get_factory_count() const {return factory_count;}

	sint32 get_electric_promille() const {return electric_promille;}

	void set_tourist_attractions( sint32 n ) { tourist_attractions = n; }
	sint32 get_tourist_attractions() const {return tourist_attractions;}

	void set_city_count(sint32 n) {city_count=n;}
	sint32 get_city_count() const {return city_count;}

	void set_mean_einwohnerzahl( sint32 n ) {mean_einwohnerzahl = n;}
	sint32 get_mean_einwohnerzahl() const {return mean_einwohnerzahl;} // Median town size

	void set_traffic_level(sint32 l) {traffic_level=l;}
	sint32 get_traffic_level() const {return traffic_level;}

	void set_show_pax(bool yesno) {show_pax=yesno;}
	bool get_show_pax() const {return show_pax != 0;}

	sint16 get_groundwater() const {return groundwater;}

	double get_max_mountain_height() const {return max_mountain_height;}

	double get_map_roughness() const {return map_roughness;}

	uint16 get_station_coverage() const {return station_coverage_size;}

	uint16 get_station_coverage_factories() const {return station_coverage_size_factories;}

	void set_allow_player_change(char n) {allow_player_change=n;}	// prissi, Oct-2005
	uint8 get_allow_player_change() const {return allow_player_change;}

	void set_use_timeline(char n) {use_timeline=n;}	// prissi, Oct-2005
	uint8 get_use_timeline() const {return use_timeline;}

	void set_starting_year( sint16 n ) { starting_year = n; }
	sint16 get_starting_year() const {return starting_year;}

	void set_starting_month( sint16 n ) { starting_month = n; }
	sint16 get_starting_month() const {return starting_month;}

	sint16 get_bits_per_month() const {return bits_per_month;}

	void set_filename(const char *n) {filename=n;}	// prissi, Jun-06
	const char* get_filename() const { return filename.c_str(); }

	bool get_beginner_mode() const {return beginner_mode;}

	void set_just_in_time(uint8 v) { just_in_time = v; }
	uint8 get_just_in_time() const {return just_in_time;}

	void set_default_climates();
	const sint16 *get_climate_borders() const { return climate_borders; }

	sint16 get_winter_snowline() const {return winter_snowline;}

	void rotate90()
	{
		rotation = (rotation+1)&3;
		set_groesse( size_y, size_x, true);
		rotate_regions(size_y);
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
	void set_random_pedestrians( bool value ) { random_pedestrians = value; }

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

	void set_player_active(uint8 i, bool b) { player_active[i] = b; }
	void set_player_type(uint8 i, uint8 t) { player_type[i] = t; }
	uint8 get_player_type(uint8 i) const { return player_type[i]; }

	bool is_separate_halt_capacities() const { return separate_halt_capacities ; }

	uint16 get_meters_per_tile() const { return meters_per_tile; }
	void   set_meters_per_tile(uint16 value);
	uint32 get_steps_per_km() const { return steps_per_km; }

	uint32 get_base_meters_per_tile() const { return base_meters_per_tile; }
	uint32 get_base_bits_per_month() const { return base_bits_per_month; }
	uint32 get_job_replenishment_per_hundredths_of_months() const { return job_replenishment_per_hundredths_of_months; }

	uint8  get_tolerable_comfort_short() const { return tolerable_comfort_short; }
	uint16 get_tolerable_comfort_short_minutes() const { return tolerable_comfort_short_minutes; }
	uint8  get_tolerable_comfort_median_short() const { return tolerable_comfort_median_short; }
	uint16 get_tolerable_comfort_median_short_minutes() const { return tolerable_comfort_median_short_minutes; }
	uint8  get_tolerable_comfort_median_median() const { return tolerable_comfort_median_median; }
	uint16 get_tolerable_comfort_median_median_minutes() const { return tolerable_comfort_median_median_minutes; }
	uint8  get_tolerable_comfort_median_long() const { return tolerable_comfort_median_long; }
	uint16 get_tolerable_comfort_median_long_minutes() const { return tolerable_comfort_median_long_minutes; }
	uint8  get_tolerable_comfort_long() const { return tolerable_comfort_long; }
	uint16 get_tolerable_comfort_long_minutes() const { return tolerable_comfort_long_minutes; }
	void   cache_comfort_tables(); // Cache the list of values above in piecewise-linear functions.

	uint16 get_max_luxury_bonus_percent() const { return max_luxury_bonus_percent; }
	void   set_max_luxury_bonus_percent(uint16 value) { max_luxury_bonus_percent = value; }
	uint8  get_max_luxury_bonus_differential() const { return max_luxury_bonus_differential; }
	void   set_max_luxury_bonus_differential(uint8 value) { max_luxury_bonus_differential = value; }

	uint16 get_max_discomfort_penalty_percent() const { return max_discomfort_penalty_percent; }
	void   set_max_discomfort_penalty_percent(uint16 value) { max_discomfort_penalty_percent = value; }
	uint8  get_max_discomfort_penalty_differential() const { return max_discomfort_penalty_differential; }
	void   set_max_discomfort_penalty_differential(uint8 value) { max_discomfort_penalty_differential = value; }

	uint16 get_catering_min_minutes() const { return catering_min_minutes; }

	uint16 get_catering_level1_minutes() const { return catering_level1_minutes; }
	uint16 get_catering_level1_max_revenue() const { return catering_level1_max_revenue; }

	uint16 get_catering_level2_minutes() const { return catering_level2_minutes; }
	uint16 get_catering_level2_max_revenue() const { return catering_level2_max_revenue; }

	uint16 get_catering_level3_minutes() const { return catering_level3_minutes; }
	uint16 get_catering_level3_max_revenue() const { return catering_level3_max_revenue; }

	uint16 get_catering_level4_minutes() const { return catering_level4_minutes; }
	uint16 get_catering_level4_max_revenue() const { return catering_level4_max_revenue; }

	uint16 get_catering_level5_minutes() const { return catering_level5_minutes; }
	uint16 get_catering_level5_max_revenue() const { return catering_level5_max_revenue; }

	void   cache_catering_revenues(); // Cache the list of values above in piecewise-linear functions.

	uint16 get_tpo_min_minutes() const { return tpo_min_minutes; }
	void   set_tpo_min_minutes(uint16 value) { tpo_min_minutes = value; }
	uint16 get_tpo_revenue() const { return tpo_revenue; }
	void   set_tpo_revenue(uint16 value) { tpo_revenue = value; }

	uint16 get_obsolete_running_cost_increase_percent() const { return obsolete_running_cost_increase_percent; }
	void   set_obsolete_running_cost_increase_percent(uint16 value) { obsolete_running_cost_increase_percent = value; }
	uint16 get_obsolete_running_cost_increase_phase_years() const { return obsolete_running_cost_increase_phase_years; }
	void   set_obsolete_running_cost_increase_phase_years(uint16 value) { obsolete_running_cost_increase_phase_years = value; }

	uint8 get_passenger_routing_packet_size() const { return passenger_routing_packet_size; }

	uint16 get_max_alternative_destinations_visiting() const { return max_alternative_destinations_visiting; }
	// Subtract the minima from the maxima as these are added when the random number is generated.
	void update_max_alternative_destinations_visiting(uint32 global_visitor_demand) { max_alternative_destinations_visiting = max_alternative_destinations_per_visitor_demand_millionths > 0 ? (uint32)((((uint64)max_alternative_destinations_per_visitor_demand_millionths * (uint64)global_visitor_demand) / 1000000ul) + 1ul) - min_alternative_destinations_visiting : max_alternative_destinations_visiting - min_alternative_destinations_visiting; }
	uint16 get_max_alternative_destinations_commuting() const { return max_alternative_destinations_commuting; }
	void update_max_alternative_destinations_commuting(uint32 global_jobs) { max_alternative_destinations_commuting = max_alternative_destinations_per_job_millionths > 0 ? (uint32)((((uint64)max_alternative_destinations_per_job_millionths * (uint64)global_jobs) / 1000000ul) + 1ul) - min_alternative_destinations_commuting : max_alternative_destinations_commuting - min_alternative_destinations_commuting; }

	uint16 get_min_alternative_destinations_visiting() const { return min_alternative_destinations_visiting; }
	void update_min_alternative_destinations_visiting(uint32 global_visitor_demand) { min_alternative_destinations_visiting = min_alternative_destinations_per_visitor_demand_millionths > 0 ? (uint32)(((uint64)min_alternative_destinations_per_visitor_demand_millionths * (uint64)global_visitor_demand) / 1000000ul) : min_alternative_destinations_visiting; }
	uint16 get_min_alternative_destinations_commuting() const { return min_alternative_destinations_commuting; }
	void update_min_alternative_destinations_commuting(uint32 global_jobs) { min_alternative_destinations_commuting = min_alternative_destinations_per_job_millionths > 0 ? (uint32)(((uint64)min_alternative_destinations_per_job_millionths * (uint64)global_jobs) / 1000000ul) : min_alternative_destinations_commuting; }

	uint32 get_max_alternative_destinations_per_job_millionths() const { return max_alternative_destinations_per_job_millionths; }
	uint32 get_max_alternative_destinations_per_visitor_demand_millionths() const { return max_alternative_destinations_per_visitor_demand_millionths; }

	uint32 get_min_alternative_destinations_per_job_millionths() const { return min_alternative_destinations_per_job_millionths; }
	uint32 get_min_alternative_destinations_per_visitor_demand_millionths() const { return min_alternative_destinations_per_visitor_demand_millionths; }

	uint8 get_always_prefer_car_percent() const { return always_prefer_car_percent; }
	uint8 get_congestion_density_factor () const { return congestion_density_factor; }
	void set_congestion_density_factor (uint8 value)  { congestion_density_factor = value; }

	uint8 get_curve_friction_factor (waytype_t waytype) const { return curve_friction_factor[waytype]; }

	sint32 get_corner_force_divider(waytype_t waytype) const { return corner_force_divider[waytype]; }

	sint32 get_tilting_min_radius_effect() const { return tilting_min_radius_effect; }

	uint16 get_factory_max_years_obsolete() const { return factory_max_years_obsolete; }

	uint8 get_interest_rate_percent() const { return interest_rate_percent; }
	bool insolvency_allowed() const { return allow_insolvency; }
	bool insolvent_purchases_allowed() const { return allow_purchases_when_insolvent; }

	uint32 get_unit_reverse_time() const { return unit_reverse_time; }
	uint32 get_hauled_reverse_time() const { return hauled_reverse_time; }
	uint32 get_turntable_reverse_time() const { return turntable_reverse_time; }
	uint32 get_road_reverse_time() const { return road_reverse_time; }

	uint16 get_unit_reverse_time_seconds() const { return unit_reverse_time_seconds; }
	uint16 get_hauled_reverse_time_seconds() const { return hauled_reverse_time_seconds; }
	uint16 get_turntable_reverse_time_seconds() const { return turntable_reverse_time_seconds; }
	uint16 get_road_reverse_time_seconds() const { return road_reverse_time_seconds; }

	uint16 get_global_power_factor_percent() const { return global_power_factor_percent; }
	void set_global_power_factor_percent(uint16 value) { global_power_factor_percent = value; }
	uint16 get_global_force_factor_percent() const { return global_force_factor_percent; }
	void set_global_force_factor_percent(uint16 value) { global_force_factor_percent = value; }

	uint8 get_enforce_weight_limits() const { return enforce_weight_limits; }
	void set_enforce_weight_limits(uint8 value) { enforce_weight_limits = value; }

	bool get_allow_airports_without_control_towers() const { return allow_airports_without_control_towers; }
	void set_allow_airports_without_control_towers(bool value) { allow_airports_without_control_towers = value; }

	// allowed modes are 0,1,2
	enum { TO_PREVIOUS=0, TO_TRANSFER, TO_DESTINATION };


	bool is_avoid_overcrowding() const { return avoid_overcrowding; }

	uint32 get_passenger_max_wait() const { return passenger_max_wait; }

	uint8 get_max_rerouting_interval_months() const { return max_rerouting_interval_months; }

	sint16 get_river_number() const { return river_number; }
	sint16 get_min_river_length() const { return min_river_length; }
	sint16 get_max_river_length() const { return max_river_length; }

	// true, if this pak should be used with extensions (default)
	void set_with_private_paks(bool b ) {with_private_paks = b;}
	bool get_with_private_paks() const { return with_private_paks; }

	// @author: jamespetts
	uint32 get_min_visiting_tolerance() const { return min_visiting_tolerance; }
	void set_min_visiting_tolerance(uint16 value) { min_visiting_tolerance = value; }
	uint32 get_range_commuting_tolerance() const { return range_commuting_tolerance; }
	void set_range_commuting_tolerance(uint16 value) { range_commuting_tolerance = value; }
	uint32 get_min_commuting_tolerance() const { return min_commuting_tolerance; }
	void set_min_commuting_tolerance(uint16 value) { min_commuting_tolerance = value; }
	uint32 get_range_visiting_tolerance() const { return range_visiting_tolerance; }
	void set_range_visiting_tolerance(uint16 value) { range_visiting_tolerance = value; }

	// town growth stuff
	sint32 get_passenger_multiplier() const { return passenger_multiplier; }
	sint32 get_mail_multiplier() const { return mail_multiplier; }
	sint32 get_goods_multiplier() const { return goods_multiplier; }
	sint32 get_electricity_multiplier() const { return electricity_multiplier; }

	// Also there are size dependent factors (0=no growth)
	sint32 get_growthfactor_small() const { return growthfactor_small; }
	sint32 get_growthfactor_medium() const { return growthfactor_medium; }
	sint32 get_growthfactor_large() const { return growthfactor_large; }

	// Knightly : number of periods for averaging the amount of arrived pax/mail at factories
	uint16 get_factory_arrival_periods() const { return factory_arrival_periods; }

	// Knightly : whether factory pax/mail demands are enforced
	bool get_factory_enforce_demand() const { return factory_enforce_demand; }

	uint16 get_factory_maximum_intransit_percentage() const { return factory_maximum_intransit_percentage; }

	// disallow using obsolete vehicles in depot
	uint8 get_allow_buying_obsolete_vehicles() const { return allow_buying_obsolete_vehicles; }

	// forest stuff
	uint8 get_forest_base_size() const { return forest_base_size; }
	uint8 get_forest_map_size_divisor() const { return forest_map_size_divisor; }
	uint8 get_forest_count_divisor() const { return forest_count_divisor; }
	uint16 get_forest_inverse_spare_tree_density() const { return forest_inverse_spare_tree_density; }
	uint8 get_max_no_of_trees_on_square() const { return max_no_of_trees_on_square; }
	uint16 get_tree_climates() const { return tree_climates; }
	uint16 get_no_tree_climates() const { return no_tree_climates; }
	bool get_no_trees() const { return no_trees; }
	void set_no_trees(bool b) { no_trees = b; }

	bool get_lake() const { return lake; }
	void set_lake(bool b) { lake = b; }

	uint32 get_industry_increase_every() const { return industry_increase; }
	uint32 get_city_isolation_factor() const { return city_isolation_factor; }
	void set_industry_increase_every( uint32 n ) { industry_increase = n; }

	sint16 get_used_vehicle_reduction() const { return used_vehicle_reduction; }

	void set_default_player_color( player_t *player ) const;

	// usually only used in network mode => no need to set them!
	uint32 get_random_counter() const { return random_counter; }
	uint32 get_frames_per_second() const { return frames_per_second; }
	uint32 get_frames_per_step() const { return frames_per_step; }

	bool get_quick_city_growth() const { return quick_city_growth; }
	void set_quick_city_growth(bool value) { quick_city_growth = value; }
	bool get_assume_everywhere_connected_by_road() const { return assume_everywhere_connected_by_road; }
	void set_assume_everywhere_connected_by_road(bool value) { assume_everywhere_connected_by_road = value; }

	uint32 get_city_threshold_size() const { return city_threshold_size; }
	void set_city_threshold_size(uint32 value) { city_threshold_size = value; }
	uint32 get_capital_threshold_size() const { return capital_threshold_size; }
	void set_capital_threshold_size(uint32 value) { capital_threshold_size = value; }
	uint32 get_max_small_city_size() const { return max_small_city_size; }
	void set_max_small_city_size(uint32 value) { max_small_city_size = value; }
	uint32 get_max_city_size() const { return max_city_size; }
	void set_max_city_size(uint32 value) { max_city_size = value; }

	uint8 get_capital_threshold_percentage() const { return capital_threshold_percentage; }
	void set_capital_threshold_percentage(uint8 value) { capital_threshold_percentage = value; }
	uint8 get_city_threshold_percentage() const { return city_threshold_percentage; }
	void set_city_threshold_percentage(uint8 value) { city_threshold_percentage = value; }

	uint16 get_default_increase_maintenance_after_years(waytype_t wtype) const { return default_increase_maintenance_after_years[wtype]; }
	void set_default_increase_maintenance_after_years(waytype_t wtype, uint16 value) { default_increase_maintenance_after_years[wtype] = value; }
	uint32 get_server_frames_ahead() const { return server_frames_ahead; }

	uint8 get_spacing_shift_mode() const { return spacing_shift_mode; }
	void set_spacing_shift_mode(uint8 s) { spacing_shift_mode = s; }

	sint16 get_spacing_shift_divisor() const { return spacing_shift_divisor; }
	void set_spacing_shift_divisor(sint16 s) { spacing_shift_divisor = s; }

	class livery_scheme_t* get_livery_scheme(uint16 index) { return !livery_schemes.empty() ? livery_schemes.get_element(index) : NULL; }
	vector_tpl<class livery_scheme_t*>* get_livery_schemes() { return &livery_schemes; }

	bool get_allow_routing_on_foot() const { return allow_routing_on_foot; }
	void set_allow_routing_on_foot(bool value);

	bool is_drive_left() const { return drive_on_left; }
	bool is_signals_left() const { return signals_on_left; }

	uint16 get_min_wait_airport() const { return min_wait_airport; }
	void set_min_wait_airport(uint16 value) { min_wait_airport = value; }

	sint32 get_way_toll_runningcost_percentage() const { return way_toll_runningcost_percentage; }
	sint32 get_way_toll_waycost_percentage() const { return way_toll_waycost_percentage; }
	sint32 get_way_toll_revenue_percentage() const { return way_toll_revenue_percentage; }
	sint32 get_seaport_toll_revenue_percentage() const { return seaport_toll_revenue_percentage; }
	sint32 get_airport_toll_revenue_percentage() const { return airport_toll_revenue_percentage; }

	bool get_toll_free_public_roads() const { return toll_free_public_roads; }

	bool get_allow_making_public() const { return allow_making_public; }

	sint64 get_private_car_toll_per_km() const { return private_car_toll_per_km; }

	bool get_towns_adopt_player_roads() const { return towns_adopt_player_roads; }

	uint32 get_reroute_check_interval_steps() const { return reroute_check_interval_steps; }

	uint8 get_walking_speed() const { return walking_speed; }

	uint32 get_random_mode_commuting() const { return random_mode_commuting; }
	uint32 get_random_mode_visiting() const { return random_mode_visiting; }

	uint32 get_tolerance_modifier_percentage() const { return tolerance_modifier_percentage; }

	uint32 get_industry_density_proportion_override() const { return industry_density_proportion_override; }

#ifndef NETTOOL
	float32e8_t get_simtime_factor() const { return simtime_factor; }
	float32e8_t meters_to_steps(const float32e8_t &meters) const { return steps_per_meter * meters; }
	float32e8_t steps_to_meters(const float32e8_t &steps) const { return meters_per_step * steps; }
	float32e8_t ticks_to_seconds(sint32 delta_t) const { return seconds_per_tick * delta_t; }
#endif
	uint8 get_max_elevated_way_building_level() const { return max_elevated_way_building_level; }
	void set_max_elevated_way_building_level(uint8 value) { max_elevated_way_building_level = value; }

	bool get_allow_underground_transformers() const { return allow_underground_transformers; }
	bool get_disable_make_way_public() const { return disable_make_way_public; }
	void set_scale();

	uint16 get_remove_dummy_player_months() const { return remove_dummy_player_months; }
	uint16 get_unprotect_abandoned_player_months() const { return unprotect_abandoned_player_months; }

	uint16 get_population_per_level() const { return population_per_level; }
	uint16 get_visitor_demand_per_level() const { return visitor_demand_per_level; }
	uint16 get_jobs_per_level() const { return jobs_per_level; }
	uint16 get_mail_per_level() const { return mail_per_level; }
	uint32 get_power_revenue_factor_percentage() const { return power_revenue_factor_percentage; }

	uint32 get_passenger_trips_per_month_hundredths() const { return passenger_trips_per_month_hundredths; }
	uint32 get_mail_packets_per_month_hundredths() const { return mail_packets_per_month_hundredths; }

	uint32 get_max_onward_trips() const { return max_onward_trips; }

	uint16 get_onward_trip_chance_percent() const { return onward_trip_chance_percent; }
	uint16 get_commuting_trip_chance_percent() const { return commuting_trip_chance_percent; }

	// This is the number of ticks that must elapse before a single job is replenished.
	sint64 get_job_replenishment_ticks() const { return job_replenishment_ticks; }
	void calc_job_replenishment_ticks();

	sint64 get_forge_cost_road() const { return forge_cost_road; }
	sint64 get_forge_cost_track() const { return forge_cost_track; }
	sint64 get_forge_cost_water() const { return forge_cost_water; }
	sint64 get_forge_cost_monorail() const { return forge_cost_monorail; }
	sint64 get_forge_cost_maglev() const { return forge_cost_maglev; }
	sint64 get_forge_cost_tram() const { return forge_cost_tram; }
	sint64 get_forge_cost_narrowgauge() const { return forge_cost_narrowgauge; }
	sint64 get_forge_cost_air() const { return forge_cost_air; }

	uint16 get_parallel_ways_forge_cost_percentage_road() const { return parallel_ways_forge_cost_percentage_road; }
	uint16 get_parallel_ways_forge_cost_percentage_track() const { return parallel_ways_forge_cost_percentage_track; }
	uint16 get_parallel_ways_forge_cost_percentage_water() const { return parallel_ways_forge_cost_percentage_water; }
	uint16 get_parallel_ways_forge_cost_percentage_monorail() const { return parallel_ways_forge_cost_percentage_monorail; }
	uint16 get_parallel_ways_forge_cost_percentage_maglev() const { return parallel_ways_forge_cost_percentage_maglev; }
	uint16 get_parallel_ways_forge_cost_percentage_tram() const { return parallel_ways_forge_cost_percentage_tram; }
	uint16 get_parallel_ways_forge_cost_percentage_narrowgauge() const { return parallel_ways_forge_cost_percentage_narrowgauge; }
	uint16 get_parallel_ways_forge_cost_percentage_air() const { return parallel_ways_forge_cost_percentage_air; }

	sint64 get_forge_cost(waytype_t wt) const;
	sint64 get_parallel_ways_forge_cost_percentage(waytype_t wt) const;

	uint32 get_max_diversion_tiles() const { return max_diversion_tiles; }

	sint8 get_way_height_clearance() const { return way_height_clearance; }
	void set_way_height_clearance( sint8 n ) { way_height_clearance = n; }

	uint32 get_default_ai_construction_speed() const { return default_ai_construction_speed; }
	void set_default_ai_construction_speed( uint32 n ) { default_ai_construction_speed = n; }

	uint32 get_way_degradation_fraction() const { return way_degradation_fraction; }

	uint32 get_way_wear_power_factor_road_type() const { return way_wear_power_factor_road_type; }
	uint32 get_way_wear_power_factor_rail_type() const { return way_wear_power_factor_rail_type; }
	uint16 get_standard_axle_load() const { return standard_axle_load; }
	uint32 get_citycar_way_wear_factor() const { return citycar_way_wear_factor; }

	uint32 get_sighting_distance_meters() const { return sighting_distance_meters; }
	uint16 get_sighting_distance_tiles() const { return sighting_distance_tiles; }

	uint32 get_assumed_curve_radius_45_degrees() const { return assumed_curve_radius_45_degrees; }

	sint32 get_max_speed_drive_by_sight_kmh() const { return max_speed_drive_by_sight_kmh; }
	sint32 get_max_speed_drive_by_sight() const { return max_speed_drive_by_sight; }

	uint32 get_time_interval_seconds_to_clear() const { return time_interval_seconds_to_clear; }
	uint32 get_time_interval_seconds_to_caution() const { return time_interval_seconds_to_caution; }

	uint32 get_town_road_speed_limit() const { return town_road_speed_limit; }

	uint32 get_minimum_staffing_percentage_consumer_industry() const { return minimum_staffing_percentage_consumer_industry; }
	uint32 get_minimum_staffing_percentage_full_production_producer_industry() const { return minimum_staffing_percentage_full_production_producer_industry; }

	uint16 get_max_comfort_preference_percentage() const { return max_comfort_preference_percentage; }

	bool get_rural_industries_no_staff_shortage() const { return rural_industries_no_staff_shortage; }
	uint32 get_auto_connect_industries_and_attractions_by_road() const { return auto_connect_industries_and_attractions_by_road; }

	uint32 get_path_explorer_time_midpoint() const { return path_explorer_time_midpoint; }
	bool get_save_path_explorer_data() const { return save_path_explorer_data; }

	bool get_show_future_vehicle_info() const { return show_future_vehicle_info; }
	//void set_show_future_vehicle_info(bool yesno) { show_future_vehicle_info = yesno; }

	uint32 get_max_route_tiles_to_process_in_a_step() const { return max_route_tiles_to_process_in_a_step; }
	void set_max_route_tiles_to_process_in_a_step(uint32 value) { max_route_tiles_to_process_in_a_step = value; }
};

#endif
