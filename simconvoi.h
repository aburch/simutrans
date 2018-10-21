/**
 * @file
 * Contains definition of convoi_t class
 */

#ifndef simconvoi_h
#define simconvoi_h

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
class signal_t;

/**
 * Base class for all vehicle consists. Convoys can be referenced by handles, see halthandle_t.
 *
 * @author Hj. Malthaner
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

	/* Constants
	 * @author prissi
	 */
	enum { default_vehicle_length=4};

	enum states {INITIAL,
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
	/**
	* Route of this convoi - a sequence of coordinates. Actually
	* the path of the first vehicle
	* @author Hj. Malthaner
	*/
	route_t route;

	/**
	* assigned line
	* @author hsiegeln
	*/
	linehandle_t line;

	/**
	* holds id of line with pending update
	* -1 if no pending update
	* @author hsiegeln
	*/
	linehandle_t line_update_pending;

	/**
	* Name of the convoi.
	* @see set_name
	* @author V. Meyer
	*/
	uint8 name_offset;
	char name_and_id[128];

	/**
	* All vehicle-schedule pointers point here
	* @author Hj. Malthaner
	*/
	schedule_t *schedule;

	koord3d schedule_target;

	/**
	* loading_level was minimum_loading before. Actual percentage loaded for loadable vehicles (station length!).
	* needed as int, since used by the gui
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	sint32 loading_level;

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* needed as int, since used by the gui
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	sint32 loading_limit;

	/**
	* The vehicles of this convoi
	*
	* @author Hj. Malthaner
	*/
	array_tpl<vehicle_t*> fahr;

	/*
	 * a list of all catg_index, which can be transported by this convoy.
	 */
	minivec_tpl<uint8> goods_catg_index;

	/**
	* Convoi owner
	* @author Hj. Malthaner
	*/
	player_t *owner_p;

	/**
	* Current map
	* @author Hj. Malthaner
	*/
	static karte_ptr_t welt;

	/**
	* the convoi is being withdrawn from service
	* @author kierongreen
	*/
	bool withdraw;

	/**
	* nothing will be loaded onto this convoi
	* @author kierongreen
	*/
	bool no_load;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	* @author prissi
	*/
	bool freight_info_resort;

	// true, if at least one vehicle of a convoi is obsolete
	bool has_obsolete;

	// true, if there is at least one engine that requires catenary
	bool is_electric;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	* @author prissi
	*/
	uint8 freight_info_order;

	/**
	* Number of vehicles in this convoi.
	* @author Hj. Malthaner
	*/
	uint8 anz_vehikel;

	/* Number of steps the current convoi did already
	 * (only needed for leaving/entering depot)
	 */
	sint16 steps_driven;

	/*
	 * caches the running costs
	 */
	sint32 sum_running_costs;

	/**
	* Overall performance.
	* Not stored, but calculated from individual parts
	* @author Hj. Malthaner
	*/
	uint32 sum_power;

	/**
	* Overall performance with Gear.
	* Not stored, but calculated from individual parts
	* @author prissi
	*/
	sint32 sum_gear_and_power;

	/* sum_weight: unloaded weight of all vehicles *
	* sum_gesamtweight: total weight of all vehicles *
	* Not stored, but calculated from individual weights
	* when loading/driving.
	* @author Hj. Malthaner, prissi
	*/
	sint64 sum_weight;
	sint64 sum_gesamtweight;

	bool recalc_data_front;  ///< true, when front vehicle has to recalculate braking
	bool recalc_data;        ///< true, when convoy has to recalculate weights and speed limits
	bool recalc_speed_limit; ///< true, when convoy has to recalculate speed limits

	sint64 sum_friction_weight;
	sint32 speed_limit;

	/**
	* Lowest top speed of all vehicles. Doesn't get saved, but calculated
	* from the vehicles data
	* @author Hj. Malthaner
	*/
	sint32 min_top_speed;

	/**
	 * this give the index of the next signal or the end of the route
	 * convois will slow down before it, if this is not a waypoint or the cannot pass
	 * The slowdown is done by the vehicle routines
	 * @author prissi
	 */
	uint16 next_stop_index;

	/**
	 * this give the index until which the route has been reserved. It is used for
	 * restoring reservations after loading a game.
	 * @author prissi
	 */
	uint16 next_reservation_index;
	
	/**
	 * This holds coordinates reserved by this convoy.
	 * Used when reservation is triggered by longblocksignal.
	 * @author THLeaderH
	 */
	vector_tpl<koord3d> reserved_tiles;

	/**
	 * The convoi is not processed every sync step for various actions
	 * (like waiting before signals, loading etc.) Such action will only
	 * continue after a waiting time larger than wait_lock
	 * @author Hanjsörg Malthaner
	 */
	sint32 wait_lock;

	/**
	 * Time when convoi arrived at the current stop
	 * Used to calculate when it should depart due to the 'month wait time'
	 */
	uint32 arrived_time;

	/**
	 *The flag whether this convoi is requested to change lane by the convoi behind this.
	 *@author teamhimeH
	 */
	bool requested_change_lane;

	/**
	* accumulated profit over a year
	* @author Hanjsörg Malthaner
	*/
	sint64 jahresgewinn;

	/* the odometer */
	sint64 total_distance_traveled;

	uint32 distance_since_last_stop; // number of tiles entered since last stop
	uint32 sum_speed_limit; // sum of the speed limits encountered since the last stop

	sint32 speedbonus_kmh; // speed used for speedbonus calculation in km/h
	sint32 maxspeed_average_count;	// just a simple count to average for statistics

	// things for the world record
	sint32 max_record_speed; // current convois fastest speed ever
	koord record_pos;

	// needed for speed control/calculation
	sint32 brake_speed_soll;    // brake target speed
	sint32 akt_speed_soll;    // target speed
	sint32 akt_speed;	        // current speed
	sint32 sp_soll;           // steps to go
	sint32 previous_delta_v;  // Stores the previous delta_v value; otherwise these digits are lost during calculation and vehicle do not accelerate

	uint32 next_wolke;	// time to next smoke

	states state;

	ribi_t::ribi alte_richtung;
	
	typedef struct {
		bool valid;
		signal_t* sig;
		uint16 next_block;
	} longblock_signal_request_t;
	longblock_signal_request_t longblock_signal_request;

	/**
	* Initialize all variables with default values.
	* Each constructor must call this method first!
	* @author Hj. Malthaner
	*/
	void init(player_t *player);

	/**
	* Calculate route from Start to Target Coordinate
	* @author Hanjsörg Malthaner
	*/
	bool drive_to();

	/**
	* Setup vehicles for moving in same direction than before
	* if the direction is the same as before
	* @author Hanjsörg Malthaner
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
	* @author Hanjsörg Malthaner
	*/
	void set_erstes_letztes();

	// returns the index of the vehikel at position length (16=1 tile)
	int get_vehicle_at_length(uint16);

	/**
	* calculate income for last hop
	* only used for entering depot or recalculating routes when a schedule window is opened
	* @author Hj. Malthaner
	*/
	void calc_gewinn();

	/**
	* Recalculates loading level and limit.
	* While driving loading_limit will be set to 0.
	* @author Volker Meyer
	* @date  20.06.2003
	*/
	void calc_loading();

	/* Calculates (and sets) akt_speed
	 * needed for driving, entering and leaving a depot)
	 */
	void calc_acceleration(uint32 delta_t);

	/*
	* struct holds new financial history for convoi
	* @author hsiegeln
	*/
	sint64 financial_history[MAX_MONTHS][MAX_CONVOI_COST];

	/**
	* initialize the financial history
	* @author hsiegeln
	*/
	void init_financial_history();

	/**
	* the koordinate of the home depot of this convoi
	* the last depot visited is considered being the home depot
	* @author hsiegeln
	*/
	koord3d home_depot;

	/**
	* unset line -> remove cnv from line
	* @author hsiegeln
	*/
	void unset_line();

	// matches two halts; if the pos is not identical, maybe the halt still is
	bool matches_halt( const koord3d pos1, const koord3d pos2 );

	/**
	 * Register the convoy with the stops in the schedule
	 * @author Knightly
	 */
	void register_stops();

	/**
	 * Unregister the convoy from the stops in the schedule
	 * @author Knightly
	 */
	void unregister_stops();

	uint32 move_to(uint16 start_index);

	/**
	 * the route index of the point to quit yielding lane
	 * == -1 means this convoi isn't yielding.
	 * @author teamhimeH
	 */
	sint32 yielding_quit_index;

	// 0: not fixed, -1: fixed to traffic lane, 1: fixed to passing lane
	sint8 lane_affinity;
	uint32 lane_affinity_end_index;

	// true, if this vehicle will cross lane and block other vehicles.
	bool next_cross_lane;
	// When this convoy requested lane crossing...
	uint32 request_cross_ticks;

public:
	/**
	* Convoi haelt an Haltestelle und setzt quote fuer Fracht
	* @author Hj. Malthaner
	*/
	void hat_gehalten(halthandle_t halt);

	const route_t* get_route() const { return &route; }
	route_t* access_route() { return &route; }

	const koord3d get_schedule_target() const { return schedule_target; }
	void set_schedule_target( koord3d t ) { schedule_target = t; }

	/**
	* get line
	* @author hsiegeln
	*/
	linehandle_t get_line() const {return line;}

	/* true, if electrification needed for this convoi */
	bool needs_electrification() const { return is_electric; }

	/**
	* set line
	* @author hsiegeln
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
	* @author hsiegeln
	*/
	int get_state() const { return state; }

	/**
	* true if in waiting state (maybe also due to starting)
	* @author hsiegeln
	*/
	bool is_waiting() { return (state>=WAITING_FOR_CLEARANCE  &&  state<=CAN_START_TWO_MONTHS)  &&  state!=SELF_DESTRUCT; }

	/**
	* reset state to no error message
	* @author prissi
	*/
	void reset_waiting() { state=WAITING_FOR_CLEARANCE; }

	/**
	* The handle for ourselves. In Anlehnung an 'this' aber mit
	* allen checks beim Zugriff.
	* @author Hanjsörg Malthaner
	*/
	convoihandle_t self;

	/**
	 * The profit in this year
	 * @author Hanjsörg Malthaner
	 */
	const sint64 & get_jahresgewinn() const {return jahresgewinn;}

	const sint64 & get_total_distance_traveled() const { return total_distance_traveled; }

	/**
	 * @return the total monthly fix cost for all vehicles in convoi
	 */
	sint32 get_fix_cost() const;

	/**
	 * returns the total running cost for all vehicles in convoi
	 * @author hsiegeln
	 */
	sint32 get_running_cost() const;

	/**
	 * returns the total new purchase cost for all vehicles in convoy
	 */
	sint64 get_purchase_cost() const;

	/**
	* Constructor for loading from file,
	* @author Hj. Malthaner
	*/
	convoi_t(loadsave_t *file);

	convoi_t(player_t* player_);

	virtual ~convoi_t();

	/**
	* Load or save this convoi data
	* @author Hj. Malthaner
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
	* @author Hanjsörg Malthaner
	*/
	void betrete_depot(depot_t *dep);

	/**
	* Return the internal name of the convois
	* @return Name of the convois
	* @author Hj. Malthaner
	*/
	const char *get_internal_name() const {return name_and_id+name_offset;}

	/**
	* Allows editing ...
	* @return Name of the Convois
	* @author Hj. Malthaner
	*/
	char *access_internal_name() {return name_and_id+name_offset;}

	/**
	* Return the name of the convois
	* @return Name of the convois
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return name_and_id;}

	/**
	* Sets the name. Copies name into this->name and translates it.
	* @author V. Meyer
	*/
	void set_name(const char *name, bool with_new_id = true);

	/**
	 * Return the position of the convois.
	 * @return Position of the convois
	 * @author Hj. Malthaner
	 */
	koord3d get_pos() const;

	/**
	 * @return current speed, this might be different from topspeed
	 *         actual currently set speed.
	 * @author Hj. Malthaner
	 */
	const sint32& get_akt_speed() const { return akt_speed; }

	/**
	 * @return total power of this convoi
	 * @author Hj. Malthaner
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
	 * @author neroden
	 */
	uint32 get_length_in_steps() const { return get_length() * VEHICLE_STEPS_PER_CARUNIT; }

	/**
	 * Add the costs for travelling one tile
	 * @author Hj. Malthaner
	 */
	void add_running_cost( const weg_t *weg );

	/**
	 * moving the vehicles of a convoi and acceleration/deceleration
	 * all other stuff => convoi_t::step()
	 * @author Hj. Malthaner
	 */
	sync_result sync_step(uint32 delta_t);

	/**
	 * All things like route search or loading, that may take a little
	 * @author Hj. Malthaner
	 */
	void step();

	/**
	* sets a new convoi in route
	* @author Hj. Malthaner
	*/
	void start();

	void ziel_erreicht(); ///< Called, when the first vehicle reaches the target

	/**
	* When a vehicle has detected a problem
	* force calculate a new route
	* @author Hanjsörg Malthaner
	*/
	void suche_neue_route();

	/**
	* Wait until vehicle 0 reports free route
	* will be called during a hop_check, if the road/track is blocked
	* @author Hj. Malthaner
	*/
	void warten_bis_weg_frei(sint32 restart_speed);

	/**
	* @return Vehicle count
	* @author Hj. Malthaner
	*/
	uint8 get_vehicle_count() const { return anz_vehikel; }

	/**
	 * @return Vehicle at position i
	 */
	vehicle_t* get_vehikel(uint16 i) const { return fahr[i]; }

	vehicle_t* front() const { return fahr[0]; }

	vehicle_t* back() const { return fahr[anz_vehikel - 1]; }

	/**
	* Adds a vehicle at the start or end of the convoi.
	* @author Hj. Malthaner
	*/
	bool add_vehikel(vehicle_t *v, bool infront = false);

	/**
	* Removes vehicles at position i
	* @author Hj. Malthaner
	*/
	vehicle_t * remove_vehikel_bei(unsigned short i);

	const minivec_tpl<uint8> &get_goods_catg_index() const { return goods_catg_index; }

	// recalculates the good transported by this convoy and (in case of changes) will start schedule recalculation
	void recalc_catg_index();

	/**
	* Sets a schedule
	* @author Hj. Malthaner
	*/
	bool set_schedule(schedule_t *f);

	/**
	* @return Current schedule
	* @author Hj. Malthaner
	*/
	schedule_t* get_schedule() const { return schedule; }

	/**
	* Creates a new schedule if there isn't one already.
	* @return Current schedule
	* @author Hj. Malthaner
	*/
	schedule_t * create_schedule();

	// remove wrong freight when schedule changes etc.
	void check_freight();

	/**
	* @return Owner of this convoi
	* @author Hj. Malthaner
	*/
	player_t * get_owner() const { return owner_p; }

	/**
	* Opens an information window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void open_info_window();

	/**
	* @return a description string for the object, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	* @see simwin
	*/
	void info(cbuffer_t & buf) const;

	/**
	* @param buf the buffer to fill
	* @return Freight description text (buf)
	* @author Hj. Malthaner
	*/
	void get_freight_info(cbuffer_t & buf);
	void set_sortby(uint8 order);
	uint8 get_sortby() const { return freight_info_order; }

	/**
	* Opens the schedule window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void open_schedule_window( bool show );

	/**
	* pruefe ob Beschraenkungen fuer alle Fahrzeuge erfuellt sind
	* @author Hj. Malthaner
	*/
	bool pruefe_alle();

	/**
	* Control loading and unloading
	* V.Meyer: returns nothing
	* @author Hj. Malthaner
	*/
	void laden();

	/**
	* Setup vehicles before starting to move
	* @author Hanjsörg Malthaner
	*/
	void vorfahren();

	/**
	* Calculate the total value of the convoi as the sum of all vehicle values.
	* @author Volker Meyer
	* @date  09.06.2003
	*/
	sint64 calc_restwert() const;

	/**
	* Check if this convoi has entered a depot.
	* @author Volker Meyer
	* @date  09.06.2003
	*/
	bool in_depot() const { return state == INITIAL; }

	/**
	* loading_level was minimum_loading before. Actual percentage loaded of loadable
	* vehicles.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	const sint32 &get_loading_level() const { return loading_level; }

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	const sint32 &get_loading_limit() const { return loading_limit; }

	/**
	* Schedule convois for self destruction. Will be executed
	* upon next sync step
	* @author Hj. Malthaner
	*/
	void self_destruct();

	/**
	* Helper method to remove convois from the map that cannot
	* removed normally (i.e. by sending to a depot) anymore.
	* This is a workaround for bugs in the game.
	* @author Hj. Malthaner
	* @date  12-Jul-03
	*/
	void destroy();

	/**
	* Debug info to stderr
	* @author Hj. Malthaner
	* @date 04-Sep-03
	*/
	void dump() const;

	/**
	* book a certain amount into the convois financial history
	* is called from vehicle during un/load
	* @author hsiegeln
	*/
	void book(sint64 amount, int cost_type);

	/**
	* return a pointer to the financial history
	* @author hsiegeln
	*/
	sint64* get_finance_history() { return *financial_history; }

	/**
	* return a specified element from the financial history
	* @author hsiegeln
	*/
	sint64 get_finance_history(int month, int cost_type) const { return financial_history[month][cost_type]; }
	sint64 get_stat_converted(int month, int cost_type) const;

	/**
	* only purpose currently is to roll financial history
	* @author hsiegeln
	*/
	void new_month();

	/**
	 * Method for yearly action
	 * @author Hj. Malthaner
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
	 * @author prissi
	 */
	uint16 get_next_stop_index() const {return next_stop_index;}
	void set_next_stop_index(uint16 n);

	/* including this route_index, the route was reserved the last time
	 * currently only used for tracks
	 */
	uint16 get_next_reservation_index() const {return next_reservation_index;}
	void set_next_reservation_index(uint16 n);
	
	/* these functions modify only reserved_tiles.
	 * reservation of tiles has to be done separately.
	 * @author THLeaderH
	 */
	void unreserve_pos(koord3d pos) { reserved_tiles.remove(pos); }
	void reserve_pos(koord3d pos) {reserved_tiles.append_unique(pos); }
	bool is_reservation_empty() const { return reserved_tiles.empty(); }
	vector_tpl<koord3d>& get_reserved_tiles() { return reserved_tiles; }

	/* the current state of the convoi */
	PIXVAL get_status_color() const;

	// returns tiles needed for this convoi
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
	virtual bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other);

	/*
	 * Functions related to requested_change_lane
	 * @author teamhimeH
	 */
	bool is_requested_change_lane() const { return requested_change_lane; }
	void set_requested_change_lane(bool x) { requested_change_lane = x; }
	void yield_lane_space();
	sint32 get_yielding_quit_index() const { return yielding_quit_index; }
	void quit_yielding_lane() { yielding_quit_index = -1; must_recalc_speed_limit(); }

	/*
	 * Functions related to lane fixing
	 * @author teamhimeH
	 */
	 bool calc_lane_affinity(uint8 lane_affinity_sign); // If true, lane fixing started.
	 uint32 get_lane_affinity_end_index() const { return lane_affinity_end_index; }
	 sint8 get_lane_affinity() const { return lane_affinity; }
	 void reset_lane_affinity() { lane_affinity = 0; }

	 bool get_next_cross_lane();
	 void set_next_cross_lane(bool);

	 virtual void refresh(sint8,sint8) OVERRIDE;
	 
	 void request_longblock_signal_judge(signal_t *sig, uint16 next_block);
	 void set_longblock_signal_judge_request_invalid() { longblock_signal_request.valid = false; };
};

#endif
