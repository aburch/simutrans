/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMWORLD_H
#define SIMWORLD_H


#include "simconst.h"
#include "simtypes.h"
#include "simunits.h"

#include "convoihandle_t.h"
#include "halthandle_t.h"

#include "tpl/weighted_vector_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/slist_tpl.h"
#include "tpl/koordhashtable_tpl.h"

#include "dataobj/settings.h"
#include "network/pwd_hash.h"
#include "dataobj/loadsave.h"
#include "dataobj/rect.h"

#include "simware.h"

#include "simplan.h"

#include "simdebug.h"

#ifdef _MSC_VER
#define snprintf sprintf_s
#else
#define sprintf_s snprintf
#endif

#ifdef MULTI_THREAD
#include "utils/simthread.h"
#endif

struct sound_info;
class stadt_t;
class fabrik_t;
class gebaeude_t;
class zeiger_t;
class grund_t;
class planquadrat_t;
class main_view_t;
class interaction_t;
class sync_steppable;
class tool_t;
class scenario_t;
class message_t;
class way_desc_t;
class tunnel_desc_t;
class network_world_command_t;
class goods_desc_t;
class memory_rw_t;
class viewport_t;

#define CHK_RANDS 32
#define CHK_DEBUG_SUMS 8

#ifdef MULTI_THREAD
//#define FORBID_MULTI_THREAD_PASSENGER_GENERATION_IN_NETWORK_MODE
//#define FORBID_MULTI_THREAD_PATH_EXPLORER
#define MULTI_THREAD_PASSENGER_GENERATION
#ifndef FORBID_MULTI_THREAD_CONVOYS
#define MULTI_THREAD_CONVOYS
#endif
#ifndef FORBID_MULTI_THREAD_PATH_EXPLORER
#define MULTI_THREAD_PATH_EXPLORER
#endif
#ifndef FORBID_MULTI_THREAD_ROUTE_UNRESERVER
#define MULTI_THREAD_ROUTE_UNRESERVER
#endif
#endif

#ifndef FORBID_MULTI_THREAD_PASSENGER_GENERATION_IN_NETWORK_MODE
//#define FORBID_SYNC_OBJECTS
//#define FORBID_PRIVATE_CARS
//#define FORBID_PEDESTRIANS
//#define FORBID_CONGESTION_EFFECTS
//#define DISABLE_JOB_EFFECTS
//#define FORBID_PUBLIC_TRANSPORT
//#define FORBID_RETURN_TRIPS
//#define DISABLE_GLOBAL_WAITING_LIST
//#define FORBID_PARALLELL_PASSENGER_GENERATION_IN_NETWORK_MODE // Revised to work only in network mode for testing VS/GCC desync (May 2017)
//#define FORBID_SWITCHING_TO_RETURN_ON_FOOT
//#define FORBID_SET_GENERATED_PASSENGERS
//#define FORBID_RECORDING_RETURN_FACTORY_PASSENGERS
//#define FORBID_FIND_ROUTE_FOR_RETURNING_PASSENGERS_1
//#define FORBID_FIND_ROUTE_FOR_RETURNING_PASSENGERS_2
//#define FORBID_STARTE_MIT_ROUTE_FOR_RETURNING_PASSENGERS
#endif

struct checklist_t
{
	uint32 ss;
	uint32 st;
	uint8 nfc;
	uint32 random_seed;
	uint16 halt_entry;
	uint16 line_entry;
	uint16 convoy_entry;

	uint32 rand[CHK_RANDS];
	uint32 debug_sum[CHK_DEBUG_SUMS];


	checklist_t(uint32 _ss, uint32 _st, uint8 _nfc, uint32 _random_seed, uint16 _halt_entry, uint16 _line_entry, uint16 _convoy_entry, uint32 *_rands, uint32 *_debug_sums);
	checklist_t() : ss(0), st(0), nfc(0), random_seed(0), halt_entry(0), line_entry(0), convoy_entry(0)
	{
		for(  uint8 i = 0;  i < CHK_RANDS;  i++  ) {
			rand[i] = 0;
		}
		for(  uint8 i = 0;  i < CHK_DEBUG_SUMS;  i++  ) {
			debug_sum[i] = 0;
		}
	}

	bool operator == (const checklist_t &other) const
	{
		bool rands_equal = true;
		for(  uint8 i = 0;  i < CHK_RANDS  &&  rands_equal;  i++  ) {
			rands_equal = rands_equal  &&  rand[i] == other.rand[i];
		}
		bool debugs_equal = true;
		for(  uint8 i = 0;  i < CHK_DEBUG_SUMS  &&  debugs_equal;  i++  ) {
			// If debug sums are too expensive, then this test below would allow them to be switched off independently at either end:
			// debugs_equal = debugs_equal  &&  (debug_sum[i] == 0  ||  other.debug_sum[i] == 0  ||  debug_sum[i] == other.debug_sum[i]);
			debugs_equal = debugs_equal  &&  debug_sum[i] == other.debug_sum[i];
		}
		return ( rands_equal &&
			debugs_equal &&
			ss == other.ss &&
			st == other.st &&
			nfc == other.nfc &&
			random_seed == other.random_seed &&
			halt_entry == other.halt_entry &&
			line_entry == other.line_entry &&
			convoy_entry == other.convoy_entry
			);
	}
	bool operator != (const checklist_t &other) const { return !( (*this)==other ); }

	void rdwr(memory_rw_t *buffer);
	int print(char *buffer, const char *entity) const;
};

// Private car ownership information.
// @author: jamespetts
// (But much of this code is adapted from the speed bonus code,
// written by Prissi).

class car_ownership_record_t
{
public:
	sint64 year;
	sint16 ownership_percent;
	car_ownership_record_t( sint64 y = 0, sint16 ownership = 0 )
	{
		year = y * 12;
		ownership_percent = ownership;
	};
};

class transferring_cargo_t
{
public:
	ware_t ware;
	sint64 ready_time;

	bool operator ==(const transferring_cargo_t& o)
	{
		return ware == o.ware && o.ready_time == ready_time;
	}
};

/**
 * Threaded function caller.
 */
typedef void (karte_t::*xy_loop_func)(sint16, sint16, sint16, sint16 /*, sint32*/);


/**
 * The map is the central part of the simulation. It stores all data and objects.
 * @brief Stores all data and objects of the simulated world.
 * @author Hj. Malthaner
 */
class karte_t
{
	friend karte_t* world();  // to access the single instance
	friend class karte_ptr_t; // to access the single instance

	static karte_t* world; ///< static single instance

public:
	/**
	 * Height of a point of the map with "perlin noise"
	 *
	 * @param frequency in 0..1.0 roughness, the higher the rougher
	 * @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
	 * @author Hj. Malthaner
	 */
	static sint32 perlin_hoehe(settings_t const*, koord pos, koord const size, sint32 map_size_max);
	sint32 perlin_hoehe(settings_t const*, koord pos, koord const size);

	/**
	 * Loops over tiles setting heights from perlin noise
	 */
	void perlin_hoehe_loop(sint16, sint16, sint16, sint16);

	enum player_cost {
		WORLD_CITICENS=0,		//!< total people
		WORLD_JOBS,				//!< total jobs
		WORLD_VISITOR_DEMAND,	//!< total visitor demand
		WORLD_GROWTH,			//!< growth (just for convenience)
		WORLD_TOWNS,			//!< number of all cities
		WORLD_FACTORIES,		//!< number of all consuming only factories
		WORLD_CONVOIS,			//!< total number of convois
		WORLD_CITYCARS,			//!< number of passengers completing their journeys by private car
		WORLD_PAS_RATIO,		//!< percentage of passengers that started successful
		WORLD_PAS_GENERATED,	//!< total number generated
		WORLD_MAIL_RATIO,		//!< percentage of mail that started successful
		WORLD_MAIL_GENERATED,	//!< all letters generated
		WORLD_GOODS_RATIO,		//!< ratio of chain completeness
		WORLD_TRANSPORTED_GOODS,//!< all transported goods
		WORLD_CAR_OWNERSHIP,	//!< The proportion of people with access to a private car
		MAX_WORLD_COST
	};

	#define MAX_WORLD_HISTORY_YEARS  (12) // number of years to keep history
	#define MAX_WORLD_HISTORY_MONTHS  (12) // number of months to keep history

	enum route_status_type
	{
		initialising,
		no_route,
		too_slow,
		overcrowded,
		destination_unavailable,
		public_transport,
		private_car,
		on_foot
	};

	enum { NORMAL=0, PAUSE_FLAG = 0x01, FAST_FORWARD=0x02, FIX_RATIO=0x04 };

	/**
	 * Missing things during loading:
	 * factories, vehicles, roadsigns or catenary may be severe
	 */
	enum missing_level_t { NOT_MISSING=0, MISSING_FACTORY=1, MISSING_VEHICLE=2, MISSING_SIGN=3, MISSING_WAYOBJ=4, MISSING_ERROR=4, MISSING_BRIDGE, MISSING_BUILDING, MISSING_WAY };

	void set_car_ownership_history_month(int month, sint64 value) { finance_history_month[month][WORLD_CAR_OWNERSHIP] = value; }
	void set_car_ownership_history_year(int year, sint64 value) { finance_history_year[year][WORLD_CAR_OWNERSHIP] = value; }

private:
	/**
	 * @name Map properties
	 * Basic map properties are stored in this variables.
	 * @{
	 */
	/**
	 * @brief Map settings are stored here.
	 */
	settings_t settings;

	/**
	 * For performance reasons we have the map grid size cached locally, comes from the environment (Einstellungen)
	 * @brief Cached map grid size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 */
	koord cached_grid_size;

	/**
	 * For performance reasons we have the map size cached locally, comes from the environment (Einstellungen).
	 * @brief Cached map size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 * @note These values are one less than the size values of the grid.
	 */
	koord cached_size;

	/**
	 * @brief The maximum of the two dimensions.
	 * Maximum size for waiting bars etc.
	 */
	int cached_size_max;
	/** @} */

	/**
	 * All cursor interaction goes via this function, it will call save_mouse_funk first with
	 * init, then with the position and with exit, when another tool is selected without click
	 * @see simtool.cc for practical examples of such functions.
	 */
	tool_t *selected_tool[MAX_PLAYER_COUNT];

	/**
	 * Redraw whole map.
	 */
	bool dirty;

	/*
	 * Redraw background.
	 */
	bool background_dirty;

 	/**
	 * True during destroying of the map.
	 */
	bool destroying;

#ifdef MULTI_THREAD
	/**
	* True when threads are to be terminated.
	*/
	bool terminating_threads;
#endif

	/**
	 * The rotation of the map when first loaded.
	 */
	uint8 loaded_rotation;

	/**
	 * The one and only camera looking at our world.
	 */
	viewport_t *viewport;

	/**
	 * @name Mouse pointer and cursor management
	 * @author Hj. Malthaner
	 * @{
	 */

	/**
	 * @brief Map mouse cursor tool.
	 */
	zeiger_t *zeiger;
	/** @} */

	/**
	 * Time when last mouse moved to check for ambient sound events.
	 */
	uint32 mouse_rest_time;

	/**
	 * Waiting time before next event.
	 */
	uint32 sound_wait_time;

	/**
	 * Gets an ambient sound id appropriate for the given tile.
	 * Returns NO_SOUND if no appropriate sound is found.
	 */
	sint16 get_sound_id(grund_t *gr);

	/**
	 * If true, this map cannot be saved.
	 */
	bool nosave;
	bool nosave_warning;

	/**
	 * Water level height.
	 * @author Hj. Malthaner
	 */
	sint8 groundwater;

	/**
	 * Current snow height.
	 * @note Might change during the game.
	 * @author prissi
	 */
	sint16 snowline;

	/**
	 * Changes the season and/or snowline height
	 * @author prissi
	 */
	void recalc_season_snowline(bool set_pending);

	/**
	 * >0 means a season change is needed
	 */
	sint8 pending_season_change;
	sint8 pending_snowline_change;

	/**
	 * Recalculates sleep time etc.
	 */
	void update_frame_sleep_time();

	/**
	 * Table for fast conversion from height to climate.
	 * @author prissi
	 */
	uint8 height_to_climate[32];

	/**
	 * Array containing the convois.
	 */
	vector_tpl<convoihandle_t> convoi_array;

	/**
	 * Array containing the factories.
	 */
	vector_tpl<fabrik_t *> fab_list;
	//slist_tpl<fabrik_t *> fab_list;

	/**
	 * Stores a list of goods produced by factories currently in the game;
	 */
	vector_tpl<const goods_desc_t*> goods_in_game;

	weighted_vector_tpl<gebaeude_t *> world_attractions;

	slist_tpl<koord> labels;

	/**
	 * Stores the cities.
	 */
	weighted_vector_tpl<stadt_t*> stadt;

	sint64 last_month_bev;

	/**
	 * The recorded history so far.
	 */
	sint64 finance_history_year[MAX_WORLD_HISTORY_YEARS][MAX_WORLD_COST];

	/**
	 * The recorded history so far.
	 */
	sint64 finance_history_month[MAX_WORLD_HISTORY_MONTHS][MAX_WORLD_COST];

	/**
	 * Attached view to this world.
	 */
	main_view_t *view;

	/**
	 * Event manager of this world.
	 */
	interaction_t *eventmanager;

	/**
	 * Checks whether the heights of the corners of the tile at (@p x, @p y) can be raised.
	 * If the desired height of a corner is lower than its current height, this corner is ignored.
	 * @param player player who wants to lower
	 * @param x coordinate
	 * @param y coordinate
	 * @param keep_water returns false if water tiles would be raised above water
	 * @param hsw desired height of sw-corner
	 * @param hse desired height of se-corner
	 * @param hse desired height of ne-corner
	 * @param hnw desired height of nw-corner
	 * @returns NULL if raise_to operation can be performed, an error message otherwise
	 */
	const char* can_raise_to(const player_t* player, sint16 x, sint16 y, bool keep_water, bool allow_deep_water, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw) const;

	/**
	 * Raises heights of the corners of the tile at (@p x, @p y).
	 * New heights for each corner given.
	 * @pre can_raise_to should be called before this method.
	 * @see can_raise_to
	 * @returns count of full raise operations (4 corners raised one level)
	 * @note Clear tile, reset water/land type, calc reliefkarte (relief map) pixel.
	 */
	int  raise_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);

	/**
	 * Checks whether the heights of the corners of the tile at (@p x, @p y) can be lowered.
	 * If the desired height of a corner is higher than its current height, this corner is ignored.
	 * @param player player who wants to lower
	 * @param x coordinate
	 * @param y coordinate
	 * @param hsw desired height of sw-corner
	 * @param hse desired height of se-corner
	 * @param hse desired height of ne-corner
	 * @param hnw desired height of nw-corner
	 * @returns NULL if lower_to operation can be performed, an error message otherwise
	 */
	const char* can_lower_to(const player_t* player, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, bool allow_deep_water) const;

	/**
	 * Lowers heights of the corners of the tile at (@p x, @p y).
	 * New heights for each corner given.
	 * @pre can_lower_to should be called before this method.
	 * @see can_lower_to
	 * @returns count of full lower operations (4 corners lowered one level)
	 * @note Clear tile, reset water/land type, calc reliefkarte (relief map) pixel.
	 */
	int  lower_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);

	/**
	 * Raise grid point (@p x,@p y). Changes grid_hgts only, used during map creation/enlargement.
	 * @see clean_up
	 */
	void raise_grid_to(sint16 x, sint16 y, sint8 h);

	/**
	 * Lower grid point (@p x,@p y). Changes grid_hgts only, used during map creation/enlargement.
	 * @see clean_up
	 */
	void lower_grid_to(sint16 x, sint16 y, sint8 h);

	/**
	 * The fractal generation of the map is not perfect.
	 * cleanup_karte() eliminates errors.
	 * @author Hj. Malthaner
	 */
	void cleanup_karte( int xoff, int yoff );

	/**
	 * @name Map data structures
	 *       This variables represent the simulated map.
	 * @{
	 */

	/**
	 * Array containing all the map tiles.
	 * @see cached_size
	 */
	planquadrat_t *plan;

	/**
	 * Array representing the height of each point of the grid.
	 * @see cached_grid_size
	 */
	sint8 *grid_hgts;

	/**
	 * Array representing the height of water on each point of the grid.
	 * @see cached_grid_size
	 */
	sint8 *water_hgts;
	/** @} */

	/**
	 * @name Player management
	 *       Variables related to the player management in game.
	 * @author Hj. Malthaner
	 * @{
	 */
	/**
	 * The players of the game.
	 * @note Standard human player has index 0, public player 1.
	 */
	player_t *players[MAX_PLAYER_COUNT];

	/**
	 * Active player.
	 */
	player_t *active_player;

	/**
	 * Active player index.
	 */
	uint8 active_player_nr;

	/**
	 * Locally stored password hashes, will be used after reconnect to a server.
	 */
	pwd_hash_t player_password_hash[MAX_PLAYER_COUNT];
	/** @} */

	/*
	 * Counter for schedules.
	 * If a new schedule is active, this counter will increment
	 * stations check this counter and will reroute their goods if changed.
	 * @author prissi
	 */
	uint8 schedule_counter;

	/**
	 * @name Display timing and scheduling
	 *       These variables store system display timings in the past frames
	 *       and allow for adequate adjustments to adapt to the system performance
	 *       and available resources (also in network mode).
	 * @{
	 */

	/**
	 * ms since creation.
	 * @note The time is in ms (milliseconds)
	 * @author Hj. Malthaner
	 */
	sint64 ticks;		      // ms since creation

	/**
	 * Ticks counter at last steps.
	 * @note The time is in ms (milliseconds)
	 * @author Hj. Malthaner
	 */
	sint64 last_step_ticks;

	/**
	 * From now on is next month.
	 * @note The time is in ms (milliseconds)
	 * @author Hj. Malthaner
	 */
	sint64 next_month_ticks;

	/**
	 * Default time stretching factor.
	 */
	uint32 time_multiplier;

	uint8 step_mode;

	/// @note variable used in interactive()
	uint32 sync_steps;

	// The maximum sync_steps that a client can safely advance to.
	uint32 sync_steps_barrier;

#define LAST_CHECKLISTS_COUNT 64
	/// @note variable used in interactive()
	checklist_t last_checklists[LAST_CHECKLISTS_COUNT];
#define LCHKLST(x) (last_checklists[(x) % LAST_CHECKLISTS_COUNT])
	uint32 rands[CHK_RANDS];
	uint32 debug_sums[CHK_DEBUG_SUMS];


	/// @note variable used in interactive()
	uint8  network_frame_count;
	/**
	 * @note Variable used in interactive().
	 * @note Set in reset_timer().
	 */
	uint32 fix_ratio_frame_time;

	/**
	 * For performance comparison.
	 * @author Hj. Malthaner
	 */
	uint32 realFPS;

	/**
	 * For performance comparison.
	 * @author Hj. Malthaner
	 */
	uint32 simloops;

	/// To calculate the fps and the simloops.
	uint32 last_frame_ms[32];

	/// To calculate the fps and the simloops.
	uint32 last_step_nr[32];

	/// To calculate the fps and the simloops.
	uint8 last_frame_idx;

	/**
	 * ms, when the last time events were handled.
	 * To calculate the fps and the simloops.
	 */
	uint32 last_interaction;

	/**
	 * ms, when the last step was done.
	 * To calculate the fps and the simloops.
	 */
	uint32 last_step_time;

	/**
	 * ms, when the next step is to be done.
	 * To calculate the fps and the simloops.
	 */
	uint32 next_step_time;

	/// To calculate the fps and the simloops.
	uint32 idle_time;
	/** @} */

	/**
	 * Current accumulated month number, counting January of year 0 as 0.
	 * @note last_month + (last_year*12);
	 */
	sint32 current_month;

	/**
	 * Last month 0..11
	 */
	sint32 last_month;

	/**
	 * Last year.
	 */
	sint32 last_year;

	/**
	 * Current season.
	 * @note 0=winter, 1=spring, 2=summer, 3=autumn
	 */
	uint8 season;

	/**
	 * Number of steps since creation.
	 */
	sint32 steps;

	/**
	 * Flag, that now no sound will play.
	 */
	bool is_sound;

	/**
	 * Flag for ending simutrans (true -> end simutrans).
	 */
	bool finish_loop;

	/**
	 * May change due to timeline.
	 */
	const way_desc_t *city_road;

	// Data for maintaining industry density even
	// after industries close
	// @author: jamespetts
	uint32 industry_density_proportion;
	uint32 actual_industry_density;

	/**
	 * What game objectives.
	 */
	scenario_t *scenario;

	/**
	 * Holds all the text messages in the messagebox (chat, new vehicle etc).
	 */
	message_t *msg;

	/**
	 * Used to distribute the workload when changing seasons to several steps.
	 */
	uint32 tile_counter;

	/**
	 * To identify different stages of the same game.
	 */
	uint32 map_counter;

	/**
	 * Re-calculate vehicle details monthly.
	 * Used to be used for the speed bonus
	 */
	void recalc_average_speed(bool skip_messages);

	/**
	 * Monthly actions.
	 */
	void new_month();

	/**
	 * Yearly actions.
	 */
	void new_year();

	/**
	 * Internal saving method.
	 * @author Hj. Malthaner
	 */
	void save(loadsave_t *file,bool silent);

	/**
	 * Internal loading method.
	 * @author Hj. Malthaner
	 */
	void load(loadsave_t *file);

	/**
	 * Removes all objects, deletes all data structures and frees all accessible memory.
	 * @author Hj. Malthaner
	 */
	void destroy();

	/**
	 * Restores history for older savegames.
	 */
	void restore_history();

	/**
	 * Will create rivers.
	 */
	void create_rivers(sint16 number);

	/**
	 * Will create lakes.
	 */
	void create_lakes( int xoff, int yoff );

	/**
	 * Will create beaches.
	 */
	void create_beaches( int xoff, int yoff );

	/**
	 * Distribute groundobjs and cities on the map but not
	 * in the rectangle from (0,0) till (old_x, old_y).
	 * It's now an extra function so we don't need the code twice.
	 * @author Gerd Wachsmuth, neroden
	 */
	void distribute_groundobjs_cities(settings_t const * const set, sint16 old_x, sint16 old_y);

	/**
	 * Distribute just the cities.  Subroutine of distribute_groundobjs_cities.
	 * Skipped if no cities are being built.
	 * @author Gerd Wachsmuth, neroden
	 */
	void distribute_cities(settings_t const * const set, sint16 old_x, sint16 old_y);

	// Used for detecting whether paths/connexions are stale.
	// @author: jamespetts
	uint16 base_pathing_counter;

	sint32 citycar_speed_average;

	void set_citycar_speed_average();

	bool recheck_road_connexions;

	/**
	 * Speed per tile in *tenths* of
	 * minutes.
	 * @author: jamespetts, April 2010
	 */
	uint32 generic_road_time_per_tile_city;
	uint32 generic_road_time_per_tile_intercity;

	uint32 max_road_check_depth;

	slist_tpl<stadt_t*> cities_awaiting_private_car_route_check;

	/**
	 * The last time when a server announce was performed (in ms).
	 */
	uint32 server_last_announce_time;

	enum { SYNCX_FLAG = 0x01, GRIDS_FLAG = 0x02 };

	void world_xy_loop(xy_loop_func func, uint8 flags);
	static void *world_xy_loop_thread(void *);

	/**
	 * Loops over plans after load.
	 */
	void plans_finish_rd(sint16, sint16, sint16, sint16);

	/**
	 * Updates all images.
	 */
	void update_map_intern(sint16, sint16, sint16, sint16);

	/**
	 * This contains all buildings in the world from which passenger
	 * journeys ultimately start, weighted by their level.
	 * @author: jamespetts
	 */
	weighted_vector_tpl <gebaeude_t *> passenger_origins;

	/**
	 * This contains all buildings in the world to which passengers make
	 * journeys to work, weighted by their (adjusted) level.
	 * This is an array indexed by class.
	 * @author: jamespetts
	 */
	weighted_vector_tpl <gebaeude_t *> *commuter_targets;

	/**
	 * This contains all buildings in the world to which passengers make
	 * journeys other than to work, weighted by their (adjusted) level.
	 * This is an array indexed by class.
	 * @author: jamespetts
	 */
	weighted_vector_tpl <gebaeude_t *> *visitor_targets;

	/**
	 * This contains all buildings in the world to and from which mail
	 * is delivered and generated respectively, weighted by their mail
	 * level.
	 * @author: jamespetts
	 */
	weighted_vector_tpl <gebaeude_t *> mail_origins_and_targets;

	/** Stores the value of the next step for passenger/mail generation
	 * purposes.
	 */
	sint32 next_step_passenger;
	sint32 next_step_mail;

	sint32 passenger_step_interval;
	sint32 mail_step_interval;

	// Signals in the time interval working method that need
	// to be checked periodically to see whether they need
	// to change to a less restrictive aspect.
	vector_tpl<signal_t*> time_interval_signals_to_check;

	// Do not repeat sounds from the same types of vehicles
	// too often, so store the time when the next sound from
	// that type of vehicle should next be played.
	//
	// Vehicles use their waytypes. ignore_wt is used when
	// the cooldown timer should not be used. noise_barrier_wt
	// is used for industry; overheadlines_wt is used for
	// crossings.
	sint64 sound_cooldown_timer[noise_barrier_wt + 1];

	// The number of operations to run in parallel.
	// This is important for multi-threading
	// synchronisation over the network.
	// -1: this is not in network mode: use the number of threads
	// 0: this is the network server: broadcast the number of threads
	// >0: This is the number of parallel operations to use.
	sint32 parallel_operations;

	/// A helper method for use in init/new month
	void recalc_passenger_destination_weights();

#ifdef MULTI_THREAD
	bool passengers_and_mail_threads_working;
	bool convoy_threads_working;
	bool path_explorer_working;
	bool private_car_threads_working;
public:
	static simthread_barrier_t step_convoys_barrier_external;
	static simthread_barrier_t unreserve_route_barrier;
	static simthread_barrier_t private_car_barrier;
	static pthread_mutex_t unreserve_route_mutex;
	static pthread_mutex_t step_passengers_and_mail_mutex;
	static bool private_car_route_mutex_initialised;
	static pthread_mutex_t private_car_route_mutex;
	void start_passengers_and_mail_threads();
	void start_convoy_threads();
	void start_path_explorer();
	void start_private_car_threads(bool override_suspend = false);
#else
public:
#endif
	// These will do nothing if multi-threading is disabled.
	void await_passengers_and_mail_threads();
	void await_convoy_threads();
	void await_path_explorer();
	void await_private_car_threads(bool override_suspend = false);
	void suspend_private_car_threads();
	void await_all_threads();

	enum building_type { passenger_origin, commuter_target, visitor_target, mail_origin_or_target, none };
	enum trip_type { commuting_trip, visiting_trip, mail_trip };

private:

	enum destination_object_type { town, factory, attraction, invalid };

	struct destination
	{
		koord location;
		uint16 type;
		gebaeude_t* building;
	};

	/**
	* Generates passengers and mail from all origin buildings
	* to be distributed to all destination buildings
	*/
	void step_passengers_and_mail(uint32 delta_t);

	sint32 calc_adjusted_step_interval(const uint32 weight, uint32 trips_per_month_hundredths) const;

	sint32 generate_passengers_or_mail(const goods_desc_t * wtyp);

	destination find_destination(trip_type trip, uint8 g_class);

#ifdef MULTI_THREAD
	friend void *check_road_connexions_threaded(void* args);
	friend void *unreserve_route_threaded(void* args);
	friend void *step_passengers_and_mail_threaded(void* args);
	friend void *step_convoys_threaded(void* args);
	friend void *path_explorer_threaded(void* args);
	friend void *step_individual_convoy_threaded(void* args);
	static sint32 cities_to_process;
	static vector_tpl<convoihandle_t> convoys_next_step;
	public:
	static bool threads_initialised;

	// These are both intended to be arrays of vectors
	static vector_tpl<private_car_t*> *private_cars_added_threaded;
	static vector_tpl<pedestrian_t*> *pedestrians_added_threaded;

	static thread_local uint32 passenger_generation_thread_number;
	static thread_local uint32 marker_index;

	// These are used so as to obviate the need to create and
	// destroy a vector of start halts every time that the
	// passenger generation is run.
	static vector_tpl<nearby_halt_t> *start_halts;
	static vector_tpl<halthandle_t> *destination_list;

	private:
#else
	public:
	static const uint32 marker_index = UINT32_MAX_VALUE;
	static vector_tpl<nearby_halt_t> start_halts;
	static vector_tpl<halthandle_t> destination_list;
#endif

public:

	static void privatecar_init(const std::string &objfilename);

private:

	static const sint16 default_car_ownership_percent = 25;

	// This is an array indexed by the number of passenger classes.
	static vector_tpl<car_ownership_record_t>* car_ownership;

	sint16 get_private_car_ownership(sint32 monthyear, uint8 g_class) const;
	void privatecar_rdwr(loadsave_t *file);

public:

	void set_rands(uint8 num, uint32 val) { rands[num] = val; }
	void inc_rands(uint8 num) { rands[num]++; }
	inline void add_to_debug_sums(uint8 num, uint32 val) { debug_sums[num] += val; }


	/**
	 * Announce server and current state to listserver.
	 * @param status Specifies what information should be announced
	 * or offline (the latter only in cases where it is shutting down)
	 */
	void announce_server(int status);

	vector_tpl<fabrik_t*> closed_factories_this_month;

	/// cache the current maximum and minimum height on the map
	sint8 max_height, min_height;

	/**
	 * Returns the messagebox message container.
	 */
	message_t *get_message() const { return msg; }

	/**
	 * Set to something useful, if there is a total distance != 0 to show in the bar below.
	 */
	koord3d show_distance;

	/**
	 * For warning, when stuff had to be removed/replaced
	 * level must be >=1 (1=factory, 2=vehicles, 3=not so important)
	 * may be refined later
	 */
	void add_missing_paks( const char *name, missing_level_t critical_level );

	/**
	 * Absolute month.
	 * @author prissi
	 */
	inline uint32 get_last_month() const { return last_month; }

	/**
	 * Returns last year.
	 * @author hsiegeln
	 */
	inline sint32 get_last_year() const { return last_year; }

	/**
	 * dirty: redraw whole screen.
	 * @author Hj. Malthaner
	 */
	void set_dirty() {dirty=true;}

	/**
	 * dirty: redraw whole screen.
	 * @author Hj. Malthaner
	 */
	void unset_dirty() {dirty=false;}

	/**
	 * dirty: redraw whole screen.
	 * @author Hj. Malthaner
	 */
	bool is_dirty() const {return dirty;}

	/**
	 * background_dirty: redraw background.
	 */
	void set_background_dirty() { background_dirty = true; }

	/**
	 * background_dirty: redraw whole screen.
	 */
	bool is_background_dirty() const { return background_dirty; }

	/**
	 * background_dirty: redraw background.
	 */
	void unset_background_dirty() { background_dirty = false; }

	/**
	 * @return true if the current viewport contains regions outside the world.
	 */
	bool is_background_visible() const;

	// do the internal accounting
	void buche(sint64 betrag, player_cost type);

	// calculates the various entries
	void update_history();

	scenario_t *get_scenario() const { return scenario; }
	void set_scenario(scenario_t *s);

	/**
	 * Returns the finance history for player.
	 * @author hsiegeln
	 */
	sint64 get_finance_history_year(int year, int type) const { return finance_history_year[year][type]; }

	/**
	 * Returns the finance history for player.
	 * @author hsiegeln
	 */
	sint64 get_finance_history_month(int month, int type) const { return finance_history_month[month][type]; }

	/**
	 * Returns pointer to finance history for player.
	 * @author hsiegeln
	 */
	const sint64* get_finance_history_year() const { return *finance_history_year; }

	/**
	 * Returns pointer to finance history for player.
	 * @author hsiegeln
	 */
	const sint64* get_finance_history_month() const { return *finance_history_month; }

	/**
	 * Recalcs all map images.
	 */
	void update_map();

	/**
	 * Recalcs images after change of underground mode.
	 */
	void update_underground();

	/**
	 * @brief Prepares an area of the map to be drawn.
	 *
	 * New area is the area that will be prepared. Old area is the area that was
	 * already prepared. Only the difference between the two rects is prepared.
	 */
	void prepare_tiles(rect_t const& new_area, rect_t const& old_area);

	/**
	 * @returns true if world gets destroyed
	 */
	bool is_destroying() const { return destroying; }

#ifdef MULTI_THREAD
	/**
	* @returns true if threads are being terminated
	*/
	bool is_terminating_threads() const { return terminating_threads;  }
#endif

	/**
	 * Gets the world view.
	 */
	main_view_t *get_view() const { return view; }

	/**
	 * Gets the world viewport.
	 */
	viewport_t *get_viewport() const { return viewport; }

	/**
	 * Sets the world view.
	 */
	void set_view(main_view_t *v) { view = v; }

	/**
	 * Sets the world event manager.
	 */
	void set_eventmanager(interaction_t *em) { eventmanager = em; }

	settings_t const& get_settings() const { return settings; }
	settings_t&       get_settings()       { return settings; }

	/// time lapse mode ...
	bool is_paused() const { return step_mode&PAUSE_FLAG; }
	/// stops the game with interaction
	void set_pause( bool );

	bool is_fast_forward() const { return step_mode == FAST_FORWARD; }
	void set_fast_forward(bool ff);

	/**
	 * (un)pause for network games.
	 */
	void network_game_set_pause(bool pause_, uint32 syncsteps_);

	/**
	 * @return The active mouse cursor.
	 */

	zeiger_t * get_zeiger() const { return zeiger; }

	/**
	 * Marks an area using the grund_t mark flag.
	 * @author prissi
	 */
	void mark_area( const koord3d center, const koord radius, const bool mark ) const;

	/**
	 * Player management here
	 */
	uint8 sp2num(player_t *player);
	player_t * get_player(uint8 n) const { return players[n&15]; }
	player_t* get_active_player() const { return active_player; }
	uint8 get_active_player_nr() const { return active_player_nr; }
	void switch_active_player(uint8 nr, bool silent);
	const char *init_new_player( uint8 nr, uint8 type );
	void store_player_password_hash( uint8 player_nr, const pwd_hash_t& hash );
	const pwd_hash_t& get_player_password_hash( uint8 player_nr ) const { return player_password_hash[player_nr]; }
	void clear_player_password_hashes();
	void rdwr_player_password_hashes(loadsave_t *file);
	void remove_player(uint8 player_nr);

	/**
	* Get the default public service player.
	* @return the default public service player
	*/
	player_t *get_public_player() const;

	/**
	 * Network safe initiation of new and deletion of players, change freeplay.
	 * @param param Player type (human / ai) of new players.
	 */
	void call_change_player_tool(uint8 cmd, uint8 player_nr, uint16 param, bool scripted_call=false);

	enum change_player_tool_cmds { new_player=1, toggle_freeplay=2, delete_player=3 };
	/**
	 * @param exec If false checks whether execution is allowed, if true executes tool.
	 * @returns Whether execution is allowed.
	 */
	bool change_player_tool(uint8 cmd, uint8 player_nr, uint16 param, bool public_player_unlocked, bool exec);

	/**
	 * If a schedule is changed, it will increment the schedule counter
	 * every step the haltestelle will check and reroute the goods if needed.
	 */
	uint8 get_schedule_counter() const { return schedule_counter; }

	/**
	 * If a schedule is changed, it will increment the schedule counter
	 * every step the haltestelle will check and reroute the goods if needed.
	 */
	void set_schedule_counter();

	/**
	 * @note Often used, therefore found here.
	 */
	bool use_timeline() const { return settings.get_use_timeline(); }

	void reset_timer();
	void reset_interaction();
	void step_year();

	/**
	 * Jump one or more months ahead.
	 * @note Updating history!
	 */
	void step_month( sint16 months=1 );

	/**
	 * @return Either 0 or the current year*16 + month
	 */
	uint16 get_timeline_year_month() const { return settings.get_use_timeline() ? current_month : 0; }

	/**
	 * Number of ticks per day in bits.
	 * @see ticks_per_world_month
	 * @author Hj. Malthaner
	 */
	uint32 ticks_per_world_month_shift;

	/**
	 * Number of ticks per MONTH!
	 * @author Hj. Malthaner
	 */
	sint64 ticks_per_world_month;

	void set_ticks_per_world_month_shift(sint16 bits) {ticks_per_world_month_shift = bits; ticks_per_world_month = (1LL << ticks_per_world_month_shift); }

	/**
	 * Converts speed (yards per tick) into tiles per month
	 *       * A derived quantity:
	 * speed * ticks_per_world_month / yards_per_tile
	 * == speed << ticks_per_world_month_shift  (pak-set dependent, 18 in old games)
	 *          >> yards_per_tile_shift (which is 12 + 8 = 20, see above)
	 * This is hard to do with full generality, because shift operators
	 * take positive numbers!
	 *
	 * @author neroden
	 */
	uint32 speed_to_tiles_per_month(uint32 speed) const
	{
		const int left_shift = (int)(ticks_per_world_month_shift - YARDS_PER_TILE_SHIFT);
		if (left_shift >= 0) {
			return speed << left_shift;
		} else {
			const int right_shift = -left_shift;
			// round to nearest
			return (speed + (1<<(right_shift -1)) ) >> right_shift;
		}
	}

	/**
	 * Scales value proportionally with month length.
	 * Used to scale monthly maintenance costs and factory production.
	 * @returns value << ( ticks_per_world_month_shift -18 )
	 * DEPRECATED - use calc_adjusted_montly_figure() instead
	 */
	sint64 scale_with_month_length(sint64 value)
	{
		const int left_shift = ticks_per_world_month_shift - (sint64)get_settings().get_base_bits_per_month();
		if (left_shift >= 0) {
			return value << left_shift;
		} else {
			return value >> (-left_shift);
		}
	}

	/**
	 * Scales value inverse proportionally with month length.
	 * Used to scale monthly maintenance costs and factory production.
	 * @returns value << ( 18 - ticks_per_world_month_shift )
	 */
	sint64 inverse_scale_with_month_length(sint64 value)
	{
		const int left_shift = (sint64)get_settings().get_base_bits_per_month() - ticks_per_world_month_shift;
		if (left_shift >= 0) {
			return value << left_shift;
		} else {
			return value >> (-left_shift);
		}
	}

	sint32 get_time_multiplier() const { return time_multiplier; }
	void change_time_multiplier( sint32 delta );

	/**
	 * calc_adjusted_monthly_figure()
	 *
	 * NOTE: Avoid calling this from a method called from step(), as this is now more
	 * computationally intensive than previously. Precalculate figures periodically instead.
	 *
	 * Quantities defined on a per month base must be adjusted according to the virtual
	 * game speed defined by the ticks per month and meters per tile settings in simuconf.tab.
	 *
	 * NOTICE: Don't confuse, on the one hand, the game speed, represented by the number in the
	 * lower right hand corner, and, on the other hand, the ticks per month and meters per tile
	 * settings.
	 *
	 * The first increases or decreases the speed of all things that happen in the game by the
	 * same amount: the whole game is fast forwarded or slowed down.
	 *
	 * The second are very different: they alter the relative scale of the speed of the game,
	 * on the one hand, against the passing of time on the other. Increasing the speed using
	 * the ticks per month or metters per tile settings increases the number of months/years
	 * that pass in comparison to everything else that  happens in the game.
	 *
	 * Example (for bits per month only):
	 *
	 * suppose that you have a railway between two towns: A and B, and one train on
	 * that railway. At a speed setting of 1.00 and a ticks per month setting of 18 (the default),
	 * suppose that your train goes between A and B ten times per month, which takes ten real minutes.
	 * At a speed setting of 2.00 and the same ticks per month setting, in ten real minutes, the train
	 * would travel between A and B twenty times, earning twice as much revenue, and costing twice as
	 * much in maintenance, and two months would pass.
	 *
	 * However, if, instead of increasing the time to 2.00, you reduced the ticks per month to 17 and
	 * left the game speed at 1.00, in ten minutes of real time, the train would still travel ten times
	 * between A and B and earn the same amount of revenue, and cost the same amount in maintenance as
	 * the first instance, but two game months would pass.
	 *
	 * It should be apparent from that description that the maintenance cost per month needs to be
	 * scaled with the proportion of months to ticks, since, were it not, lowering the ticks per month
	 * setting would mean that the fixed maintenance cost (the per month cost) would increase the
	 * monthly maintenance cost, but not the variable maintenance cost. Since changing the ticks per
	 * month setting is supposed to be cost-neutral, this cannot happen, so all costs that are
	 * calculated monthly have to be adjusted to take account of the ticks per month setting in order
	 * to counteract its effect.
	 *
	 * James E. Petts
	 *
	 * same adjustment applies to production rates.
	 *
	 * @author: Bernd Gabriel, 14.06.2009
	 */

	// At all defaults, 1,000 meters per tile and 18 bits per month, we get 3.2 hours
	// (that is, 3:12h) in a month, or 1/7.5th of a day. If we want to have raw numbers based on
	// daily production for factories, daily electricity usage, daily passenger demand, etc. we
	// would need to multiply the defaults by 7.5 if we want the raw numbers in the pakset to be
	// based on these real life values.
	// Consider what to do about things already calibrated to a different level. (Answer: they could probably
	// do with recalibration anyway).

	sint32 calc_adjusted_monthly_figure(sint32 nominal_monthly_figure) const
	{
		// Adjust for meters per tile
		const sint32 base_meters_per_tile = (sint32)get_settings().get_base_meters_per_tile();
		const uint32 base_bits_per_month = (sint32)get_settings().get_base_bits_per_month();
		const sint32 adjustment_factor = base_meters_per_tile / (sint32)get_settings().get_meters_per_tile();

		// Adjust for bits per month
		if(ticks_per_world_month_shift >= base_bits_per_month)
		{
			const sint32 adjusted_monthly_figure = (sint32)(nominal_monthly_figure << (ticks_per_world_month_shift - base_bits_per_month));
			return adjusted_monthly_figure / adjustment_factor;
		}
		else
		{
			const sint32 adjusted_monthly_figure = nominal_monthly_figure / adjustment_factor;
			return (sint32)(adjusted_monthly_figure >> (base_bits_per_month - ticks_per_world_month_shift));
		}
	}

	sint64 calc_adjusted_monthly_figure(sint64 nominal_monthly_figure) const
	{
		// Adjust for meters per tile
		const sint64 base_meters_per_tile = (sint64)get_settings().get_base_meters_per_tile();
		const sint64 base_bits_per_month = (sint64)get_settings().get_base_bits_per_month();
		const sint64 adjustment_factor = base_meters_per_tile / (sint64)get_settings().get_meters_per_tile();

		// Adjust for bits per month
		if(ticks_per_world_month_shift >= base_bits_per_month)
		{
			if (nominal_monthly_figure < adjustment_factor)
			{
				// This situation can lead to loss of precision.
				const sint64 adjusted_monthly_figure = (nominal_monthly_figure * 100ll) / adjustment_factor;
				return (adjusted_monthly_figure << -(base_bits_per_month - ticks_per_world_month_shift)) / 100ll;
			}
			else
			{
				const sint64 adjusted_monthly_figure = nominal_monthly_figure / adjustment_factor;
				return (adjusted_monthly_figure << -(base_bits_per_month - ticks_per_world_month_shift));
			}
		}
		else
		{
			if (nominal_monthly_figure < adjustment_factor)
			{
				// This situation can lead to loss of precision.
				const sint64 adjusted_monthly_figure = (nominal_monthly_figure * 100ll) / adjustment_factor;
				return (adjusted_monthly_figure >> (base_bits_per_month - ticks_per_world_month_shift)) / 100ll;
			}
			else
			{
				const sint64 adjusted_monthly_figure = nominal_monthly_figure / adjustment_factor;
				return adjusted_monthly_figure >> (base_bits_per_month - ticks_per_world_month_shift);
			}
		}
	}

	uint64 calc_adjusted_monthly_figure(uint64 nominal_monthly_figure) const
	{
		// Adjust for meters per tile
		const uint64 base_meters_per_tile = (uint64)get_settings().get_base_meters_per_tile();
		const uint64 base_bits_per_month = (uint64)get_settings().get_base_bits_per_month();
		const uint64 adjustment_factor = base_meters_per_tile / (uint64)get_settings().get_meters_per_tile();

		// Adjust for bits per month
		if (ticks_per_world_month_shift >= base_bits_per_month)
		{
			const uint64 adjusted_monthly_figure = nominal_monthly_figure / adjustment_factor;
			return (adjusted_monthly_figure << -(base_bits_per_month - ticks_per_world_month_shift));
		}
		else
		{
			const uint64 adjusted_monthly_figure = nominal_monthly_figure / adjustment_factor;
			return adjusted_monthly_figure >> (base_bits_per_month - ticks_per_world_month_shift);
		}
	}

	uint32 calc_adjusted_monthly_figure(uint32 nominal_monthly_figure) const
	{
		// Adjust for meters per tile
		const uint32 base_meters_per_tile = get_settings().get_base_meters_per_tile();
		const uint32 base_bits_per_month =  get_settings().get_base_bits_per_month();
		const uint32 adjustment_factor = base_meters_per_tile / (uint32)get_settings().get_meters_per_tile();

		// Adjust for bits per month
		if(ticks_per_world_month_shift >= base_bits_per_month)
		{
			const uint32 adjusted_monthly_figure = (uint32)(nominal_monthly_figure << (ticks_per_world_month_shift - base_bits_per_month));
			return adjusted_monthly_figure / adjustment_factor;
		}
		else
		{
			const uint32 adjusted_monthly_figure = nominal_monthly_figure / adjustment_factor;
			return (uint32)(adjusted_monthly_figure >> (base_bits_per_month - ticks_per_world_month_shift));
		}
	}

	uint64 scale_for_distance_only(uint64 value) const
	{
		const uint64 base_meters_per_tile = (uint64)get_settings().get_base_meters_per_tile();
		const uint64 set_meters_per_tile = (uint64)get_settings().get_meters_per_tile();

		return (value * set_meters_per_tile) / base_meters_per_tile;
	}

	uint32 scale_for_distance_only(uint32 value) const
	{
		const uint32 base_meters_per_tile = (uint32)get_settings().get_base_meters_per_tile();
		const uint32 adjustment_factor = base_meters_per_tile / (uint32)get_settings().get_meters_per_tile();

		return value / adjustment_factor;
	}

	sint32 scale_for_distance_only(sint32 value) const
	{
		const sint32 base_meters_per_tile = (sint32)get_settings().get_base_meters_per_tile();
		const sint32 adjustment_factor = base_meters_per_tile / (sint32)get_settings().get_meters_per_tile();

		return value / adjustment_factor;
	}

	sint64 scale_for_distance_only(sint64 value) const
	{
		const sint64 base_meters_per_tile = (sint64)get_settings().get_base_meters_per_tile();
		const sint64 adjustment_factor = base_meters_per_tile / (sint64)get_settings().get_meters_per_tile();

		return value / adjustment_factor;
	}

	/**
	 * Standard timing conversion
	 * @author: jamespetts
	 */
	inline sint64 ticks_to_tenths_of_minutes(sint64 ticks) const
	{
		return ticks_to_seconds(ticks) / 6L;
	}

	/**
	 * Finer timing conversion for UI only
	 * @author: jamespetts
	 */
	inline sint64 ticks_to_seconds(sint64 ticks) const
	{
		/*
		 * Currently this is altered according to meters_per_tile / 1000.
		 * This is a "convention" to speed up time when changing distance;
		 * it needs to be changed (separated into a new world setting).
		 *
		 * The rest of this is much weirder: there are by default
		 * (4096 / 180) = 22.7555555... ticks per second.
		 * This also needs to be changed because it's stupid; it's based on
		 * old settings which are now in simunits.h
		 */
		return get_settings().get_meters_per_tile() * ticks * 30L * 6L / (4096L * 1000L);
	}
#ifndef NETTOOL
	/**
	* Reverse conversion of the above.
	*/
	inline sint64 get_seconds_to_ticks(uint32 seconds) const
	{
		// S = a * T * c * d / (e * f)
		// S / a = T * c * d / (e * f)
		// S / a / c / d = T / (e * f)
		// (S / a / c / d) * (e * f) = T

		//return ((sint64)seconds * 4096L * 1000L) / (sint64)get_settings().get_meters_per_tile() / 30L / 6L;

		return seconds_to_ticks(seconds, get_settings().get_meters_per_tile());
	}
#endif
	/**
	* Adds a single tile of a building to the relevant world list for passenger
	* and mail generation purposes
	* @author: jamespetts
	*/
	void add_building_to_world_list(gebaeude_t *gb, bool ordered = false);

	/**
	* Removes a single tile of a building to the relevant world list for passenger
	* and mail generation purposes
	* @author: jamespetts
	*/
	void remove_building_from_world_list(gebaeude_t *gb);

	/**
	* Updates the weight of a building in the world list if it changes its
	* passenger/mail demand
	* @author: jamespetts
	*/
	void update_weight_of_building_in_world_list(gebaeude_t *gb);

	/**
	* Removes all references to a city from every building in
	* the world. Used when deleting cities.
	*/
	void remove_all_building_references_to_city(stadt_t* city);

	/// Returns the region of the selected co-ordinate.
	uint8 get_region(koord k) const;
	static uint8 get_region(koord k, settings_t const* const sets);

	/// Returns the region name of the selected co-ordinate
	std::string get_region_name(koord k) const;

private:
	/*
	 * This is a cache to speed up several unit conversions and avoid
	 * excessive pointer indirection
	 * @author: neroden
	 */

	/**
	 * If this is true, the cached factors for travel speed are set.
	 * If not, call set_speed_factors.
	 */
	bool speed_factors_are_set;
	/**
	 * Multiply this by distance, then divide by movement_denominator, to get walking freight haulage time
	 * at 1 km/h in tenths of minutes
	 * Divide that by speed to get the haulage time at a given speed in tenths of minutes
	 */
	mutable uint32 unit_movement_numerator;
	/**
	 * Multiply this by distance, then divide by movement_denominator, to get walking passenger/mail time
	 * in tenths of minutes
	 */
	mutable uint32 walking_numerator;
	/**
	 * This is just to make the integer math work with enough precision.
	 * It will be slightly faster to make it a power of two shift.
	 */
	mutable uint32 movement_denominator_shift;

	/*
	 * Cache constant factors involved in walking time
	 * These can only be set once, not changed after world creation
	 * They are conceptually constant
	 * @author neroden
	 */
	void set_speed_factors() const
	{
		// effectively sets movement_denominator to 2^8 = 128
		movement_denominator_shift = 8;
		// Save confusion within this method: this will be optimized out
		const uint32 movement_denominator = 1 << movement_denominator_shift;

		/*
		 * Follow the logic:
		 * distance (in tiles) * meters_per_tile = distance (in meters)
		 * distance (in meters) * 1000 = distance (in kilometers)
		 * distance (in kilometers) / speed in km/h = time (in hours) -- this step is not cached here
		 * time (in hours) * 60 = time (in minutes)
		 * time (in minutes) * 10 = time (in tenths of minutes)
		 *
		 * Accordingly, we multiply distance in tiles by 600, and by meters_per_tile, and divide by 1000.
		 * Then, to allow for dividing by the speed without severe roundoff errors,
		 * (recalling that this will be used with high speeds)
		 * we multiply by an arbitrary denominator, which we will divide by later.
		 */

		// This represents 1 km/h:
		unit_movement_numerator = get_settings().get_meters_per_tile() * 6u * movement_denominator / 10u;
		// This represents walking speed:
		walking_numerator = unit_movement_numerator / get_settings().get_walking_speed();
	}

	/**
	* This is the list of passengers/mail/goods that
	* are transferring to/from buildings without
	* going via a stop. Those that go via a stop use
	* the like named vector in the haltestelle_t class.
	* This is an array of vectors, one per thread.
	* In single threaded mode, there is only one.
	*/
	vector_tpl<transferring_cargo_t> *transferring_cargoes;

	/**
	* Iterate through the transferring_cargoes
	* vector and dispatch all those cargoes
	* that are ready.
	*/
	void check_transferring_cargoes();

	// Calculate the time that a ware packet is
	// taken out of the waiting list.
	sint64 calc_ready_time(ware_t ware, koord origin_pos) const;

public:

	/**
	 * Conversion from walking distance in tiles to walking time
	 * Returns tenths of minutes
	 */
	inline uint32 walking_time_tenths_from_distance(uint32 distance) const {
		if (!speed_factors_are_set) {
			set_speed_factors();
		}
		return (distance * walking_numerator) >> movement_denominator_shift;
	}

	/**
	 * Conversion from walking distance in tiles to walking time
	 * Returns seconds; used for display purposes
	 */
	uint32 walking_time_secs_from_distance(uint32 distance) const {
		return walking_time_tenths_from_distance(distance) * 6u;
	}

	/**
	 * Conversion from distance to time, at speed given in km/h
	 * Returns tenths of minutes
	 */
	uint32 travel_time_tenths_from_distance(uint32 distance, uint32 speed) const {
		if (!speed_factors_are_set) {
			set_speed_factors();
		}
		return (distance * unit_movement_numerator / speed ) >> movement_denominator_shift;
	}

	/**
	 * Conversion from walking distance in tiles to walk haulage time for freight
	 * Walking haulage for freight is always at 1 km/h.
	 * Returns tenths of minutes
	 */
	uint32 walk_haulage_time_tenths_from_distance(uint32 distance) const {
		if (!speed_factors_are_set) {
			set_speed_factors();
		}
		return (distance * unit_movement_numerator) >> movement_denominator_shift;
	}

	uint32 meters_from_yards(uint32 yards) const {
		return (yards * get_settings().get_meters_per_tile()) >> YARDS_PER_TILE_SHIFT;
	}

	/**
	 * @return 0=winter, 1=spring, 2=summer, 3=autumn
	 * @author prissi
	 */
	uint8 get_season() const { return season; }

	/**
	 * Time since map creation in ms.
	 * @author Hj. Malthaner
	 */
	sint64 get_ticks() const { return ticks; }


	uint32 get_next_month_ticks() const { return (uint32) next_month_ticks; }

	/**
	 * Absolute month (count start year zero).
	 * @author prissi
	 */
	uint32 get_current_month() const { return current_month; }

	/**
	 * 1/8th month (0..95) within current year
	 *
	 * Is used to calculate seasonal images.
	 *
	 * Another ticks_bits_per_tag algorithm, I found repeatedly,
	 * although ticks_bits_per_tag should be private property of karte_t.
	 *
	 * @author: Bernd Gabriel, 14.06.2009
	 */
	int get_yearsteps() { return (int) ((current_month % 12) * 8 + ((ticks >> (ticks_per_world_month_shift-3)) & 7)); }

	/**
	 * prissi: current city road.
	 * @note May change due to timeline.
	 */
	const way_desc_t* get_city_road() const { return city_road; }

	/**
	 * Number of steps elapsed since the map was generated.
	 * @author Hj. Malthaner
	 *
	 * Number of steps since map production (Babelfish)
	 */
	sint32 get_steps() const { return steps; }

	/**
	 * Idle time. Nur zur Anzeige verwenden!
	 * @author Hj. Malthaner
	 *
	 * Idle time. Use only to the announcement! (Babelfish)
	 */
	uint32 get_idle_time() const { return idle_time; }

	/**
	 * Number of frames displayed in the last real time second.
	 * @author prissi
	 *
	 * Number of frames in the last second of real time (Babelfish)
	 */
	uint32 get_realFPS() const { return realFPS; }

	/**
	 * Number of simulation loops in the last second. Can be very inaccurate!
	 * @author Hj. Malthaner
	 *
	 * Number of simulation loops in the last second. Can be very inaccurate! (Babelfish)
	 */
	uint32 get_simloops() const { return simloops; }

	/**
	 * Returns the current waterline height.
	 * @author Hj. Malthaner
	 */
	sint8 get_groundwater() const { return groundwater; }

	/**
	 * Returns the minimum allowed height on the map.
	 */
	sint8 get_minimumheight() const { return groundwater-10; }

	/**
	 * Returns the maximum allowed world height.
	 * @author Hj. Malthaner
	 */
	sint8 get_maximumheight() const { return 32; }

	/**
	 * Returns the current snowline height.
	 * @author prissi
	 */
	sint16 get_snowline() const { return snowline; }

	/**
	 * Initializes the height_to_climate field from settings.
	 */
	void init_height_to_climate();

	/**
	* Update the status of time interval signals
	*/
	void step_time_interval_signals();

	/**
	* Add cargoes to the waiting list
	*/
	void add_to_waiting_list(ware_t ware, koord origin_pos);

	/**
	* Helper methods used for runway construction to check
	* the exclusion zone of 1 tile around a runway.
	*/
	struct runway_info { ribi_t::ribi direction; koord pos; };
	runway_info check_nearby_runways(koord pos);
	bool check_neighbouring_objects(koord pos);

#ifdef MULTI_THREAD
	/**
	* Initialise threads
	*/
	void init_threads();

	/**
	* De-initialise threads
	*/
	void destroy_threads();

	void clean_threads(vector_tpl<pthread_t>* thread);
#endif

	/**
	 * Returns the climate for a given height, ruled by world creation settings.
	 * Used to determine climate when terraforming, loading old games, etc.
	 */
	climate get_climate_at_height(sint16 height) const
	{
		const sint16 h=height-groundwater;
		if(h<0) {
			return water_climate;
		} else if(h>=32) {
			return arctic_climate;
		}
		return (climate)height_to_climate[h];
	}

	/**
	 * returns the current climate for a given koordinate
	 * @author Kieron Green
	 */
	inline climate get_climate(koord k) const {
		const planquadrat_t *plan = access(k);
		return plan ? plan->get_climate() : water_climate;
	}

	/**
	 * sets the current climate for a given koordinate
	 * @author Kieron Green
	 */
	inline void set_climate(koord k, climate cl, bool recalc) {
		planquadrat_t *plan = access(k);
		if(  plan  ) {
			plan->set_climate(cl);
			if(  recalc  ) {
				recalc_transitions(k);
				for(  int i = 0;  i < 8;  i++  ) {
					recalc_transitions( k + koord::neighbours[i] );
				}
			}
		}
	}

	void set_mouse_rest_time(uint32 new_val) { mouse_rest_time = new_val; };
	void set_sound_wait_time(uint32 new_val) { sound_wait_time = new_val; };

	/**
	* Call this when a ware is ready according to
	* check_transferring_cargoes().
	*/
	void deposit_ware_at_destination(ware_t ware);

	/** Get the number of parallel operations
	* currently set. This should be set by the server
	* in network games, and based on the thread count
	* in single player games.
	*/
	sint32 get_parallel_operations() const;

private:
	/**
	 * Dummy method, to generate compiler error if someone tries to call get_climate( int ),
	 * as the int parameter will silently be cast to koord...
	 */
	climate get_climate(sint16) const;

	/**
	 * iterates over the map starting from k setting the water height
	 * lakes are left where there is no drainage
	 * @author Kieron Green
	 */
	void drain_tile(koord k, sint8 water_height);
	bool can_flood_to_depth(koord k, sint8 new_water_height, sint8 *stage, sint8 *our_stage) const;

public:
	void flood_to_depth(sint8 new_water_height, sint8 *stage);

	/**
	 * Set a new tool as current: calls local_set_tool or sends to server.
	 */
	void set_tool( tool_t *tool_in, player_t * player );

	/**
	 * Set a new tool on our client, calls init.
	 */
	void local_set_tool( tool_t *tool_in, player_t * player );
	tool_t *get_tool(uint8 nr) const { return selected_tool[nr]; }

	/**
	* Calls the work method of the tool.
	* Takes network and scenarios into account.
	*/
	const char* call_work(tool_t *t, player_t *pl, koord3d pos, bool &suspended);

	/**
	 * Returns the (x,y) map size.
	 * @brief Map size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 * @note These values are exactly one less then get_grid_size ones.
	 * @see get_grid_size()
	 */
	inline koord const &get_size() const { return cached_grid_size; }

	/**
	 * Maximum size for waiting bars etc.
	 */
	inline int get_size_max() const { return cached_size_max; }

	/**
	 * @return True if the specified coordinate is inside the world tiles(planquadrat_t) limits, false otherwise.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_limits(sint16 x, sint16 y) const {
	// prissi: since negative values will make the whole result negative, we can use bitwise or
	// faster, since pentiums and other long pipeline processors do not like jumps
		return (x|y|(cached_size.x-x)|(cached_size.y-y))>=0;
//		return x>=0 &&  y>=0  &&  cached_size_x_karte_x>=x  &&  cached_size_x_karte_y>=y;
	}

	/**
	 * @return True if the specified coordinate is inside the world tiles(planquadrat_t) limits, false otherwise.
	 * @param k (x,y) coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_limits(koord k) const { return is_within_limits(k.x, k.y); }

	/**
	 * @return True if the specified coordinate is inside the world height grid limits, false otherwise.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_grid_limits(sint16 x, sint16 y) const {
	// prissi: since negative values will make the whole result negative, we can use bitwise or
	// faster, since pentiums and other long pipeline processors do not like jumps
		return (x|y|(cached_grid_size.x-x)|(cached_grid_size.y-y))>=0;
//		return x>=0 &&  y>=0  &&  cached_size_x_gitter_x>=x  &&  cached_size_x_gitter_y>=y;
	}

	/**
	 * @return True if the specified coordinate is inside the world height grid limits, false otherwise.
	 * @param k (x,y) coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_grid_limits(const koord &k) const { return is_within_grid_limits(k.x, k.y); }

	/**
	 * @return True if the specified coordinate is inside the world height grid limits, false otherwise.
	 * @param x X coordinate.
	 * @param y Y coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_grid_limits(uint16 x, uint16 y) const {
		return (x<=(unsigned)cached_grid_size.x && y<=(unsigned)cached_grid_size.y);
	}

	/**
	 * Returns the closest coordinate to outside_pos that is within the world
	 */
	koord get_closest_coordinate(koord outside_pos);

	/**
	 * @return grund an pos/height
	 * @note Inline because called very frequently!
	 * @author Hj. Malthaner
	 */
	inline grund_t *lookup(const koord3d &pos) const
	{
		const planquadrat_t *plan = access(pos.x, pos.y);
		return plan ? plan->get_boden_in_hoehe(pos.z) : NULL;
		//"boden in height" = floor in height (Google)
	}

	/**
	 * This function takes grid coordinates as a parameter and a desired height (koord3d).
	 * Will return the ground_t object that intersects with it in it's north corner if possible.
	 * If that tile doesn't exist, returns the one that intersects with it in other corner.
	 * @param pos Grid coordinates to check for, the z points to the desired height.
	 * @see lookup_kartenboden_gridcoords
	 * @see corner_to_operate
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup_gridcoords(const koord3d &pos) const
	{
		if ( ( pos.x != cached_grid_size.x )  &&  ( pos.y != cached_grid_size.y ) ){
			return lookup(koord3d(pos.x, pos.y, pos.z));
		}

		if ( ( pos.x == cached_grid_size.x )  &&  ( pos.y == cached_grid_size.y ) ) {
			return lookup(koord3d(pos.x-1, pos.y-1, pos.z));
		}
		else if ( pos.x == cached_grid_size.x ) {
			return lookup(koord3d(pos.x-1, pos.y, pos.z));
		}
		return lookup(koord3d(pos.x, pos.y-1, pos.z));
	}

	/**
	 * This function takes 2D grid coordinates as a parameter, will return the ground_t object that
	 * intersects with it in it's north corner if possible. If that tile doesn't exist, returns
	 * the one that intersects with it in other corner.
	 * @param pos Grid coordinates to check for.
	 * @see corner_to_operate
	 * @see lookup_gridcoords
	 * @return The requested tile, or the immediately adjacent in case of lower border.
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup_kartenboden_gridcoords(const koord &pos) const
	{
		if ( ( pos.x != cached_grid_size.x )  &&  ( pos.y != cached_grid_size.y ) ){
			return lookup_kartenboden(pos);
		}

		if ( ( pos.x == cached_grid_size.x )  &&  ( pos.y == cached_grid_size.y ) ) {
			return access(koord(pos.x-1, pos.y-1))->get_kartenboden();
		}
		else if ( pos.x == cached_grid_size.x ) {
			return access(koord(pos.x-1, pos.y))->get_kartenboden();
		}
		return access(koord(pos.x, pos.y-1))->get_kartenboden();
	}

	/**
	 * @return The corner that needs to be raised/lowered on the given coordinates.
	 * @param pos Grid coordinate to check.
	 * @note Inline because called very frequently!
	 * @note Will always return north-west except on border tiles.
	 * @pre pos has to be a valid grid coordinate, undefined otherwise.
	 */
	inline slope_t::type get_corner_to_operate(const koord &pos) const
	{
		// Normal tile
		if ( ( pos.x != cached_grid_size.x )  &&  ( pos.y != cached_grid_size.y ) ){
			return slope4_t::corner_NW;
		}
		// Border on south-east
		if ( is_within_limits(pos.x-1, pos.y) ) {
			return(slope4_t::corner_NE);
		}
		// Border on south-west
		if ( is_within_limits(pos.x, pos.y-1) ) {
			return(slope4_t::corner_SW);
		}
		// Border on south
		return (slope4_t::corner_SE);
	}


private:
	/**
	 * @return grund at the bottom (where house will be build)
	 * @note Inline because called very frequently! - nocheck for more speed
	 */
	inline grund_t *lookup_kartenboden_nocheck(const sint16 x, const sint16 y) const
	{
		return plan[x+y*cached_grid_size.x].get_kartenboden();
	}

	inline grund_t *lookup_kartenboden_nocheck(const koord &pos) const { return lookup_kartenboden_nocheck(pos.x, pos.y); }
public:
	/**
	 * @return grund at the bottom (where house will be build)
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup_kartenboden(const sint16 x, const sint16 y) const
	{
		return is_within_limits(x, y) ? plan[x+y*cached_grid_size.x].get_kartenboden() : NULL;
	}

	inline grund_t *lookup_kartenboden(const koord &pos) const { return lookup_kartenboden(pos.x, pos.y); }

	/**
	 * @return The natural slope at a position.
	 * @note Uses the corner height for the best slope.
	 * @author prissi
	 */
	uint8	recalc_natural_slope( const koord k, sint8 &new_height ) const;

	/**
	 * Returns the natural slope a a position using the grid.
	 * @note No checking, and only using the grind for calculation.
	 */
	uint8	calc_natural_slope( const koord k ) const;

	// Getter/setter methods for maintaining the industry density
	inline uint32 get_target_industry_density() const { return ((uint32)finance_history_month[0][WORLD_CITICENS] * (sint64)industry_density_proportion) / 1000000ll; }
	inline uint32 get_actual_industry_density() const { return actual_industry_density; }

	inline void decrease_actual_industry_density(uint32 value) { actual_industry_density -= value; }
	inline void increase_actual_industry_density(uint32 value) { actual_industry_density += value; }

	 /**
	  * Initialize map.
	  * @param sets Game settings.
	  * @param preselected_players Defines which players the user has selected before he started the game.
	  * @author Hj. Malthaner
	  */
	void init(settings_t*, sint8 const* heights);

	void init_tiles();

	void enlarge_map(settings_t const*, sint8 const* h_field);

	karte_t();

	~karte_t();

	/**
	 * Returns an index to a halt at koord k
	 * optionally limit to that owned by player player
	 * by default create a new halt if none found
	 * @note "create_halt"==true is used during loading old games
	 */
	halthandle_t get_halt_koord_index(koord k, player_t *player=NULL, bool create_halt=true);

	/**
	 * File version used when loading (or current if generated)
	 * @note Useful for finish_rd
	 */
	loadsave_t::combined_version load_version;

	/**
	 * Checks if the planquadrat (tile) at coordinate (x,y)
	 * can be lowered at the specified height.
	 * @author V. Meyer
	 */
	const char* can_lower_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const;

	/**
	 * Checks if the planquadrat (tile) at coordinate (x,y)
	 * can be raised at the specified height.
	 * @author V. Meyer
	 */
	const char* can_raise_plan_to(const player_t *player, sint16 x, sint16 y, sint8 h) const;

	/**
	 *Checks if the whole planquadrat (tile) at coordinates (x,y) height can
	 * be changed ( for example, water height can't be changed ).
	 * @author Hj. Malthaner
	 */
	bool is_plan_height_changeable(sint16 x, sint16 y) const;

	/**
	 * Increases the height of the grid coordinate (x, y) by one.
	 * @param pos Grid coordinate.
	 */
	int grid_raise(const player_t *player, koord pos, bool allow_deep_water, const char*&err);

	/**
	 * Decreases the height of the grid coordinate (x, y) by one.
	 * @param pos Grid coordinate.
	 */
	int grid_lower(const player_t *player, koord pos, const char*&err);

	// mostly used by AI: Ask to flatten a tile
	bool can_flatten_tile(player_t *player, koord k, sint8 hgt, bool keep_water=false, bool make_underwater_hill=false);
	bool flatten_tile(player_t *player, koord k, sint8 hgt, bool keep_water=false, bool make_underwater_hill=false, bool justcheck=false);

	/**
	 * Class to manage terraform operations.
	 * Can be used for raise only or lower only operations, but not mixed.
	 */
	class terraformer_t {
		/// Structure to save terraforming operations
		struct node_t {
			sint16 x;    ///< x-coordinate
			sint16 y;    ///< y-coordinate
			sint8  h[4]; ///< height of corners, order: sw se ne nw
			uint8  changed;

			node_t(sint16 x_, sint16 y_, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 c)
			: x(x_), y(y_), changed(c) { h[0]=hsw; h[1]=hse; h[2]=hne; h[3]=hnw; }

			node_t() : x(-1), y(-1), changed(0) {}

			/// compares position
			bool operator== (const node_t& a) const { return (a.x==x)  && (a.y==y); }

			/// compares position
			static bool comp(const node_t& a, const node_t& b);
		};

		vector_tpl<node_t> list; ///< list of affected tiles
		uint8 actual_flag;       ///< internal flag to iterate through list
		bool ready;              ///< internal flag to signal iteration ready
		karte_t* welt;

		void add_node(bool raise, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);
	public:
		terraformer_t(karte_t* w) { init(); welt = w; }

		void init() { list.clear(); actual_flag = 1; ready = false; }

		/**
		 * Add tile to be raised.
		 */
		void add_raise_node(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);

		/**
		 * Add tile to be lowered.
		 */
		void add_lower_node(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);

		/**
		 * Generate list of all tiles that will be affected.
		 */
		void iterate(bool raise);

		/// Check whether raise operation would succeed
		const char* can_raise_all(const player_t *player, bool allow_deep_water, bool keep_water=false) const;
		/// Check whether lower operation would succeed
		const char* can_lower_all(const player_t *player, bool allow_deep_water) const;

		/// Do the raise operations
		int raise_all();
		/// Do the lower operations
		int lower_all();
	};

private:
	/// Internal functions to be used with terraformer_t to propagate terrain changes to neighbouring tiles
	void prepare_raise(terraformer_t& digger, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);
	void prepare_lower(terraformer_t& digger, sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);

public:

	// the convois are also handled each step => thus we keep track of them too
	void add_convoi(convoihandle_t const &cnv);
	void rem_convoi(convoihandle_t const &cnv);
	vector_tpl<convoihandle_t> const& convoys() const { return convoi_array; }

	/**
	 * To access the cities array.
	 * @author Hj. Malthaner
	 */
	const weighted_vector_tpl<stadt_t*>& get_cities() const { return stadt; }
	stadt_t *get_town_at(const uint32 weight) { return stadt.at_weight(weight); }
	uint32 get_town_list_weight() const { return stadt.get_sum_weight(); }

	void add_city(stadt_t *s);

	/**
	 * Removes town from map, houses will be left overs.
	 * @author prissi
	 */
	bool remove_city(stadt_t *s);

	/* tourist attraction list */
	void add_attraction(gebaeude_t *gb);
	void remove_attraction(gebaeude_t *gb);
	const weighted_vector_tpl<gebaeude_t*> &get_ausflugsziele() const {return world_attractions; }

	void add_label(koord k) { if (!labels.is_contained(k)) labels.append(k); }
	void remove_label(koord k) { labels.remove(k); }
	const slist_tpl<koord>& get_label_list() const { return labels; }

	bool add_fab(fabrik_t *fab);
	bool rem_fab(fabrik_t *fab);

	int get_fab_index(fabrik_t* fab)  const { return fab_list.index_of(fab); }
	fabrik_t* get_fab(unsigned index) const { return index < fab_list.get_count() ? fab_list[index] : NULL; }
	const vector_tpl<fabrik_t*>& get_fab_list() const { return fab_list; }
	vector_tpl<fabrik_t*>& access_fab_list() { return fab_list; }

	/**
	 * Returns a list of goods produced by factories that exist in current game.
	 */
	const vector_tpl<const goods_desc_t*> &get_goods_list();

	/**
	 * Searches and returns the closest city
	 * but prefers even farther cities if within their city limits
	 * @author Hj. Malthaner
	 * New for January 2020: add the choice to select the rank. 1 is
	 * the best; 2 the second best, and so forth.
	 * @author: jamespetts
	 */
	stadt_t *find_nearest_city(koord k, uint32 rank = 1) const;

	// Returns the city at the position given.
	// Returns NULL if there is no city there.
	stadt_t * get_city(koord pos) const;

	bool cannot_save() const { return nosave; }
	void set_nosave() { nosave = true; nosave_warning = true; }
	void set_nosave_warning() { nosave_warning = true; }

	/// rotate plans by 90 degrees
	void rotate90_plans(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max);

	// rotate map view by 90 degrees
	void rotate90();

	class sync_list_t {
			friend class karte_t;
		public:
			sync_list_t() : currently_deleting(NULL), sync_step_running(false) {}
			void add(sync_steppable *obj);
			void remove(sync_steppable *obj);
		private:
			void sync_step(uint32 delta_t);
			/// clears list, does not delete the objects
			void clear();

			vector_tpl<sync_steppable *> list;  ///< list of sync-steppable objects
			sync_steppable* currently_deleting; ///< deleted durign sync_step, safeguard calls to remove
			bool sync_step_running;
	};

	sync_list_t sync;              ///< vehicles, transformers, traffic lights
	sync_list_t sync_eyecandy;     ///< animated buildings
	sync_list_t sync_way_eyecandy; ///< smoke

	/**
	 * Synchronous stepping of objects like vehicles.
	 */
	void sync_step(uint32 delta_t, bool sync, bool display );	// advance also the timer
	/**
	 * Tasks that are more time-consuming, like route search of vehicles and production of factories.
	 */
	void step();

//private:
	inline planquadrat_t *access_nocheck(int i, int j) const {
		return &plan[i + j*cached_grid_size.x];
	}

	inline planquadrat_t *access_nocheck(koord k) const { return access_nocheck(k.x, k.y); }

//public:
	inline planquadrat_t *access(int i, int j) const {
		return is_within_limits(i, j) ? &plan[i + j*cached_grid_size.x] : NULL;
	}

	inline planquadrat_t *access(koord k) const { return access(k.x, k.y); }

//private:
	/**
	 * @return Height at the grid point i, j - versions without checks for speed
	 * @author Hj. Malthaner
	 */
	inline sint8 lookup_hgt_nocheck(sint16 x, sint16 y) const {
		return grid_hgts[x + y*(cached_grid_size.x+1)];
	}

	inline sint8 lookup_hgt_nocheck(koord k) const { return lookup_hgt_nocheck(k.x, k.y); }

//public:
	/**
	 * @return Height at the grid point i, j
	 * @author Hj. Malthaner
	 */
	inline sint8 lookup_hgt(sint16 x, sint16 y) const {
		return is_within_grid_limits(x, y) ? grid_hgts[x + y*(cached_grid_size.x+1)] : groundwater;
	}

	inline sint8 lookup_hgt(koord k) const { return lookup_hgt(k.x, k.y); }

	/**
	 * Sets grid height.
	 * Never set grid_hgts manually, always use this method!
	 * @author Hj. Malthaner
	 */
	void set_grid_hgt(sint16 x, sint16 y, sint8 hgt) { grid_hgts[x + y*(uint32)(cached_grid_size.x+1)] = hgt; }

	inline void set_grid_hgt(koord k, sint8 hgt) { set_grid_hgt(k.x, k.y, hgt); }


private:
	/**
	 * @return water height - versions without checks for speed
	 * @author Kieron Green
	 */
	inline sint8 get_water_hgt_nocheck(sint16 x, sint16 y) const {
		return water_hgts[x + y * (cached_grid_size.x)];
	}

	inline sint8 get_water_hgt_nocheck(koord k) const { return get_water_hgt_nocheck(k.x, k.y); }

public:
	/**
	 * @return water height
	 * @author Kieron Green
	 */
	inline sint8 get_water_hgt(sint16 x, sint16 y) const {
		return is_within_limits( x, y ) ? water_hgts[x + y * (cached_grid_size.x)] : groundwater;
	}

	inline sint8 get_water_hgt(koord k) const { return get_water_hgt(k.x, k.y); }


	/**
	 * Sets water height.
	 * @author Kieron Green
	 */
	void set_water_hgt(sint16 x, sint16 y, sint8 hgt) { water_hgts[x + y * (cached_grid_size.x)] = (hgt); }

	inline void set_water_hgt(koord k, sint8 hgt) {  set_water_hgt(k.x, k.y, hgt); }

	/**
	 * Fills array with corner heights of neighbours
	 * @author Kieron Green
	 */
	void get_neighbour_heights(const koord k, sint8 neighbour_height[8][4]) const;

	/**
	 * Calculates appropriate climate for a tile
	 * @author Kieron Green
	 */
	void calc_climate(koord k, bool recalc);

	/**
	 * Rotates climate and water transitions for a tile
	 * @author Kieron Green
	 */
	void rotate_transitions(koord k);

	/**
	 * Recalculate climate and water transitions for a tile
	 * @author Kieron Green
	 */
	void recalc_transitions(koord k);

	/**
	 * Loop recalculating transitions - suitable for multithreading
	 * @author Kieron Green
	 */
	void recalc_transitions_loop(sint16, sint16, sint16, sint16);

	/**
	 * Loop creating grounds on all plans from height and water height - suitable for multithreading
	 * @author Kieron Green
	 */
	void create_grounds_loop(sint16, sint16, sint16, sint16);

	/**
	 * Loop cleans grounds so that they have correct boden and slope - suitable for multithreading
	 * @author Kieron Green
	 */
	void cleanup_grounds_loop(sint16, sint16, sint16, sint16);

private:
	/**
	 * @return Minimum height of the planquadrats (tile) at i, j. - for speed no checks performed that coordinates are valid
	 * @author Hj. Malthaner
	 */
	sint8 min_hgt_nocheck(koord k) const;

	/**
	 * @return Maximum height of the planquadrats (tile) at i, j. - for speed no checks performed that coordinates are valid
	 * @author Hj. Malthaner
	 */
	sint8 max_hgt_nocheck(koord k) const;

public:
	/**
	 * @return Minimum height of the planquadrats (tile) at i, j.
	 * @author Hj. Malthaner
	 */
	sint8 min_hgt(koord k) const;

	/**
	 * @return Maximum height of the planquadrats (tile) at i, j.
	 * @author Hj. Malthaner
	 */
	sint8 max_hgt(koord k) const;

	/**
	 * @return true, wenn Platz an Stelle pos mit Groesse dim Water ist
	 * @author V. Meyer
	 */
	bool is_water(koord k, koord dim) const;

	/**
	 * @return true, if square in place (i,j) with size w, h is constructible.
	 * @author Hj. Malthaner
	 */
	bool square_is_free(koord k, sint16 w, sint16 h, int *last_y, climate_bits cl, uint16 regions_allowed) const;

	/**
	 * @return A list of all buildable squares with size w, h.
	 * @note Only used for town creation at the moment.
	 * @author Hj. Malthaner
	 */
	slist_tpl<koord> * find_squares(sint16 w, sint16 h, climate_bits cl, uint16 regions_allowed, sint16 old_x, sint16 old_y) const;

	/**
	 * Plays the sound when the position is inside the visible region.
	 * The sound plays lower when the position is outside the visible region.
	 * @param pos Position at which the event took place.
	 * @param idx Index of the sound
	 * @author Hj. Malthaner
	 */
	bool play_sound_area_clipped(koord k, uint16 idx, waytype_t cooldown_type);

	void mute_sound( bool state ) { is_sound = !state; }

	/**
	 * Saves the map to a file.
	 * @param Filename name of the file to write.
	 * @author Hj. Malthaner
	 */
	void save(const char *filename, const loadsave_t::mode_t savemode, const char *version, const char *ex_version, const char* ex_revision, bool silent);

	/**
	 * Loads a map from a file.
	 * @param Filename name of the file to read.
	 * @author Hj. Malthaner
	 */
	bool load(const char *filename);

	/**
	 * Creates a map from a heightfield.
	 * @param sets game settings.
	 * @author Hj. Malthaner
	 */
	void load_heightfield(settings_t*);

	/**
	 * Stops simulation and optionally closes the game.
	 * @param exit_game If true, the game will also close.
	 */
	void stop(bool exit_game);

	/**
	 * Main loop with event handling.
	 * @return false to exit.
	 * @author Hj. Malthaner
	 */

	uint16 get_base_pathing_counter() const { return base_pathing_counter; }

	bool interactive(uint32 quit_month);

	uint32 get_sync_steps() const { return sync_steps; }

	/**
	 * Checks whether checklist is available, ie given sync_step is not too far into past.
	 */
	bool is_checklist_available(const uint32 sync_step) const { return sync_step + LAST_CHECKLISTS_COUNT > sync_steps; }
	const checklist_t& get_checklist_at(const uint32 sync_step) const { return LCHKLST(sync_step); }
	void set_checklist_at(const uint32 sync_step, const checklist_t &chklst) { LCHKLST(sync_step) = chklst; }

	const checklist_t& get_last_checklist() const { return LCHKLST(sync_steps); }
	uint32 get_last_checklist_sync_step() const { return sync_steps; }

	void clear_checklist_history();
	void clear_checklist_debug_sums();
	void clear_checklist_rands();
	void clear_all_checklists();

	void command_queue_append(network_world_command_t*) const;

	void clear_command_queue() const;

	void network_disconnect();

	sint32 get_citycar_speed_average() const { return citycar_speed_average; }

	void set_recheck_road_connexions() { recheck_road_connexions = true; }

	/**
	 * These methods return an estimated
	 * road speed based on the average
	 * speed of road traffic and the
	 * speed limit of the appropriate type
	 * of road. This is measured in 100ths
	 * of a minute per tile.
	 */
	uint32 get_generic_road_time_per_tile_city() const { return generic_road_time_per_tile_city; }
	uint32 get_generic_road_time_per_tile_intercity() const { return generic_road_time_per_tile_intercity; };

	sint32 calc_generic_road_time_per_tile(const way_desc_t* desc);

	uint32 get_max_road_check_depth() const { return max_road_check_depth; }

	sint64 calc_monthly_job_demand() const;

	/**
	 * To identify the current map.
	 */
	uint32 get_map_counter() const { return map_counter; }
	void set_map_counter(uint32 new_map_counter);
	/**
	 * Called by the server before sending the sync commands.
	 */
	uint32 generate_new_map_counter() const;

	/**
	 * Time printing routines.
	 * Should be inlined.
	 */
	inline void sprintf_time_secs(char *p, size_t size, uint32 seconds) const
	{
		unsigned int minutes = seconds / 60;
		unsigned int hours = minutes / 60;
		seconds %= 60;
		if(hours)
		{
			minutes %= 60;
#if defined(_WIN32) || defined (_M_X64)
			sprintf_s(p, size, "%u:%02u:%02u", hours, minutes, seconds);
#else
			snprintf(p, size, "%u:%02u:%02u", hours, minutes, seconds);
#endif
		}
		else
		{
#if defined(_WIN32) || defined (_M_X64)
			sprintf_s(p, size, "%u:%02u", minutes, seconds);
#else
			snprintf(p, size, "%u:%02u", minutes, seconds);
#endif
		}
	}

	inline void sprintf_ticks(char *p, size_t size, sint64 ticks) const
	{
		uint32 seconds = (uint32)ticks_to_seconds(ticks);
		sprintf_time_secs(p, size, seconds);
	}

	inline void sprintf_time_tenths(char* p, size_t size, uint32 tenths) const
	{
		sprintf_time_secs(p, size, 6 * tenths);
	}

	// @author: jamespetts
	void set_scale();

	void remove_queued_city(stadt_t* stadt);
	void add_queued_city(stadt_t* stadt);

	sint64 get_land_value(koord3d k);
	double get_forge_cost(waytype_t waytype, koord3d position);
	bool is_forge_cost_reduced(waytype_t waytype, koord3d position);

	inline void add_time_interval_signal_to_check(signal_t* sig) { time_interval_signals_to_check.append_unique(sig); }
	inline bool remove_time_interval_signal_to_check(signal_t* sig) { return time_interval_signals_to_check.remove(sig); }

private:

	void calc_generic_road_time_per_tile_city() { generic_road_time_per_tile_city = calc_generic_road_time_per_tile(city_road); }
	void calc_generic_road_time_per_tile_intercity();
	void calc_max_road_check_depth();

	void process_network_commands(sint32* ms_difference);
	void do_network_world_command(network_world_command_t *nwc);
	uint32 get_next_command_step();

	void get_nearby_halts_of_tiles(const minivec_tpl<const planquadrat_t*> &tile_list, const goods_desc_t * wtyp, vector_tpl<nearby_halt_t> &halts) const;
};


/**
 * Returns pointer to single instance of the world.
 */
inline karte_t *world()
{
	return karte_t::world;
}


/**
 * Class to access the pointer to the world.
 * No need to initialize it.
 */
class karte_ptr_t
{
public:
	/// dereference operator: karte_ptr_t can be used as it would be karte_t*
	karte_t& operator*() {
		assert( karte_t::world );
		return *karte_t::world;
	}
	const karte_t& operator*() const {
		assert( karte_t::world );
		return *karte_t::world;
	}

	karte_ptr_t() {}
	/// dereference operator: karte_ptr_t can be used as it would be karte_t*
	karte_t* operator->() {
		assert( karte_t::world );
		return karte_t::world;
	}
	const karte_t* operator->() const {
		assert( karte_t::world );
		return karte_t::world;
	}

	/// cast to karte_t*
	operator karte_t* () const { return karte_t::world; }
private:
	karte_ptr_t(const karte_ptr_t&);
	karte_ptr_t& operator=(const karte_ptr_t&);
	// no cast to bool please
	operator bool () const;
};

#endif

