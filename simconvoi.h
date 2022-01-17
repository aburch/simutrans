/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMCONVOI_H
#define SIMCONVOI_H


#include "simtypes.h"
#include "simunits.h"
#include "simcolor.h"
#include "linehandle_t.h"

#include "ifc/sync_steppable.h"

#include "dataobj/route.h"
#include "vehicle/overtaker.h"
#include "tpl/array_tpl.h"
#include "tpl/minivec_tpl.h"

#include "convoihandle_t.h"
#include "halthandle_t.h"

#define MAX_MONTHS               12 // Max history

class weg_t;
class depot_t;
class karte_ptr_t;
class player_t;
class vehicle_t;
class vehicle_desc_t;
class schedule_t;
class cbuffer_t;

/**
 * Base class for all vehicle consists. Convoys can be referenced by handles, see halthandle_t.
 */
class convoi_t : public sync_steppable, public overtaker_t
{
public:
	enum {
		CONVOI_CAPACITY = 0,       // the amount of ware that could be transported, theoretically
		CONVOI_TRANSPORTED_GOODS,  // the amount of ware that has been transported
		CONVOI_REVENUE,            // the income this CONVOI generated
		CONVOI_OPERATIONS,         // the cost of operations this CONVOI generated
		CONVOI_PROFIT,             // total profit of this convoi
		CONVOI_DISTANCE,           // total distance traveled this month
		CONVOI_MAXSPEED,           // average max. possible speed
		CONVOI_WAYTOLL,
		MAX_CONVOI_COST            // Total number of cost items
	};

	/** Constants */
	enum { default_vehicle_length = 4 };

	enum states {
		INITIAL,
		EDIT_SCHEDULE,
		ROUTING_1,
		DUMMY4,
		DUMMY5,
		NO_ROUTE,
		DRIVING,
		LOADING,
		WAITING_FOR_CLEARANCE,
		WAITING_FOR_CLEARANCE_ONE_MONTH,
		CAN_START,
		CAN_START_ONE_MONTH,
		SELF_DESTRUCT,
		WAITING_FOR_CLEARANCE_TWO_MONTHS,
		CAN_START_TWO_MONTHS,
		LEAVING_DEPOT,
		ENTERING_DEPOT,
		MAX_STATES
	};

private:
	/* The data is laid out such that the most important variables for sync_step and step are
	 * concentrated at the beginning of the structure.
	 * All computations are for 64bit builds.
	 *
	 * We start with a 24 bytes header from virtual tables and data of overtaker_t :(
	 */

	/**
	 * The convoi is not processed every sync step for various actions
	 * (like waiting before signals, loading etc.) Such action will only
	 * continue after a waiting time larger than wait_lock
	 */
	sint32 wait_lock;

	/**
	 * The convoi is gradually loaded for each vehicle, i.e. there is a constant loadin speed.
	 * Hence the tick value
	 */
	uint32 last_load_tick;

	states state;
	// 32 bytes (state is int is 4 byte)

	/**
	* holds id of line with pending update
	* -1 if no pending update
	*/
	linehandle_t line_update_pending;

	uint16 recalc_data_front  : 1; ///< true, when front vehicle has to recalculate braking
	uint16 recalc_data        : 1; ///< true, when convoy has to recalculate weights and speed limits
	uint16 recalc_speed_limit : 1; ///< true, when convoy has to recalculate speed limits

	uint16 previous_delta_v   :12; /// 12 bit! // Stores the previous delta_v value; otherwise these digits are lost during calculation and vehicle do not accelerate
	// 36 bytes
	/**
	 * Overall performance with Gear.
	 * Used in movement calculations.
	 */
	sint32 sum_gear_and_power;

	// 40 bytes
	/**
	 * sum_weight: unloaded weight of all vehicles
	 * sum_gesamtweight: total weight of all vehicles
	 * Not stored, but calculated from individual weights
	 * when loading/driving.
	 */
	sint64 sum_gesamtweight;
	sint64 sum_friction_weight;
	// 56 bytes
	sint32 akt_speed_soll;    // target speed
	sint32 akt_speed;         // current speed
	// 64 bytes

	/**
	 * this give the index of the next signal or the end of the route
	 * convois will slow down before it, if this is not a waypoint or the cannot pass
	 * The slowdown is done by the vehicle routines
	 */
	route_t::index_t next_stop_index;

	// if state == LOADING, this is true while unloading
	bool unloading_state;

	sint32 speed_limit;
	// needed for speed control/calculation
	sint32 brake_speed_soll;    // brake target speed
	/**
	 * Lowest top speed of all vehicles. Doesn't get saved, but calculated
	 * from the vehicles data
	 */
	sint32 min_top_speed;

	sint32 sp_soll;           // steps to go
	sint32 max_record_speed; // current convois fastest speed ever

	// things for the world record
	koord record_pos;

	/* Number of steps the current convoi did already
	 * (only needed for leaving/entering depot)
	 */
	sint16 steps_driven;
	/**
	 * The vehicles of this convoi
	 *
	 */
	array_tpl<vehicle_t*> fahr;
	/**
	 * Number of vehicles in this convoi.
	 */
	// TODO number of vehicles is stored in array_tpl too!
	uint8 vehicle_count;

	uint32 next_wolke; // time to next smoke

	/**
	 * Route of this convoi - a sequence of coordinates. Actually
	 * the path of the first vehicle
	 */
	route_t route;

	/**
	 * assigned line
	 */
	linehandle_t line;

	/**
	* All vehicle-schedule pointers point here
	*/
	schedule_t *schedule;

	koord3d schedule_target;

	/**
	* loading_level was minimum_loading before. Actual percentage loaded for loadable vehicles (station length!).
	* needed as int, since used by the gui
	*/
	sint32 loading_level;

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* needed as int, since used by the gui
	*/
	sint32 loading_limit;

	/*
	 * a list of all catg_index, which can be transported by this convoy.
	 */
	minivec_tpl<uint8> goods_catg_index;

	/**
	* Convoi owner
	*/
	player_t *owner;

	/**
	* Current map
	*/
	static karte_ptr_t welt;

	/**
	* the convoi is being withdrawn from service
	*/
	bool withdraw;

	/**
	* nothing will be loaded onto this convoi
	*/
	bool no_load;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	*/
	bool freight_info_resort;

	// true, if at least one vehicle of a convoi is obsolete
	bool has_obsolete;

	// true, if there is at least one engine that requires catenary
	bool is_electric;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	*/
	uint8 freight_info_order;

	/*
	 * caches the running costs
	 */
	sint32 sum_running_costs;
	sint32 sum_fixed_costs;

	/**
	* Overall performance.
	* Not used in movement code.
	*/
	uint32 sum_power;

	/// sum_weight: unloaded weight of all vehicles
	sint64 sum_weight;

	/**
	 * this give the index until which the route has been reserved. It is used for
	 * restoring reservations after loading a game.
	 */
	route_t::index_t next_reservation_index;

	/**
	 * Time when convoi arrived at the current stop
	 * Used to calculate when it should depart due to the 'month wait time'
	 */
	uint32 arrived_time;

	/**
	* accumulated profit over a year
	*/
	sint64 jahresgewinn;

	/* the odometer */
	sint64 total_distance_traveled;

	uint32 distance_since_last_stop; // number of tiles entered since last stop
	uint32 sum_speed_limit; // sum of the speed limits encountered since the last stop

	sint32 speedbonus_kmh; // speed used for speedbonus calculation in km/h
	sint32 maxspeed_average_count; // just a simple count to average for statistics


	ribi_t::ribi alte_richtung;

	/**
	 * struct holds new financial history for convoi
	 */
	sint64 financial_history[MAX_MONTHS][MAX_CONVOI_COST];


	/**
	 * the koordinate of the home depot of this convoi
	 * the last depot visited is considered being the home depot
	 */
	koord3d home_depot;

	/**
	 * Name of the convoi.
	 * @see set_name
	 */
	uint8 name_offset;
	char name_and_id[128];

	/**
	* Initialize all variables with default values.
	* Each constructor must call this method first!
	*/
	void init(player_t *player);

	/**
	* Calculate route from Start to Target Coordinate
	*/
	bool drive_to();

	/**
	* Setup vehicles for moving in same direction than before
	* if the direction is the same as before
	*/
	bool can_go_alte_richtung();

	/**
	 * remove all track reservations (trains only)
	 */
	void unreserve_route();

	// reserve route until next_reservation_index
	void reserve_route();

	/**
	* Mark first and last vehicle.
	*/
	void set_erstes_letztes();

	// returns the index of the vehicle at position length (16=1 tile)
	int get_vehicle_at_length(uint16);

	/**
	* calculate income for last hop
	* only used for entering depot or recalculating routes when a schedule window is opened
	*/
	void calc_gewinn();

	/**
	* Recalculates loading level and limit.
	* While driving loading_limit will be set to 0.
	*/
	void calc_loading();

	/* Calculates (and sets) akt_speed
	 * needed for driving, entering and leaving a depot)
	 */
	void calc_acceleration(uint32 delta_t);

	/**
	* initialize the financial history
	*/
	void init_financial_history();

	/**
	* unset line -> remove cnv from line
	*/
	void unset_line();

	// matches two halts; if the pos is not identical, maybe the halt still is
	bool matches_halt( const koord3d pos1, const koord3d pos2 );

	/**
	 * Register the convoy with the stops in the schedule
	 */
	void register_stops();

	/**
	 * Unregister the convoy from the stops in the schedule
	 */
	void unregister_stops();

	uint32 move_to(route_t::index_t start_index);

public:
	uint32 get_arrival_ticks() const { return arrived_time; }

	uint32 get_departure_ticks(uint32) const;

	uint32 get_departure_ticks() const { return get_departure_ticks(arrived_time); }

	/**
	* Convoi haelt an Haltestelle und setzt quote fuer Fracht
	*/
	void hat_gehalten(halthandle_t halt);

	const route_t* get_route() const { return &route; }
	route_t* access_route() { return &route; }

	const koord3d get_schedule_target() const { return schedule_target; }
	void set_schedule_target( koord3d t ) { schedule_target = t; }

	/**
	* get line
	*/
	linehandle_t get_line() const {return line;}

	/* true, if electrification needed for this convoi */
	bool needs_electrification() const { return is_electric; }

	/**
	* set line
	*/
	void set_line(linehandle_t );

	// updates a line schedule and tries to find the best next station to go
	void check_pending_updates();

	// true if this is a waypoint
	bool is_waypoint( koord3d ) const;

	/* changes the state of a convoi via tool_t; mandatory for networkmode!
	 * for list of commands and parameter see tool_t::tool_change_convoi_t
	 */
	void call_convoi_tool( const char function, const char *extra ) const;

	/**
	 * set state: only use by tool_t::tool_change_convoi_t
	 */
	void set_state( uint16 new_state ) { assert(new_state<MAX_STATES); state = (states)new_state; }

	/**
	* get state
	*/
	int get_state() const { return state; }

	/**
	* true if in waiting state (maybe also due to starting)
	*/
	bool is_waiting() { return (state>=WAITING_FOR_CLEARANCE  &&  state<=CAN_START_TWO_MONTHS)  &&  state!=SELF_DESTRUCT; }

	bool is_unloading() { return state==LOADING  &&  unloading_state; }

	/**
	* reset state to no error message
	*/
	void reset_waiting() { state=WAITING_FOR_CLEARANCE; }

	/**
	* The handle for ourselves. In Anlehnung an 'this' aber mit
	* allen checks beim Zugriff.
	*/
	convoihandle_t self;

	/**
	 * The profit in this year
	 */
	const sint64 & get_jahresgewinn() const {return jahresgewinn;}

	const sint64 & get_total_distance_traveled() const { return total_distance_traveled; }

	/**
	 * @return the total monthly fix cost for all vehicles in convoi
	 */
	sint64 get_fixed_cost() const { return -sum_fixed_costs; }

	/**
	 * returns the total running cost for all vehicles in convoi
	 */
	sint32 get_running_cost() const { return -sum_running_costs; }

	/**
	 * returns the total new purchase cost for all vehicles in convoy
	 */
	sint64 get_purchase_cost() const;

	/**
	* Constructor for loading from file,
	*/
	convoi_t(loadsave_t *file);

	convoi_t(player_t* player);

	virtual ~convoi_t();

	/**
	* Load or save this convoi data
	*/
	void rdwr(loadsave_t *file);

	/**
	 * method to load/save convoihandle_t
	 */
	static void rdwr_convoihandle_t(loadsave_t *file, convoihandle_t &cnv);

	void finish_rd();

	void rotate90( const sint16 y_size );

	/**
	* Called if a vehicle enters a depot
	*/
	void betrete_depot(depot_t *dep);

	/**
	* Return the internal name of the convois
	* @return Name of the convois
	*/
	const char *get_internal_name() const {return name_and_id+name_offset;}

	/**
	* Allows editing ...
	* @return Name of the Convois
	*/
	char *access_internal_name() {return name_and_id+name_offset;}

	/**
	* Return the name of the convois
	* @return Name of the convois
	*/
	const char *get_name() const {return name_and_id;}

	/**
	* Sets the name. Copies name into this->name and translates it.
	*/
	void set_name(const char *name, bool with_new_id = true);

	/**
	 * Return the position of the convois.
	 * @return Position of the convois
	 */
	koord3d get_pos() const;

	/**
	 * @return current speed, this might be different from topspeed
	 *         actual currently set speed.
	 */
	const sint32& get_akt_speed() const { return akt_speed; }

	/**
	 * @return total power of this convoi
	 */
	const uint32 & get_sum_power() const {return sum_power;}
	const sint32 & get_min_top_speed() const {return min_top_speed;}
	const sint32 & get_speed_limit() const {return speed_limit;}

	void set_speed_limit(sint32 s) { speed_limit = s;}

	/// @returns weight of the convoy's vehicles (excluding freight)
	const sint64 & get_sum_weight() const {return sum_weight;}

	/// @returns weight of convoy including freight
	const sint64 & get_sum_gesamtweight() const {return sum_gesamtweight;}

	/// changes sum_friction_weight, called when vehicle changed tile and friction changes as well.
	void update_friction_weight(sint64 delta_friction_weight) { sum_friction_weight += delta_friction_weight; }

	/// @returns theoretical max speed of a convoy with given @p total_power and @p total_weight
	static sint32 calc_max_speed(uint64 total_power, uint64 total_weight, sint32 speed_limit);

	uint32 get_length() const;

	/**
	 * @return length of convoi in the correct units for movement
	 */
	uint32 get_length_in_steps() const { return get_length() * VEHICLE_STEPS_PER_CARUNIT; }

	/**
	 * Add the costs for travelling one tile
	 */
	void add_running_cost( const weg_t *weg );

	/**
	 * moving the vehicles of a convoi and acceleration/deceleration
	 * all other stuff => convoi_t::step()
	 */
	sync_result sync_step(uint32 delta_t) OVERRIDE;

	/**
	 * All things like route search or loading, that may take a little
	 */
	void step();

	/**
	* sets a new convoi in route
	*/
	void start();

	void ziel_erreicht(); ///< Called, when the first vehicle reaches the target

	/**
	* When a vehicle has detected a problem
	* force calculate a new route
	*/
	void suche_neue_route();

	/**
	* Wait until vehicle 0 reports free route
	* will be called during a hop_check, if the road/track is blocked
	*/
	void warten_bis_weg_frei(sint32 restart_speed);

	/**
	* @return Vehicle count
	*/
	uint8 get_vehicle_count() const { return vehicle_count; }

	/**
	 * @return Vehicle at position i
	 */
	vehicle_t* get_vehicle(uint16 i) const { return fahr[i]; }

	vehicle_t* front() const { return fahr[0]; }

	vehicle_t* back() const { return fahr[vehicle_count - 1]; }

	/**
	* Adds a vehicle at the start or end of the convoi.
	*/
	bool add_vehicle(vehicle_t *v, bool infront = false);

	/**
	* Removes vehicles at position i
	*/
	vehicle_t * remove_vehicle_at(unsigned short i);

	const minivec_tpl<uint8> &get_goods_catg_index() const { return goods_catg_index; }

	// recalculates the good transported by this convoy and (in case of changes) will start schedule recalculation
	void recalc_catg_index();

	/**
	* Sets a schedule
	*/
	bool set_schedule(schedule_t *f);

	schedule_t* get_schedule() const { return schedule; }

	/**
	* Creates a new schedule if there isn't one already.
	*/
	schedule_t * create_schedule();

	// remove wrong freight when schedule changes etc.
	void check_freight();

	/**
	* @return Owner of this convoi
	*/
	player_t * get_owner() const { return owner; }

	/**
	* Opens an information window
	* @see simwin
	*/
	void open_info_window();

	/**
	* @param[out] buf a description string for the object, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @see simwin
	*/
	void info(cbuffer_t & buf) const;

	/**
	* @param[out] buf Filled with freight description
	*/
	void get_freight_info(cbuffer_t & buf);
	void set_sortby(uint8 order);
	uint8 get_sortby() const { return freight_info_order; }

	/**
	* Opens the schedule window
	* @see simwin
	*/
	void open_schedule_window( bool show );

	/**
	* pruefe ob Beschraenkungen fuer alle Fahrzeuge erfuellt sind
	*/
	bool pruefe_alle();

	/**
	* Control loading and unloading
	*/
	void laden();

	/**
	* Setup vehicles before starting to move
	*/
	void vorfahren();

	/**
	* Calculate the total value of the convoi as the sum of all vehicle values.
	*/
	sint64 calc_restwert() const;

	/**
	* Check if this convoi has entered a depot.
	*/
	bool in_depot() const { return state == INITIAL; }

	/**
	* loading_level was minimum_loading before. Actual percentage loaded of loadable
	* vehicles.
	*/
	const sint32 &get_loading_level() const { return loading_level; }

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	*/
	const sint32 &get_loading_limit() const { return loading_limit; }

	/**
	* Schedule convois for self destruction. Will be executed
	* upon next sync step
	*/
	void self_destruct();

	/**
	* Helper method to remove convois from the map that cannot
	* removed normally (i.e. by sending to a depot) anymore.
	* This is a workaround for bugs in the game.
	*/
	void destroy();

	/**
	* Debug info to stderr
	*/
	void dump() const;

	/**
	* book a certain amount into the convois financial history
	* is called from vehicle during un/load
	*/
	void book(sint64 amount, int cost_type);

	/**
	* return a pointer to the financial history
	*/
	sint64* get_finance_history() { return *financial_history; }

	/**
	* return a specified element from the financial history
	*/
	sint64 get_finance_history(int month, int cost_type) const { return financial_history[month][cost_type]; }
	sint64 get_stat_converted(int month, int cost_type) const;

	/**
	* only purpose currently is to roll financial history
	*/
	void new_month();

	/**
	 * Method for yearly action
	 */
	void new_year();

	void set_update_line(linehandle_t l) { line_update_pending = l; }

	void set_home_depot(koord3d hd) { home_depot = hd; }

	koord3d get_home_depot() { return home_depot; }

	/**
	 * Sends convoi to nearest depot.
	 * Has to be called synchronously on all clients in networkmode!
	 * @returns success message
	 */
	const char* send_to_depot(bool local);

	/**
	 * this give the index of the next signal or the end of the route
	 * convois will slow down before it, if this is not a waypoint or the cannot pass
	 * The slowdown is done by the vehicle routines
	 */
	route_t::index_t get_next_stop_index() const { return next_stop_index; }
	void set_next_stop_index(route_t::index_t n);

	/* including this route_index, the route was reserved the last time
	 * currently only used for tracks
	 */
	route_t::index_t get_next_reservation_index() { return next_reservation_index; }
	void set_next_reservation_index(route_t::index_t n);

	/* the current state of the convoi */
	PIXVAL get_status_color() const;

	// returns station tiles needed for this convoi
	uint16 get_tile_length() const;

	bool has_obsolete_vehicles() const { return has_obsolete; }

	bool get_withdraw() const { return withdraw; }

	void set_withdraw(bool new_withdraw);

	bool get_no_load() const { return no_load; }

	void set_no_load(bool new_no_load) { no_load = new_no_load; }

	void must_recalc_data() { recalc_data = true; }
	void must_recalc_data_front() { recalc_data_front = true; }
	void must_recalc_speed_limit() { recalc_speed_limit = true; }

	// calculates the speed used for the speedbonus base, and the max achievable speed at current power/weight for overtakers
	void calc_speedbonus_kmh();
	sint32 get_speedbonus_kmh() const;

	// just a guess of the speed
	uint32 get_average_kmh() const;

	// Overtaking for convois
	bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other) OVERRIDE;
};

#endif
