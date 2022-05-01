/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef WORLD_SIMWORLD_H
#define WORLD_SIMWORLD_H


#include "../simconst.h"
#include "../simtypes.h"
#include "../simunits.h"

#include "../convoihandle.h"
#include "../halthandle.h"

#include "../tpl/weighted_vector_tpl.h"
#include "../tpl/array2d_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/slist_tpl.h"

#include "../dataobj/settings.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/rect.h"

#include "../utils/checklist.h"
#include "../utils/sha1_hash.h"

#include "simplan.h"
#include "surface.h"

#include "../simdebug.h"


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
class network_world_command_t;
class goods_desc_t;
class memory_rw_t;
class viewport_t;
class records_t;
class loadingscreen_t;
class terraformer_t;


/**
 * Threaded function caller.
 */
typedef void (karte_t::*xy_loop_func)(sint16, sint16, sint16, sint16);


/**
 * The map is the central part of the simulation. It stores all data and objects.
 * @brief Stores all data and objects of the simulated world.
 */
class karte_t : public surface_t
{
	friend karte_t* world();  // to access the single instance
	friend class karte_ptr_t; // to access the single instance

	static karte_t* world; ///< static single instance

public:
	/**
	 * Height of a point of the map with "perlin noise".
	 * Uses map roughness and mountain height from @p sets.
	 */
	static sint32 perlin_hoehe(settings_t const *sets, koord k, koord size);

	/**
	 * Loops over tiles setting heights from perlin noise
	 */
	void perlin_hoehe_loop(sint16, sint16, sint16, sint16);

	enum player_cost {
		WORLD_CITIZENS = 0,      ///< total people
		WORLD_GROWTH,            ///< growth (just for convenience)
		WORLD_TOWNS,             ///< number of all cities
		WORLD_FACTORIES,         ///< number of all consuming only factories
		WORLD_CONVOIS,           ///< total number of convois
		WORLD_CITYCARS,          ///< number of citycars generated
		WORLD_PAS_RATIO,         ///< percentage of passengers that started successful
		WORLD_PAS_GENERATED,     ///< total number generated
		WORLD_MAIL_RATIO,        ///< percentage of mail that started successful
		WORLD_MAIL_GENERATED,    ///< all letters generated
		WORLD_GOODS_RATIO,       ///< ratio of chain completeness
		WORLD_TRANSPORTED_GOODS, ///< all transported goods
		MAX_WORLD_COST
	};

	#define MAX_WORLD_HISTORY_YEARS   (12) // number of years to keep history
	#define MAX_WORLD_HISTORY_MONTHS  (12) // number of months to keep history

	enum {
		NORMAL       = 0,
		PAUSE_FLAG   = 1 << 0,
		FAST_FORWARD = 1 << 1,
		FIX_RATIO    = 1 << 2
	};

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

	/** @} */

	/**
	 * All cursor interaction goes via this function, it will call save_mouse_funk first with
	 * init, then with the position and with exit, when another tool is selected without click
	 * @see tool/simtool.cc for practical examples of such functions.
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
	 * Changes the season and/or snowline height
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
	 * Array containing the convois.
	 */
	vector_tpl<convoihandle_t> convoi_array;

	/**
	 * Array containing the factories.
	 */
	slist_tpl<fabrik_t *> fab_list;

	/**
	 * Stores a list of goods produced by factories currently in the game;
	 */
	vector_tpl<const goods_desc_t*> goods_in_game;

	weighted_vector_tpl<gebaeude_t *> attractions;

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
	 * World record speed manager.
	 * Keeps track of the fastest vehicles in game.
	 */
	records_t *records;

	/**
	 * Attached view to this world.
	 */
	main_view_t *view;

	/**
	 * Event manager of this world.
	 */
	interaction_t *eventmanager;

private:
	/**
	 * The fractal generation of the map is not perfect.
	 * cleanup_karte() eliminates errors.
	 */
	void cleanup_karte( int xoff, int yoff );

	/**
	 * @name Player management
	 *       Variables related to the player management in game.
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

	/**
	 * Counter for schedules.
	 * If a new schedule is active, this counter will increment
	 * stations check this counter and will reroute their goods if changed.
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
	 */
	uint32 ticks;

	/**
	 * Ticks counter at last steps.
	 * @note The time is in ms (milliseconds)
	 */
	uint32 last_step_ticks;

	/**
	 * From now on is next month.
	 * @note The time is in ms (milliseconds)
	 */
	uint32 next_month_ticks;

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
	/// @note variable used in interactive()
	uint8  network_frame_count;
	/**
	 * @note Variable used in interactive().
	 * @note Set in reset_timer().
	 */
	sint32 fix_ratio_frame_time;

	/**
	 * For performance comparison.
	 */
	uint32 realFPS;

	/**
	 * For performance comparison.
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

	/**
	* ms, when the next check if we need to start next song
	*/
	uint32 next_midi_time;

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

	/**
	 * What game objectives.
	 */
	scenario_t *scenario;

	/**
	 * Holds all the text messages in the messagebox (chat, new vehicle etc).
	 */
	message_t *msg;

	/**
	 * Array indexed per way type. Used to determine the speedbonus.
	 */
	sint32 average_speed[8];

	/**
	 * Used to distribute the workload when changing seasons to several steps.
	 */
	uint32 tile_counter;

	/**
	 * To identify different stages of the same game.
	 */
	uint32 map_counter;

	/**
	 * Recalculated speed bonus for different vehicles.
	 */
	void recalc_average_speed();

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
	 */
	void save(loadsave_t *file, bool silent);
public:
	/**
	 * Internal loading method.
	 */
	void load(loadsave_t *file);
private:
	void rdwr_gamestate(loadsave_t *file, loadingscreen_t *ls);

	/**
	 * Removes all objects, deletes all data structures and frees all accessible memory.
	 */
	void destroy();

	/**
	 * Restores history for older savegames.
	 */
	void restore_history(bool restore_transported_only);

	/**
	 * Will create rivers.
	 */
	void create_rivers(sint16 number);

	/**
	 * Will create lakes (multithreaded).
	 */
	void create_lakes_loop(sint16, sint16, sint16, sint16);

	/**
	 * Will create lakes.
	 */
	void create_lakes( int xoff, int yoff, sint8 max_lake_height );

	/**
	 * Will create beaches.
	 */
	void create_beaches( int xoff, int yoff );

	/**
	 * Distribute groundobjs and cities on the map but not
	 * in the rectangle from (0,0) till (old_x, old_y).
	 * It's now an extra function so we don't need the code twice.
	 */
	void distribute_cities(int new_cities, sint32 new_mean_citizen_count, sint16 old_x, sint16 old_y );
	void distribute_groundobjs(sint16 old_x, sint16 old_y);
	void distribute_movingobjs(sint16 old_x, sint16 old_y);

	/**
	 * The last time when a server announce was performed (in ms).
	 */
	uint32 server_last_announce_time;

	enum {
		SYNCX_FLAG = 1 << 0,
		GRIDS_FLAG = 1 << 1
	};

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

	bool can_flood_to_depth(koord k, sint8 new_water_height, sint8 *stage, sint8 *our_stage, sint16, sint16, sint16, sint16) const;

	void flood_to_depth(sint8 new_water_height, sint8 *stage);

public:
	enum server_announce_type_t
	{
		SERVER_ANNOUNCE_HELLO     = 0, ///< my server is now up
		SERVER_ANNOUNCE_HEARTBEAT = 1, ///< my server is still up
		SERVER_ANNOUNCE_GOODBYE   = 2, ///< my server is now down
	};

	/**
	 * Announce server and current state to listserver.
	 * @param status Specifies what information should be announced
	 * or offline (the latter only in cases where it is shutting down)
	 */
	void announce_server(server_announce_type_t status);

	/**
	 * Returns the messagebox message container.
	 */
	message_t *get_message() { return msg; }

	/**
	 * Set to something useful, if there is a total distance != 0 to show in the bar below.
	 */
	koord3d show_distance;

	/**
	 * Absolute month.
	 */
	inline uint32 get_last_month() const { return last_month; }

	/**
	 * Returns last year.
	 */
	inline sint32 get_last_year() const { return last_year; }

	/**
	 * dirty: redraw whole screen.
	 */
	void set_dirty() {dirty=true;}

	/**
	 * dirty: redraw whole screen.
	 */
	void unset_dirty() {dirty=false;}

	/**
	 * dirty: redraw whole screen.
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

	// do the internal accounting
	void buche(sint64 betrag, player_cost type);

	// calculates the various entries
	void update_history();

	scenario_t *get_scenario() const { return scenario; }
	void set_scenario(scenario_t *s);

	/**
	 * Returns the finance history for player.
	 */
	sint64 get_finance_history_year(int year, int type) const { return finance_history_year[year][type]; }

	/**
	 * Returns the finance history for player.
	 */
	sint64 get_finance_history_month(int month, int type) const { return finance_history_month[month][type]; }

	/**
	 * Returns pointer to finance history for player.
	 */
	const sint64* get_finance_history_year() const { return *finance_history_year; }

	/**
	 * Returns pointer to finance history for player.
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

	/// returns current speed bonus
	sint32 get_average_speed(waytype_t typ) const { return average_speed[ (typ==16 ? 3 : (int)(typ-1)&7 ) ]; }

	/// speed record management
	sint32 get_record_speed( waytype_t w ) const;
	void notify_record( convoihandle_t cnv, sint32 max_speed, koord k );

	/// time lapse mode ...
	bool is_paused() const { return step_mode&PAUSE_FLAG; }
	/// stops the game with interaction
	void set_pause( bool );

	bool is_fast_forward() const { return step_mode == FAST_FORWARD; }
	void set_fast_forward(bool ff);

	bool is_step_mode_normal() const { return step_mode == NORMAL; }

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

	enum change_player_tool_cmds {
		new_player           = 1,
		toggle_freeplay      = 2,
		delete_player        = 3,
		toggle_player_active = 4
	};

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
	 */
	uint32 ticks_per_world_month_shift;

	/**
	 * Number of ticks per MONTH!
	 */
	uint32 ticks_per_world_month;

	void set_ticks_per_world_month_shift(uint32 bits) {ticks_per_world_month_shift = bits; ticks_per_world_month = (1 << ticks_per_world_month_shift); }

	/**
	 * Converts speed (yards per tick) into tiles per month
	 *       * A derived quantity:
	 * speed * ticks_per_world_month / yards_per_tile
	 * == speed << ticks_per_world_month_shift  (pak-set dependent, 18 in old games)
	 *          >> yards_per_tile_shift (which is 12 + 8 = 20, see above)
	 * This is hard to do with full generality, because shift operators
	 * take positive numbers!
	 */
	uint32 speed_to_tiles_per_month(uint32 speed) const
	{
		const int left_shift = ticks_per_world_month_shift - YARDS_PER_TILE_SHIFT;
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
	 */
	sint64 scale_with_month_length(sint64 value)
	{
		const int left_shift = ticks_per_world_month_shift - 18;
		if(left_shift >= 0) {
			return value << left_shift;
		}
		else {
			return value >> (-left_shift);
		}
	}

	/**
	 * Scales value inverse proportionally with month length.
	 * Used to scale monthly maintenance costs, city growth, and factory production.
	 * @returns value << ( 18 - ticks_per_world_month_shift )
	 */
	sint64 inverse_scale_with_month_length(sint64 value)
	{
		const int left_shift = (int)ticks_per_world_month_shift-18;
		if(  left_shift < 0  ) {
			return value << (-left_shift);
		}
		else {
			return value >> left_shift;
		}
	}

	sint32 get_time_multiplier() const;
	void change_time_multiplier( sint32 delta );

	/**
	 * @return 0=winter, 1=spring, 2=summer, 3=autumn
	 * Here instead of in surface_t because this is required for the script API
	 */
	uint8 get_season() const { return season; }

	/**
	 * Time since map creation or the last load in ms.
	 */
	uint32 get_ticks() const { return ticks; }


	uint32 get_next_month_ticks() const { return next_month_ticks; }

	/**
	 * Absolute month (count start year zero).
	 */
	uint32 get_current_month() const { return current_month; }

	/**
	 * current city road.
	 * @note May change due to timeline.
	 */
	const way_desc_t* get_city_road() const { return city_road; }

	/**
	 * Number of steps elapsed since the map was generated.
	 */
	sint32 get_steps() const { return steps; }

	/**
	 * Idle time. Nur zur Anzeige verwenden!
	 */
	uint32 get_idle_time() const { return idle_time; }

	/**
	 * Number of frames displayed in the last real time second.
	 */
	uint32 get_realFPS() const { return realFPS; }

	/**
	 * Number of simulation loops in the last second. Can be very inaccurate!
	 */
	uint32 get_simloops() const { return simloops; }

	/**
	 * Returns the current snowline height.
	 */
	sint8 get_snowline() const { return snowline; }

	void set_mouse_rest_time(uint32 new_val) { mouse_rest_time = new_val; }
	void set_sound_wait_time(uint32 new_val) { sound_wait_time = new_val; }

public:
	/**
	 * Set a new tool as current: calls local_set_tool or sends to server.
	 */
	void set_tool( tool_t *tool_in, player_t* player );

	/**
	 * Set a new tool on our client, calls init.
	 */
	void local_set_tool( tool_t *tool_in, player_t* player );
	tool_t *get_tool(uint8 nr) const { return selected_tool[nr]; }

	/**
	 * Calls the work method of the tool.
	 * Takes network and scenarios into account.
	 */
	const char* call_work(tool_t *t, player_t *pl, koord3d pos, bool &suspended);

	 /**
	  * Initialize map.
	  * @param sets Game settings.
	  */
	void init(settings_t *sets, sint8 const* heights);

	void init_tiles();

	void enlarge_map(settings_t const*, sint8 const* h_field);

	karte_t();

	~karte_t();

	/**
	 * File version used when loading (or current if generated)
	 * @note Useful for finish_rd
	 */
	uint32 load_version;

public:
	// the convois are also handled each step => thus we keep track of them too
	void add_convoi(convoihandle_t);
	void rem_convoi(convoihandle_t);
	vector_tpl<convoihandle_t> const& convoys() const { return convoi_array; }

	/**
	 * To access the cities array.
	 */
	const weighted_vector_tpl<stadt_t*>& get_cities() const { return stadt; }
	void add_city(stadt_t *s);

	/**
	 * Removes town from map, houses will be left overs.
	 */
	bool remove_city(stadt_t *s);

	/* tourist attraction list */
	void add_attraction(gebaeude_t *gb);
	void remove_attraction(gebaeude_t *gb);
	const weighted_vector_tpl<gebaeude_t*> &get_attractions() const {return attractions; }

	void add_label(koord k) { if (!labels.is_contained(k)) labels.append(k); }
	void remove_label(koord k) { labels.remove(k); }
	const slist_tpl<koord>& get_label_list() const { return labels; }

	bool add_fab(fabrik_t *fab);
	bool rem_fab(fabrik_t *fab);
	int get_fab_index(fabrik_t* fab)  const { return fab_list.index_of(fab); }
	fabrik_t* get_fab(unsigned index) const { return index < fab_list.get_count() ? fab_list.at(index) : NULL; }
	const slist_tpl<fabrik_t*>& get_fab_list() const { return fab_list; }

	/**
	 * Returns a list of goods produced by factories that exist in current game.
	 */
	const vector_tpl<const goods_desc_t*> &get_goods_list();

	/**
	 * Searches and returns the closest city
	 * but prefers even farther cities if within their city limits
	 */
	stadt_t *find_nearest_city(koord k) const;

	bool cannot_save() const { return nosave; }
	void set_nosave() { nosave = true; nosave_warning = true; }
	void set_nosave_warning() { nosave_warning = true; }

	/// rotate plans by 90 degrees
	void rotate90_plans(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max);

	/// rotate map view by 90 degrees
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
	void sync_step(uint32 delta_t); // advance also the timer

	void display(uint32 delta_t);

	/**
	 * Tasks that are more time-consuming, like route search of vehicles and production of factories.
	 */
	void step();

public:
	/**
	* Calculates appropriate climate for a region using elliptic areas for each
	*/
	void calc_climate_map_region( sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom );

	/**
	* Calculates appropriate climate for a region using elliptic areas for each
	*/
	void calc_humidity_map_region( sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom );

	/**
	 * assign climated from the climate map to a region
	 */
	void assign_climate_map_region( sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom );

	/**
	 * Since the trees follow humidity, we have to redistribute them only in the new region
	 */
	void distribute_trees_region( sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom );

	/**
	 * Loop recalculating transitions - suitable for multithreading
	 */
	void recalc_transitions_loop(sint16, sint16, sint16, sint16);

	/**
	 * Loop cleans grounds so that they have correct boden and slope - suitable for multithreading
	 */
	void cleanup_grounds_loop(sint16, sint16, sint16, sint16);

	/**
	 * @return A list of all buildable squares with size w, h.
	 * @note Only used for town creation at the moment.
	 */
	slist_tpl<koord> * find_squares(sint16 w, sint16 h, climate_bits cl, sint16 old_x, sint16 old_y) const;

	/**
	 * Plays the sound when the position is inside the visible region.
	 * The sound plays lower when the position is outside the visible region.
	 * @param k Position at which the event took place.
	 * @param idx Index of the sound
	 * @param t is the type of sound (for selective muting etc.)
	 */
	bool play_sound_area_clipped(koord k, uint16 idx, sound_type_t t) const;

	void mute_sound( bool state ) { is_sound = !state; }

	/* if start is true, the current map will be used as servergame
	 * Does not announce a new map!
	 */
	void switch_server( bool start_server, bool port_forwarding );

	/**
	 * Saves the map to a file.
	 * @param filename name of the file to write.
	 */
	void save(const char *filename, bool autosave, const char *version, bool silent);

	/**
	 * Loads a map from a file.
	 * @param filename name of the file to read.
	 */
	bool load(const char *filename);

	/**
	 * Creates a map from a heightfield.
	 * @param sets game settings.
	 */
	void load_heightfield(settings_t *sets);

	/**
	 * Stops simulation and optionally closes the game.
	 * @param exit_game If true, the game will also close.
	 */
	void stop(bool exit_game);

	/**
	 * Main loop with event handling.
	 * @return false to exit.
	 */
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

	void command_queue_append(network_world_command_t*) const;

	void clear_command_queue() const;

	void network_disconnect();

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
	 * Generates hash of game state by streaming a save to a hash function
	 */
	uint32 get_gamestate_hash();

private:
	void process_network_commands(sint32* ms_difference);
	void do_network_world_command(network_world_command_t *nwc);
	uint32 get_next_command_step();
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
