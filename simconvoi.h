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
#include "tpl/fixed_list_tpl.h"
#include "tpl/koordhashtable_tpl.h"
#include "tpl/inthashtable_tpl.h"
#include "tpl/minivec_tpl.h"

#include "convoihandle_t.h"
#include "halthandle_t.h"

#include "simconst.h"

#include "simdings.h"
#include "convoy.h"

/*
 * Waiting time for infinite loading (ms)
 * @author Hj- Malthaner
 */
#define WAIT_INFINITE 9223372036854775807ll
#define MAX_MONTHS               12 // Max history

class weg_t;
class depot_t;
class karte_t;
class spieler_t;
class vehikel_t;
class vehikel_besch_t;
class schedule_t;
class cbuffer_t;
class ware_t;
class replace_data_t;

/**
 * Base class for all vehicle consists. Convoys can be referenced by handles, see halthandle_t.
 *
 * @author Hj. Malthaner
 */
class convoi_t : public sync_steppable, public overtaker_t, public lazy_convoy_t
{
public:
	enum {
		CONVOI_CAPACITY =			0, // the amount of ware that could be transported, theoretically	
		CONVOI_TRANSPORTED_GOODS =	1, // the amount of ware that has been transported
		CONVOI_AVERAGE_SPEED =		2, // The average speed of the convoy per rolling month
		CONVOI_COMFORT =			3, // The aggregate comfort rating of this convoy
		CONVOI_REVENUE =			4, // the income this CONVOI generated
		CONVOI_OPERATIONS =			5, // the cost of operations this CONVOI generated
		CONVOI_PROFIT =				6, // total profit of this convoi
		CONVOI_DISTANCE =			7, // total distance traveled this month
		CONVOI_REFUNDS =			8, // The refunds passengers waiting for this convoy (only when not attached to a line) have received.
		MAX_CONVOI_COST =			9
	};

	/* Konstanten
	* @author prissi
	*/
	enum { max_vehicle=8, max_rail_vehicle = 64 };

	enum states {INITIAL,
		FAHRPLANEINGABE,
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
		REVERSING,
		MAX_STATES
	};

	/**
	* time, when a convoi waiting for full load will drive on
	* @author prissi
	*/
	sint64 go_on_ticks;

	struct departure_data_t
	{
	public:
		/** 
		  * Departure time in internal ticks
		  */
		sint64 departure_time;

		/**
		* Accumulated distance since the convoy departed from
		* this stop, indexed by the player number of the way 
		* over which the convoy has passed. If the way is
		* ownerless, it is recorded as belonging to the owner
		* of they convoy, unless it is open water, in which case
		* it is recorded as being MAX_PLAYER_COUNT (in other 
		* words, two greater than the maximum number of players).
		* This is to facilitate proper apportionment of revenues
		* for ocean-going ships coming into ports owned by
		* other players (+2) and the measurement of journey
		* distance by straight line distance between halts,
		* rather than route distance (+1)
		*/
	private:
		uint32 accumulated_distance_since_departure[MAX_PLAYER_COUNT + 2];

	public:

		departure_data_t()
		{
			departure_time = 0ll;
			reset_distances();
		}

		/**
		 * Method for adding the total distance at intermediate halts
		 */
		void add_overall_distance(uint32 distance)
		{
			accumulated_distance_since_departure[MAX_PLAYER_COUNT] += distance;
		}

		/** 
		 * Method to get the overall distance. This should be the basis
		 * for measuring the total revenue.
		 */
		uint32 get_overall_distance() const
		{
			return accumulated_distance_since_departure[MAX_PLAYER_COUNT];
		}

		uint32 get_way_distance(uint8 index) const
		{
			return accumulated_distance_since_departure[index];
		}


		/** 
		 * Method to increment by one the distance recorded as
		 * travelled by a vehicle over a particular way, indexed
		 * by the player ID of the way. This is used for revenue
		 * apportionment.
		 */
		void increment_way_distance(uint8 player, uint32 steps)
		{
			accumulated_distance_since_departure[player] += steps;
		}

		/**
		 * Method for setting values in the array ab initio. 
		 * Used when loading from a saved game only.
		 */
		void set_distance(uint8 index, uint32 value)
		{
			accumulated_distance_since_departure[index] = value;
		}

		/**
		 * Method for resetting the value of the overall distance
		 * Used in circular routes when the convoy reaches a
		 * halt from which it has previously departed, to
		 * prevent over-accumulation of distance.
		 */
		void reset_distances()
		{
			for(int i = 0; i < MAX_PLAYER_COUNT + 1; i ++)
			{
				accumulated_distance_since_departure[i] = 0;
			}
		}
	};

// BG, 31.12.2012: virtual methods of lazy_convoy_t:
private:
	weight_summary_t weight;
protected:
	virtual void update_vehicle_summary(vehicle_summary_t &vehicle);
	virtual void update_adverse_summary(adverse_summary_t &adverse);
	virtual void update_freight_summary(freight_summary_t &freight);
	virtual void update_weight_summary(weight_summary_t &weight);
	virtual float32e8_t get_brake_summary(/*const float32e8_t &speed*/ /* in m/s */);
	virtual float32e8_t get_force_summary(const float32e8_t &speed /* in m/s */);
	virtual float32e8_t get_power_summary(const float32e8_t &speed /* in m/s */);
public:
	virtual sint16 get_current_friction();
	
	// weight_summary becomes invalid, when vehicle_summary or envirion_summary 
	// becomes invalid.
	inline void invalidate_weight_summary()
	{
		is_valid &= ~cd_weight_summary;
	}

	// weight_summary is valid if (is_valid & cd_weight_summary != 0)
	inline void validate_weight_summary() {
		if (!(is_valid & cd_weight_summary)) 
		{
			is_valid |= cd_weight_summary;
			update_weight_summary(weight);
		}
	}

	// weight_summary needs recaching only, if it is going to be used. 
	inline const weight_summary_t &get_weight_summary() {
		validate_weight_summary();
		return weight;
	}

	inline void calc_move(const settings_t &settings, long delta_t, sint32 akt_speed_soll, sint32 next_speed_limit, sint32 steps_til_limit, sint32 steps_til_brake, sint32 &akt_speed, sint32 &sp_soll, float32e8_t &akt_v)
	{
		validate_weight_summary();
		convoy_t::calc_move(settings, delta_t, weight, akt_speed_soll, next_speed_limit, steps_til_limit, steps_til_brake, akt_speed, sp_soll, akt_v);
	}
// BG, 31.12.2012: end of virtual methods of lazy_convoy_t.

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
	* holds id of line with pendig update
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
	* Alle vehikel-fahrplanzeiger zeigen hierauf
	* @author Hj. Malthaner
	*/
	schedule_t *fpl;

	// Added by : Knightly
	// Purpose  : To hold the original schedule before opening schedule window
	schedule_t *old_fpl;

	/**
	* loading_level was ladegrad before. Actual percentage loaded for loadable vehicles (station length!).
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
	 * Free seats for passengers, calculated in convoi_t::calc_loading
	 * @author Inkelyad
	 */
	sint32 free_seats;

	/**
	* The vehicles of this convoi
	*
	* @author Hj. Malthaner
	*/
	array_tpl<vehikel_t*> fahr;

	/*
	 * a list of all catg_index, which can be transported by this convoy.
	 */
	minivec_tpl<uint8> goods_catg_index;

	/**
	* Convoi owner
	* @author Hj. Malthaner
	*/
	spieler_t *besitzer_p;

	/**
	* Current map
	* @author Hj. Malthaner
	*/

	static karte_t *welt;

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
	* If the convoy is marked for automatic replacing,
	* this will point to the dataset for it; otherwise,
	* it is NULL.
	* @author: jamespetts, March 2010
	*/
	replace_data_t *replace;

	/**
	* send to depot when empty
	* @author isidoro
	*/
	bool depot_when_empty;

	/**
	* the convoi traverses its schedule in reverse order
	* @author yobbobandana
	*/
	bool reverse_schedule;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	* @author prissi
	*/
	bool freight_info_resort;

	// true, if at least one vehicle of a convoi is obsolete
	bool has_obsolete;

	// ture, if there is at least one engine that requires catenary
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

	/**
	* Gesamtleistung. Wird nicht gespeichert, sondern aus den Einzelleistungen
	* errechnet.
	* @author Hj. Malthaner
	*/
	//uint32 sum_leistung;

	/**
	* Gesamtleistung mit Gear. Wird nicht gespeichert, sondern aus den Einzelleistungen
	* errechnet.
	* @author prissi
	*/
	//sint32 sum_gear_und_leistung;

	/* sum_gewicht: leergewichte aller vehicles *
	* sum_gesamtgewicht: gesamtgewichte aller vehicles *
	* Werden nicht gespeichert, sondern aus den Einzelgewichten
	* errechnet beim beladen/fahren.
	* @author Hj. Malthaner, prissi
	*/
	//sint64 sum_gewicht;
	//sint64 sum_gesamtgewicht;

	// cached values
	// will be recalculated if
	// recalc_data is true
	bool recalc_data_front; // true when front vehicle in convoi hops
	//bool recalc_data; // true when any vehicle in convoi hops

	//sint64 sum_friction_weight;
	//sint32 speed_limit;

	/**
	* Lowest top speed of all vehicles. Doesn't get saved, but calculated
	* from the vehicles data
	* @author Hj. Malthaner
	*/
	//sint32 min_top_speed;

	/**
	 * this give the index of the next signal or the end of the route
	 * convois will slow down before it, if this is not a waypoint or the cannot pass
	 * The slowdown ist done by the vehicle routines
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
	 * The coinvoi is not processed every sync step for various actions
	 * (like waiting before signals, loading etc.) Such action will only
	 * continue after a waiting time larger than wait_lock
	 * @author Hanjsörg Malthaner
	 */
	sint32 wait_lock;

	/**
	* akkumulierter gewinn über ein jahr hinweg
	* @author Hanjsörg Malthaner
	*/
	sint64 jahresgewinn;

	/* the odometer */
	// (In *km*).
	sint64 total_distance_traveled;

	// The number of steps travelled since
	// the odometer was last incremented.
	// Used for converting tiles to km.
	// @author: jamespetts
	sint64 steps_since_last_odometer_increment;

	/**
	* Set, when there was a income calculation (avoids some cheats)
	* Since 99.15 it will stored directly in the vehikel_t
	* @author prissi
	*/
	koord3d last_stop_pos;

	/**
	* Necessary for registering departure and waiting times.
	* last_stop_pos cannot be used because sea-going ships do not
	* stop on a halt tile.
	*/
	uint16 last_stop_id;

	// things for the world record
	sint32 max_record_speed; // current convois fastest speed ever
	koord record_pos;

	// needed for speed control/calculation
	sint32 akt_speed;	        // current speed
	sint32 akt_speed_soll;		// Target speed
	sint32 sp_soll;				// steps to go
	sint32 previous_delta_v;	// Stores the previous delta_v value; otherwise these digits are lost during calculation and vehicle do not accelrate
	float32e8_t v; // current speed in m/s.

	uint32 next_wolke;	// time to next smoke

	states state;

	ribi_t::ribi alte_richtung; //"Old direction" (Google)

	/**
	* The index number of the livery scheme of the current convoy
	* in the livery schemes vector in einstellungen_t.
	* @author: jamespetts, April 2011
	*/
	uint16 livery_scheme_index;

	/**
	* Initialize all variables with default values.
	* Each constructor must call this method first!
	* @author Hj. Malthaner
	*/
	void init(karte_t *welt, spieler_t *sp);

	/**
	* Berechne route von Start- zu Zielkoordinate
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

	// reseverse route until next_reservation_index
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
	//void calc_gewinn();

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
	void calc_acceleration(long delta_t);

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
	* the last depot visited is considered beeing the home depot
	* @author hsiegeln
	*/
	koord3d home_depot;

	// Helper function: used in init and replacing
	void reset();

	// Helper function: used in enter_depot and destructor.
	void close_windows();

	// Reverses the order of the convoy.
	// @author: jamespetts
	void reverse_order(bool rev);
	bool reversable;
	bool reversed;

	uint32 highest_axle_load;
	uint32 longest_min_loading_time;
	uint32 longest_max_loading_time;
	uint32 current_loading_time;

	/**
	 * Time in ticks since this convoy last departed from
	 * any given stop, plus accumulated distance since the last
	 * stop, indexed here by its handle ID.
	 * @author: jamespetts, August 2011. Replaces the original
	 * "last_departure_time" member.
	 * Modified October 2011 to include accumulated distance.
	 */
	typedef inthashtable_tpl<uint16, departure_data_t> departure_map;
	departure_map *departures;

	// When we arrived at current stop
	// @author Inkelyad
	sint64 arrival_time;

	//When convoy was at first stop.
	//Used in average round trip time calculations.
	fixed_list_tpl<sint64, MAX_CONVOI_COST> arrival_to_first_stop;
	// @author: jamespetts
	uint32 rolling_average[MAX_CONVOI_COST];
	uint16 rolling_average_count[MAX_CONVOI_COST];

	/**To prevent repeat bookings of journey time
	 * and comfort when a vehicle is waiting for
	 * load at a stop.
	 */
	uint8 current_stop;

	/**
	 * The number of times that this
	 * convoy has attempted to find
	 * a route to its destination
	 * and failed.
	 * @author: jamespetts, November 2011
	 */
	uint8 no_route_retry_count;

	/**
	 * Get obsolescence from vehicle list.
	 * Extracted from new_month().
	 * Used to recalculate convoi_t::has_obsolete
	 * 
	 * @author: Bernd Gabriel
	 */
	bool calc_obsolescence(uint16 timeline_year_month);

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

	uint32 move_to(karte_t const&, koord3d const& k, uint16 start_index);

	/**
	* Advance the schedule cursor.
	* Also toggles the reverse_schedule flag if necessary.
	* @author yobbobandana
	*/
	void advance_schedule();

	/**
	 * Measure and record the times that
	 * goods of all types have been waiting
	 * before boarding this convoy
	 * @author: jamespetts
	 */
	void book_waiting_times();

	/**
	 * Measure and record the departure
	 * time of this convoy from the current
	 * stop (if the convoy is currently at
	 * a stop)
	 * @author: jamespetts
	 */
	void book_departure_time(sint64 time);

	/**
	 * Whether this convoy is in the process
	 * of trying to reserve a path from a 
	 * choose signal. Only relevant for rail
	 * convoys, as this is for the block 
	 * reserver.
	 * @author: jamespetts
	 */
	bool is_choosing:1; 

public: 
	/**
	 * Some precalculated often used infos about a tile of the convoy's route.
	 * @author B. Gabriel
	 */
	class route_info_t
	{
	public:
		sint32 speed_limit;
		uint32 steps_from_start; // steps including this tile's length, which is VEHICLE_STEPS_PER_TILE for a straight and diagonal_vehicle_steps_per_tile for a diagonal way.
		ribi_t::ribi direction;
	};

	class route_infos_t : public vector_tpl<route_info_t>
	{
		// BG, 05.08.2012: pay attention to the aircrafts' holding pattern!
		// It starts at route_index = touchdown - HOLDING_PATTERN_LENGTH - HOLDING_PATTERN_OFFSET
		// and has a length of HOLDING_PATTERN_LENGTH.
		sint32 hp_start_index; // -1: not an aircraft or aircraft has passed the start of the holding pattern.
		sint32 hp_end_index;   // -1: not an aircraft or aircraft has passed the start of the holding pattern.
		sint32 hp_start_step;  // -1: not an aircraft or aircraft has passed the start of the holding pattern.
		sint32 hp_end_step;   // -1: not an aircraft or aircraft has passed the start of the holding pattern.
	public:
		void set_holding_pattern_indexes(sint32 current_route_index, sint32 touchdown_route_index);

		inline sint32 get_holding_pattern_start_index() const { return hp_start_index; }
		inline sint32 get_holding_pattern_end_index()   const { return hp_end_index; }
		inline sint32 get_holding_pattern_start_step()  const { return hp_start_step; }
		inline sint32 get_holding_pattern_end_step()    const { return hp_end_step; }

		// BG 05.08.2012: calc number of steps. Ignores the holding pattern, if start_step starts before it and end_step ends after it.
		// In any other case the holding pattern is part off the route or is irrelevant.
		inline sint32 calc_steps(sint32 start_step, sint32 end_step)
		{
			if (hp_start_step >= 0 && start_step < hp_start_step && hp_end_step <= end_step)
			{
				// assume they will not send us into the holding pattern: skip it.
				return (end_step - start_step) - (hp_end_step - hp_start_step);
			}
			return end_step - start_step;
		}

		// BG 05.08.2012: calc number of tiles. Ignores the holding pattern, if start_index starts before it and end_index ends after it.
		// In any other case the holding pattern is part off the route or is irrelevant.
		inline sint32 calc_tiles(sint32 start_index, sint32 end_index)
		{
			if (hp_start_index >= 0 && start_index < hp_start_index && hp_end_index <= end_index)
			{
				// assume they will not send us into the holding pattern: skip it.
				return (end_index - start_index) - (hp_end_index - hp_start_index);
			}
			return end_index - start_index;
		}
	};
#ifdef DEBUG_PHYSICS
	sint32 next_speed_limit; 
	sint32 steps_til_limit;
	sint32 steps_til_brake;
#endif

private:
	/** 
	  * List of upcoming speed limits; for braking purposes.
	  * @author: jamespetts, September 2011
	  */
	route_infos_t route_infos;

public:
	ding_t::typ get_depot_type() const;

	/**
	* Convoi haelt an Haltestelle und setzt quote fuer Fracht
	* @author Hj. Malthaner
	*/
	void hat_gehalten(halthandle_t halt);

	route_t* get_route() { return &route; }
	route_t* access_route() { return &route; }
	bool calc_route(koord3d start, koord3d ziel, sint32 max_speed);
	void update_route(uint32 index, const route_t &replacement); // replace route with replacement starting at index.

	/**
	* get line
	* @author hsiegeln
	*/
	inline linehandle_t get_line() const {return line;}

	/* true, if electrification needed for this convoi */
	inline bool needs_electrification() const { return is_electric; }

	/**
	* set line
	* @author hsiegeln
	*/
	void set_line(linehandle_t );

	// updates a line schedule and tries to find the best next station to go
	void check_pending_updates();

	/* changes the state of a convoi via werkzeug_t; mandatory for networkmode! *
	 * for list of commands and parameter see werkzeug_t::wkz_change_convoi_t
	 */
	void call_convoi_tool( const char function, const char *extra = NULL );

	/**
	* set state: only use by werkzeug_t convoi tool, or not networking!
	* @author hsiegeln
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
	inline void reset_waiting() { state=WAITING_FOR_CLEARANCE; }

	/**
	* Das Handle für uns selbst. In Anlehnung an 'this' aber mit
	* allen checks beim Zugriff.
	* @author Hanjsörg Malthaner
	*/
	convoihandle_t self;

	/*
	 * "The profit in this year" (Babelfish)
	 * @author Hanjsörg Malthaner
	 */
	inline const sint64 & get_jahresgewinn() const {return jahresgewinn;}

	const sint64 & get_total_distance_traveled() const { return total_distance_traveled; }

	/**
	 * returns the total running cost for all vehicles in convoi
	 * @author hsiegeln
	 */
	sint32 get_running_cost() const;

	// Gets the running cost per kilometre
	// for GUI purposes.
	// @author: jamespetts
	sint32 get_per_kilometre_running_cost() const;

	/**
	* returns the total monthly fixed maintenance cost for all vehicles in convoi
	* @author Bernd Gabriel
	*/
	uint32 get_fixed_cost() const;

	/**
	 * returns the total new purchase cost for all vehicles in convoy
	 */
	sint64 get_purchase_cost() const;

	/**
	* Constructor for loading from file,
	* @author Hj. Malthaner
	*/
	convoi_t(karte_t *welt, loadsave_t *file);

	convoi_t(spieler_t* sp);

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

	void laden_abschliessen();

	void rotate90( const sint16 y_size );

	/**
	* Called to make a convoi enter a depot
	* @author Hj. Malthaner, neroden
	*/
	void enter_depot(depot_t *dep);

	/**
	* @return Current map.
	* @author Hj. Malthaner
	*/
	karte_t* get_welt() { return welt; }

	/**
	* Gibt Namen des Convois zurück.
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	inline const char *get_internal_name() const {return name_and_id+name_offset;}

	/**
	* Allows editing ...
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	inline char *access_internal_name() {return name_and_id+name_offset;}

	/**
	* Gibt Namen des Convois zurück.
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	inline const char *get_name() const {return name_and_id;}

	/**
	* Sets the name. Copies name into this->name and translates it.
	* @author V. Meyer
	*/
	void set_name(const char *name, bool with_new_id = true);

	/**
	 * Gibt die Position des Convois zurück.
	 * @return Position des Convois
	 * @author Hj. Malthaner
	 */
	koord3d get_pos() const;

	/**
	 * @return current speed, this might be different from topspeed
	 *         actual currently set speed.
	 * @author Hj. Malthaner
	 */
	inline sint32 get_akt_speed() const { return akt_speed; }
	inline sint32 get_akt_speed_soll() const { return akt_speed_soll; }

	/**
	 * @return total power of this convoi
	 * @author Hj. Malthaner
	 */
	inline uint32 get_sum_leistung() {return get_continuous_power();}
	inline sint32 get_min_top_speed() {return get_vehicle_summary().max_sim_speed;}

	/// @returns weight of the convoy's vehicles (excluding freight)
	inline sint64 get_sum_gewicht() {return get_vehicle_summary().weight;}

	/// @returns weight of convoy including freight
	//inline const sint64 & get_sum_gesamtgewicht() const {return sum_gesamtgewicht;}

	/** Get power index in kW multiplied by gear.
	 * Get effective power in kW by dividing by GEAR_FACTOR, which is 64.
	 * @author Bernd Gabriel, Nov, 14 2009
	 */
	//inline const sint32 & get_power_index() { return sum_gear_und_leistung; }

	uint32 get_length() const;

	/**
	 * @return length of convoi in the correct units for movement
	 * @author neroden
	 */
	uint32 get_length_in_steps() const { return get_length() * VEHICLE_STEPS_PER_CARUNIT; }

	/**
	 * Add the costs for traveling one tile
	 * @author Hj. Malthaner
	 */
	void add_running_cost(sint64 cost, const weg_t *weg);

	// Increment the odometer,
	// adjusting for the distance scale.
	void increment_odometer(uint32 steps);

	/**
	 * moving the veicles of a convoi and acceleration/deacceleration
	 * all other stuff => convoi_t::step()
	 * @author Hj. Malthaner
	 */
	bool sync_step(long delta_t);

	/**
	 * All things like route search or laoding, that may take a little
	 * @author Hj. Malthaner
	 */
	void step();

	/**
	* setzt einen neuen convoi in fahrt
	* @author Hj. Malthaner
	*/
	void start();

	void ziel_erreicht(); ///< Called, when the first vehicle reaches the target

	/**
	* Ein Fahrzeug hat ein Problem erkannt und erzwingt die
	* Berechnung einer neuen Route
	* @author Hanjsörg Malthaner
	*/
	void suche_neue_route();

	/**
	* Wartet bis Fahrzeug 0 freie Fahrt meldet
	* will be called during a hop_check, if the road/track is blocked
	* @author Hj. Malthaner
	*/
	void warten_bis_weg_frei(int restart_speed);

	/**
	* @return Vehicle count
	* @author Hj. Malthaner
	*/
	inline uint8 get_vehikel_anzahl() const { return anz_vehikel; }

	/**
	 * @return Vehicle at position i
	 */
	inline vehikel_t* get_vehikel(uint16 i) const { return fahr[i]; }

	// Upgrades a vehicle in the convoy.
	// @author: jamespetts, February 2010
	void upgrade_vehicle(uint16 i, vehikel_t* v);

	vehikel_t* front() const { return fahr[0]; }

	vehikel_t* back() const { return fahr[anz_vehikel - 1]; }

	/**
	* Adds a vehicel at the start or end of the convoi.
	* @author Hj. Malthaner
	*/
	bool add_vehikel(vehikel_t *v, bool infront = false);

	/**
	* Removes vehicles at position i
	* @author Hj. Malthaner
	*/
	vehikel_t * remove_vehikel_bei(unsigned short i);

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
	inline schedule_t* get_schedule() const { return fpl; }

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

	spieler_t * get_besitzer() { return besitzer_p; }

	/**
	* Opens an information window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void zeige_info();

	/**
	* Get whether the convoi is traversing its schedule in reverse.
	* @author yobbobandana
	*/
	inline bool get_reverse_schedule() const { return reverse_schedule; }

	/**
	* Set whether the convoi is traversing its schedule in reverse.
	* @author yobbobandana
	*/
	void set_reverse_schedule(bool reverse) { reverse_schedule = reverse; }

	void set_is_choosing(bool value) { is_choosing = value; }
	bool get_is_choosing() const { return is_choosing; }

	/**
	* The table of point-to-point average journey times.
	* @author jamespetts
	*/
	typedef koordhashtable_tpl<id_pair, average_tpl<uint16> > journey_times_map;
private:
		journey_times_map *average_journey_times;
public:

#if 0
private:
	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	* @see simwin
	*/
	void info(cbuffer_t & buf, bool dummy = false) const;
public:
#endif
	/**
	* @param buf the buffer to fill
	* @return Freight dscription text (buf)
	* @author Hj. Malthaner
	*/
	void get_freight_info(cbuffer_t & buf);
	void set_sortby(uint8 order);
	inline uint8 get_sortby() const { return freight_info_order; }

	/**
	* Opens the schedule window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void open_schedule_window( bool show );

	/**
	* pruefe ob Beschraenkungen fuer alle Fahrzeuge erfuellt sind
	* "	examine whether restrictions for all vehicles are fulfilled" (Google)
	* @author Hj. Malthaner
	*/
	bool pruefe_alle();

	/**
	* Kontrolliert Be- und Entladen.
	* "Controlled loading and unloading" (Google)
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
	inline bool in_depot() const { return state == INITIAL; }

	/**
	* loading_level was ladegrad before. Actual percentage loaded of loadable
	* vehicles.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	inline const sint32 &get_loading_level() const { return loading_level; }

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	inline const sint32 &get_loading_limit() const { return loading_limit; }

	/**
	 * Format remaining loading time from go_on_ticks
	 */
	void snprintf_remaining_loading_time(char *p, size_t size) const;

	/**
	 * Format remaining reversing time from go_on_ticks
	 */
	void snprintf_remaining_reversing_time(char *p, size_t size) const;

	/**
	 * How many free seats for passengers in convoy? Used in overcrowded loading
	 * @auhor Inkelyad
	 */
	inline const sint32 &get_free_seats() const { return free_seats; }

	/**
	* Schedule convoid for self destruction. Will be executed
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
	* Debug info nach stderr
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
	inline sint64* get_finance_history() { return *financial_history; }

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
	 * Methode fuer jaehrliche aktionen
	 * @author Hj. Malthaner
	 */
	void neues_jahr();

	inline void set_update_line(linehandle_t l) { line_update_pending = l; }

	void set_home_depot(koord3d hd) { home_depot = hd; }

	inline koord3d get_home_depot() { return home_depot; }

	/**
	 * this give the index of the next signal or the end of the route
	 * convois will slow down before it, if this is not a waypoint or the cannot pass
	 * The slowdown ist done by the vehicle routines
	 * @author prissi
	 */
	uint16 get_next_stop_index() const {return next_stop_index;}
	void set_next_stop_index(uint16 n);

	/* including this route_index, the route was reserved the laste time
	 * currently only used for tracks
	 */
	uint16 get_next_reservation_index() const {return next_reservation_index;}
	void set_next_reservation_index(uint16 n);

	/* the current state of the convoi */
	COLOR_VAL get_status_color() const;

	// returns tiles needed for this convoi
	uint16 get_tile_length() const;

	// get cached obsolescence.
	inline bool has_obsolete_vehicles() const { return has_obsolete; }

	inline bool get_withdraw() const { return withdraw; }

	void set_withdraw(bool new_withdraw);

	inline bool get_no_load() const { return no_load; }

	inline void set_no_load(bool new_no_load) { no_load = new_no_load; }

	/**
	* unset line -> remove cnv from line
	* @author hsiegeln
	*/
	void unset_line();

	replace_data_t* get_replace() const { return replace; }

	void set_replace(replace_data_t *new_replace);

	void clear_replace();

	bool get_depot_when_empty() const { return depot_when_empty; }

	void set_depot_when_empty(bool new_dwe);

	// True if the convoy has the same vehicles
	bool has_same_vehicles(convoihandle_t other) const;

	// Go to depot, if possible
	bool go_to_depot(bool show_success, bool use_home_depot = false);

	// True if convoy has no cargo
	//@author: isidoro
	bool has_no_cargo() const;

	void must_recalc_data() { invalidate_adverse_summary(); }

	// Overtaking for convois
	virtual bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other);

	//Returns the maximum catering level of the category type given in the convoy.
	//@author: jamespetts
	uint8 get_catering_level(uint8 type) const;

	//@author: jamespetts
	inline bool get_reversable() const { return reversable; }
	inline bool is_reversed() const { return reversed; }

	//@author: jamespetts
	uint32 calc_highest_axle_load();
	inline uint32 get_highest_axle_load() const { return highest_axle_load; }
	
	//@author: jamespetts
	uint32 calc_longest_min_loading_time();
	uint32 calc_longest_max_loading_time();
	inline uint32 get_longest_min_loading_time() const { return longest_min_loading_time; }
	inline uint32 get_longest_max_loading_time() const { return longest_max_loading_time; }

	void calc_current_loading_time(uint16 load_charge);
	inline uint16 get_current_loading_time() const { return current_loading_time; }

	// @author: jamespetts
	// Returns the number of standing passengers (etc.) in this convoy.
	uint16 get_overcrowded() const;

	// @author: jamespetts
	// Returns the average comfort of this convoy,
	// taking into account any catering.
	uint8 get_comfort() const;

	/** The new revenue calculation method for per-leg
	 * based revenue calculation, rather than per-hop
	 * based revenue calculation. This method calculates
	 * the revenue of a ware packet as it is unloaded.
	 *
	 * It also calculates allocations of revenue to different
	 * players based on track usage.
	 * @author: jamespetts, neroden, Knightly
	 */
	sint64 calc_revenue(const ware_t &ware, array_tpl<sint64> & apportioned_revenues);

	uint16 get_livery_scheme_index() const;
	void set_livery_scheme_index(uint16 value) { livery_scheme_index = value; }

	void apply_livery_scheme();
	sint64 get_average_round_trip_time() {
		int items = arrival_to_first_stop.get_count();
		if (items>1) {
			return (arrival_to_first_stop[items-1] - arrival_to_first_stop[0])/(items-1);
		} else {
			return 0;
		}
	}

	uint32 calc_reverse_delay() const;

	static uint16 get_waiting_minutes(uint32 waiting_ticks);
	
	bool is_wait_infinite() const { return go_on_ticks == WAIT_INFINITE; }

	route_infos_t& get_route_infos();

	void set_akt_speed(sint32 akt_speed) { 
		this->akt_speed = akt_speed; 
		if (akt_speed > 8)
			v = speed_to_v(akt_speed/2); 
		else
			v = speed_to_v(akt_speed); 
	}

	bool is_circular_route() const;
	
	/** For going to a depot automatically
	 *  when stuck - will teleport if necessary.
	 */
	void emergency_go_to_depot();

	koordhashtable_tpl<id_pair, average_tpl<uint16> > * get_average_journey_times() const;
	inline koordhashtable_tpl<id_pair, average_tpl<uint16> > * get_average_journey_times_this_convoy_only() const { return average_journey_times; }

	/**
	 * Clears the departure data.
	 * Used when the line changes
	 * its shcedule.
	 */
	void clear_departures();
};

#endif
