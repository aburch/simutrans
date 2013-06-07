/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * zentrale Datenstruktur von Simutrans
 * von Hj. Malthaner 1998
 */

#ifndef simworld_h
#define simworld_h

#include "simconst.h"
#include "simtypes.h"
#include "simunits.h"

#include "convoihandle_t.h"
#include "halthandle_t.h"

#include "tpl/weighted_vector_tpl.h"
#include "tpl/ptrhashtable_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/slist_tpl.h"

#include "dataobj/marker.h"
#include "dataobj/einstellungen.h"
#include "dataobj/pwd_hash.h"
#include "dataobj/loadsave.h"

#include "simplan.h"

#include "simdebug.h"

struct event_t;
struct sound_info;
class stadt_t;
class fabrik_t;
class gebaeude_t;
class zeiger_t;
class grund_t;
class planquadrat_t;
class karte_ansicht_t;
class sync_steppable;
class werkzeug_t;
class scenario_t;
class message_t;
class weg_besch_t;
class network_world_command_t;
class ware_besch_t;
class memory_rw_t;


struct checklist_t
{
	uint32 random_seed;
	uint16 halt_entry;
	uint16 line_entry;
	uint16 convoy_entry;

	checklist_t() : random_seed(0), halt_entry(0), line_entry(0), convoy_entry(0) { }
	checklist_t(uint32 _random_seed, uint16 _halt_entry, uint16 _line_entry, uint16 _convoy_entry)
		: random_seed(_random_seed), halt_entry(_halt_entry), line_entry(_line_entry), convoy_entry(_convoy_entry) { }

	bool operator == (const checklist_t &other) const
	{
		return ( random_seed==other.random_seed && halt_entry==other.halt_entry && line_entry==other.line_entry && convoy_entry==other.convoy_entry );
	}
	bool operator != (const checklist_t &other) const { return !( (*this)==other ); }

	void rdwr(memory_rw_t *buffer);
	int print(char *buffer, const char *entity) const;
};


/**
 * Threaded function caller.
 */
typedef void (karte_t::*xy_loop_func)(sint16, sint16, sint16, sint16);


/**
 * The map is the central part of the simulation. It stores all data and objects.
 * @brief Stores all data and objects of the simulated world.
 * @author Hj. Malthaner
 */
class karte_t
{
public:
	/**
	 * Height of a point of the map with "perlin noise"
	 *
	 * @param frequency in 0..1.0 roughness, the higher the rougher
	 * @param amplitude in 0..160.0 top height of mountains, may not exceed 160.0!!!
	 * @author Hj. Malthaner
	 */
	static sint32 perlin_hoehe(settings_t const*, koord pos, koord size);

	enum player_cost {
		WORLD_CITICENS=0,		//!< total people
		WORLD_GROWTH,			//!< growth (just for convenience)
		WORLD_TOWNS,			//!< number of all cities
		WORLD_FACTORIES,		//!< number of all consuming only factories
		WORLD_CONVOIS,			//!< total number of convois
		WORLD_CITYCARS,			//!< number of citycars generated
		WORLD_PAS_RATIO,		//!< percentage of passengers that started successful
		WORLD_PAS_GENERATED,	//!< total number generated
		WORLD_MAIL_RATIO,		//!< percentage of mail that started successful
		WORLD_MAIL_GENERATED,	//!< all letters generated
		WORLD_GOODS_RATIO,		//!< ratio of chain completeness
		WORLD_TRANSPORTED_GOODS,//!< all transported goods
		MAX_WORLD_COST
	};

	#define MAX_WORLD_HISTORY_YEARS  (12) // number of years to keep history
	#define MAX_WORLD_HISTORY_MONTHS  (12) // number of months to keep history

	enum { NORMAL=0, PAUSE_FLAG = 0x01, FAST_FORWARD=0x02, FIX_RATIO=0x04 };

	/**
	 * Missing things during loading:
	 * factories, vehicles, roadsigns or catenary may be severe
	 */
	enum missing_level_t { NOT_MISSING=0, MISSING_FACTORY=1, MISSING_VEHICLE=2, MISSING_SIGN=3, MISSING_WAYOBJ=4, MISSING_ERROR=4, MISSING_BRIDGE, MISSING_BUILDING, MISSING_WAY };

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
	 * For performance reasons we have the map grid size cached locally, comes from the enviroment (Einstellungen)
	 * @brief Cached map grid size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 */
	koord cached_grid_size;

	/**
	 * For performance reasons we have the map size cached locally, comes from the enviroment (Einstellungen).
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

	/**
	 * @}
	 */

	/**
	 * All cursor interaction goes via this function, it will call save_mouse_funk first with
	 * init, then with the position and with exit, when another tool is selected without click
	 * @see simwerkz.cc for practical examples of such functions.
	 */
	werkzeug_t *werkzeug[MAX_PLAYER_COUNT];

	/**
	 * Redraw whole map.
	 */
	bool dirty;

	/*
	 * Redraw background.
	 */
	bool background_dirty;

	/**
	 * The rotation of the map when first loaded.
	 */
	uint8 loaded_rotation;

	/**
	 * @name Camera position
	 *       This variables are related to the view camera position.
	 * @{
	 */

	sint16 x_off; //!< Fine scrolling x offset.
	sint16 y_off; //!< Fine scrolling y offset.

	koord ij_off; //!< Current view position.

	koord view_ij_off; //!< This is the current offset for getting from tile to screen.

	/**
	 * @}
	 */

	/**
	 * @name Mouse pointer and cursor management
	 * @author Hj. Malthaner
	 * @{
	 */
	sint32 mi; //!< Mouse position, i coordinate.
	sint32 mj; //!< Mouse position, j coordinate.

	/**
	 * @brief Map mouse cursor tool.
	 */
	zeiger_t *zeiger;

	/**
	 * @}
	 */

	/**
	 * Time when last mouse moved to check for ambient sound events.
	 */
	uint32 mouse_rest_time;

	/**
	 * Waiting time before next event.
	 */
	uint32 sound_wait_time;

	/**
	 * If this is true, the map will not be scrolled on right-drag.
	 * @author Hj. Malthaner
	 */
	bool scroll_lock;

	/**
	 * If true, this map cannot be saved.
	 */
	bool nosave;
	bool nosave_warning;

	/*
	 * The current convoi to follow.
	 * @author prissi
	 */
	convoihandle_t follow_convoi;

	/**
	 * Water level height.
	 * @author Hj. Malthaner
	 */
	sint8 grundwasser;

	/**
	 * Current snow height.
	 * @note Might change during the game.
	 * @author prissi
	 */
	sint16 snowline;

	/**
	 * Changes the snowline height (for the seasons).
	 * @return true if a change is needed.
	 * @author prissi
	 */
	bool recalc_snowline();

	/**
	 * >0 means a season change is needed
	 */
	int pending_season_change;

	/**
	 * Recalculates sleep time etc.
	 */
	void update_frame_sleep_time(long delta_t);

	/**
	 * Table for fast conversion from height to climate.
	 * @author prissi
	 */
	uint8 height_to_climate[32];

	/**
	 * These objects will be added to the sync_list (but before next sync step, so they do not interfere!)
	 */
	slist_tpl<sync_steppable *> sync_add_list;

   /**
	 * These objects will be removed from the sync_list (but before next sync step, so they do not interfere!)
	 */
	slist_tpl<sync_steppable *> sync_remove_list;


	/**
	 * Sync list.
	 */
#ifndef SYNC_VECTOR
	slist_tpl<sync_steppable *> sync_list;
#else
	vector_tpl<sync_steppable *> sync_list;
#endif

   /**
	 * These objects will be added to the eyecandy sync_list (but before next sync step, so they do not interfere!)
	 */
	slist_tpl<sync_steppable *> sync_eyecandy_add_list;

	/**
	 * These objects will be removed to the eyecandy sync_list (but before next sync step, so they do not interfere!)
	 */
	slist_tpl<sync_steppable *> sync_eyecandy_remove_list;

	/**
	 * Sync list for eyecandy objects.
	 */
	ptrhashtable_tpl<sync_steppable *,sync_steppable *> sync_eyecandy_list;

	/**
	 * These objects will be added to the eyecandy way objects (smoke) sync_list (but before next sync step, so they do not interfere!)
	 */
	slist_tpl<sync_steppable *> sync_way_eyecandy_add_list;

	/**
	 * These objects will be removed to the eyecandy way objects (smoke) sync_list (but before next sync step, so they do not interfere!)
	 */
	slist_tpl<sync_steppable *> sync_way_eyecandy_remove_list;

	/**
	 * Sync list for eyecandy way objects (smoke).
	 */
#ifndef SYNC_VECTOR
	slist_tpl<sync_steppable *> sync_way_eyecandy_list;
#else
	vector_tpl<sync_steppable *> sync_way_eyecandy_list;
#endif

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
	vector_tpl<const ware_besch_t*> goods_in_game;

	weighted_vector_tpl<gebaeude_t *> ausflugsziele;

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
	 * @name World record speed management
	 *       These variables keep track of the fastest vehicles in game.
	 * @{
	 */
	/**
	 * Class representing a word speed record.
	 */
	class speed_record_t {
	public:
		convoihandle_t cnv;
		sint32	speed;
		koord	pos;
		spieler_t *besitzer;
		sint32 year_month;

		speed_record_t() : cnv(), speed(0), pos(koord::invalid), besitzer(NULL), year_month(0) {}
	};

	/// World rail speed record
	speed_record_t max_rail_speed;
	/// World monorail speed record
	speed_record_t max_monorail_speed;
	/// World maglev speed record
	speed_record_t max_maglev_speed;
	/// World narrowgauge speed record
	speed_record_t max_narrowgauge_speed;
	/// World road speed record
	speed_record_t max_road_speed;
	/// World ship speed record
	speed_record_t max_ship_speed;
	/// World air speed record
	speed_record_t max_air_speed;

	/**
	 * @}
	 */

	/**
	 * Attached view to this world.
	 */
	karte_ansicht_t *view;

	/**
	 * Checks whether the heights of the corners of the tile at (@p x, @p y) can be raised.
	 * If the desired height of a corner is lower than its current height, this corner is ignored.
	 * @param x coordinate
	 * @param y coordinate
	 * @param keep_water returns false if water tiles would be raised above water
	 * @param hsw desired height of sw-corner
	 * @param hse desired height of se-corner
	 * @param hse desired height of ne-corner
	 * @param hnw desired height of nw-corner
	 * @param ctest which directions should be recursively checked (ribi_t style bitmap)
	 * @returns whether raise_to operation can be performed
	 */
	bool can_raise_to(sint16 x, sint16 y, bool keep_water, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 ctest=15) const;

	/**
	 * Raises heights of the corners of the tile at (@p x, @p y).
	 * New heights for each corner given.
	 * @pre can_raise_to should be called before this method.
	 * @see can_raise_to
	 * @returns count of full raise operations (4 corners raised one level)
	 * @note Clear tile, reset water/land type, calc reliefkarte pixel.
	 */
	int  raise_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw);

	/**
	 * Checks whether the heights of the corners of the tile at (@p x, @p y) can be lowered.
	 * If the desired height of a corner is higher than its current height, this corner is ignored.
	 * @param x coordinate
	 * @param y coordinate
	 * @param hsw desired height of sw-corner
	 * @param hse desired height of se-corner
	 * @param hse desired height of ne-corner
	 * @param hnw desired height of nw-corner
	 * @param ctest which directions should be recursively checked (ribi_t style bitmap)
	 * @returns whether lower_to operation can be performed
	 */
	bool can_lower_to(sint16 x, sint16 y, sint8 hsw, sint8 hse, sint8 hne, sint8 hnw, uint8 ctest=15) const;

	/**
	 * Lowers heights of the corners of the tile at (@p x, @p y).
	 * New heights for each corner given.
	 * @pre can_lower_to should be called before this method.
	 * @see can_lower_to
	 * @returns count of full lower operations (4 corners lowered one level)
	 * @note Clear tile, reset water/land type, calc reliefkarte pixel.
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
	 * Processes a mouse event that's moving the camera.
	 */
	void move_view(event_t *ev);

	/**
	 * Gets a new world position to put the cursor, under the mouse position.
	 * @param ev system event, we take the mouse position from here. Input parameter.
	 * @param grid_coordinates indicates if this function is to check against the map tiles, or the grid of heights. Input parameter.
	 * @return koord3d::invalid if no position exists under the mouse pointer, a 3d koord directly under the mouse otherwise.
	 */
	koord3d get_new_cursor_position(const event_t *ev, bool grid_coordinates);

	/**
	 * Processes a cursor movement event, related to the tool pointer in-map.
	 * @see zeiger_t
	 */
	void move_cursor(const event_t *ev);

	/**
	 * Searches for the ground_t that's under the requested screen position.
	 * @param screen_pos Screen coordinates to check for.
	 * @param intersect_grid Special case for the lower/raise tool, will return a limit border tile if we are on the south/east border of screen.
	 * @param found_i If we have a match, it will be set to the i coordinate of the found tile. Undefined otherwise.
	 * @param found_j If we have a match, it will be set to the j coordinate of the found tile. Undefined otherwise.
	 * @return the grund_t that's under the desired screen coordinate. NULL if we are outside map or we can't find it.
	 */
	grund_t* get_ground_on_screen_coordinate(const koord screen_pos, sint32 &found_i, sint32 &found_j, const bool intersect_grid=false) const;

	/**
	 * Processes a user event on the map, like a keyclick, or a mouse event.
	 */
	void interactive_event(event_t &ev);

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

	/**
	 * @}
	 */

	/**
	 * ??
	 */
	marker_t marker;

	/**
	 * @name Player management
	 *       Varables related to the player management in game.
	 * @author Hj. Malthaner
	 * @{
	 */
	/**
	 * The players of the game.
	 * @note Standard human player has index 0, public player 1.
	 */
	spieler_t *spieler[MAX_PLAYER_COUNT];

	/**
	 * Active player.
	 */
	spieler_t *active_player;

	/**
	 * Active player index.
	 */
	uint8 active_player_nr;

	/**
	 * Locally stored password hashes, will be used after reconnect to a server.
	 */
	pwd_hash_t player_password_hash[MAX_PLAYER_COUNT];

	/**
	 * @}
	 */

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
	 *       and allow for adecuate adjustments to adapt to the system performance
	 *       and available resources (also in network mode).
	 * @{
	 */

	/**
	 * ms since creation.
	 * @note The time is in ms (milliseconds)
	 * @author Hj. Malthaner
	 */
	uint32 ticks;

	/**
	 * Ticks counter at last steps.
	 * @note The time is in ms (milliseconds)
	 * @author Hj. Malthaner
	 */
	uint32 last_step_ticks;

	/**
	 * From now on is next month.
	 * @note The time is in ms (milliseconds)
	 * @author Hj. Malthaner
	 */
	uint32 next_month_ticks;

	/**
	 * Default time stretching factor.
	 */
	uint32 time_multiplier;

	uint8 step_mode;

	/// @note variable used in interactive()
	uint32 sync_steps;
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

	/**
	 * @}
	 */

	/**
	 * Current month 0..11
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
	long steps;

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
	const weg_besch_t *city_road;

	/**
	 * What game objectives.
	 */
	scenario_t *scenario;

	/**
	 * Holds all the text messages in the messagebox (chat, new vehicle etc).
	 */
	message_t *msg;

	/**
	 * Array indexed per way tipe. Used to determine the speedbonus.
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
	 * Recalculated speed boni for different vehicles.
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
	 * @author Hj. Malthaner
	 */
	void save(loadsave_t *file,bool silent);

	/**
	 * Internal loading method.
	 * @author Hj. Malthaner
	 */
	void load(loadsave_t *file);

	/**
	 * Removes all objects, deletes all data structures and frees all accesible memory.
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
	 * Distribute groundobjs and cities on the map but not
	 * in the rectangle from (0,0) till (old_x, old_y).
	 * It's now an extra function so we don't need the code twice.
	 * @auther Gerd Wachsmuth
	 */
	void distribute_groundobjs_cities(int new_cities, sint32 new_mittlere_einwohnerzahl, sint16 old_x, sint16 old_y );

	/**
	 * The last time when a server announce was performed (in ms).
	 */
	uint32 server_last_announce_time;

	void world_xy_loop(xy_loop_func func, bool sync_x_steps);
	static void *world_xy_loop_thread(void *);

	/**
	 * Loops over plans after load.
	 */
	void plans_laden_abschliessen(sint16, sint16, sint16, sint16);

	/**
	 * Updates all images.
	 */
	void update_map_intern(sint16, sint16, sint16, sint16);

public:
	/**
	 * Announce server and current state to listserver.
	 * @param status Specifies what information should be announced
	 * or offline (the latter only in cases where it is shutting down)
	 */
	void announce_server(int status);

	/**
	 * Reads height data from 8 or 25 bit bmp or ppm files.
	 * @return Either pointer to heightfield (use delete [] for it) or NULL.
	 */
	static bool get_height_data_from_file( const char *filename, sint8 grundwasser, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values );

	/**
	 * Returns the messagebox message container.
	 */
	message_t *get_message() { return msg; }

	/**
	 * Set to something useful, if there is a total distance != 0 to show in the bar below.
	 */
	koord3d show_distance;

	/**
	 * For warning, when stuff had to be removed/replaced
	 * level must be >=1 (1=factory, 2=vechiles, 3=not so important)
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
	 * Gets the world view.
	 */
	karte_ansicht_t *get_view() const { return view; }

	/**
	 * Sets the world view.
	 */
	void set_view(karte_ansicht_t *v) { view = v; }

	/**
	 * Viewpoint in tile coordinates.
	 * @author Hj. Malthaner
	 */
	koord get_world_position() const { return ij_off; }

	/**
	 * Fine offset within the viewport tile.
	 */
	int get_x_off() const {return x_off;}

	/**
	 * Fine offset within the viewport tile.
	 */
	int get_y_off() const {return y_off;}

	/**
	 * Set center viewport position.
	 * @author prissi
	 */
	void change_world_position( koord ij, sint16 x=0, sint16 y=0 );

	/**
	 * Set center viewport position, taking height into account
	 */
	void change_world_position( koord3d ij );

	/**
	 * Converts 3D coord to 2D actually used for main view.
	 */
	koord calculate_world_position( koord3d ) const;

	/**
	 * the koordinates between the screen and a tile may have several offset
	 * this routine caches them
	 */
	void set_view_ij_offset( koord k ) { view_ij_off=k; }

	/**
	 * the koordinates between the screen and a tile may have several offset
	 * this routine caches them
	 */
	koord get_view_ij_offset() const { return view_ij_off; }

	/**
	 * If this is true, the map will not be scrolled on right-drag.
	 * @author Hj. Malthaner
	 */
	void set_scroll_lock(bool yesno);

	/**
	 * Function for following a convoi on the map give an unbound handle to unset.
	 */
	void set_follow_convoi(convoihandle_t cnv) { follow_convoi = cnv; }

	/**
	 * ??
	 */
	convoihandle_t get_follow_convoi() const { return follow_convoi; }

	settings_t const& get_settings() const { return settings; }
	settings_t&       get_settings()       { return settings; }

	/// returns current speed bonus
	sint32 get_average_speed(waytype_t typ) const { return average_speed[ (typ==16 ? 3 : (int)(typ-1)&7 ) ]; }

	/// speed record management
	sint32 get_record_speed( waytype_t w ) const;
	void notify_record( convoihandle_t cnv, sint32 max_speed, koord pos );

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
	uint8 sp2num(spieler_t *sp);
	spieler_t * get_spieler(uint8 n) const { return spieler[n&15]; }
	spieler_t* get_active_player() const { return active_player; }
	uint8 get_active_player_nr() const { return active_player_nr; }
	void switch_active_player(uint8 nr, bool silent);
	const char *new_spieler( uint8 nr, uint8 type );
	void store_player_password_hash( uint8 player_nr, const pwd_hash_t& hash );
	const pwd_hash_t& get_player_password_hash( uint8 player_nr ) const { return player_password_hash[player_nr]; }
	void clear_player_password_hashes();
	void rdwr_player_password_hashes(loadsave_t *file);
	void remove_player(uint8 player_nr);

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
	 *
	 * @author neroden
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

	sint32 get_time_multiplier() const { return time_multiplier; }
	void change_time_multiplier( sint32 delta );

	/**
	 * @return 0=winter, 1=spring, 2=summer, 3=autumn
	 * @author prissi
	 */
	uint8 get_season() const { return season; }

	/**
	 * Zeit seit Kartenerzeugung/dem letzen laden in ms
	 * @author Hj. Malthaner
	 */
	uint32 get_zeit_ms() const { return ticks; }

	/**
	 * Absolute month (count start year zero).
	 * @author prissi
	 */
	uint32 get_current_month() const { return current_month; }

	/**
	 * prissi: current city road.
	 * @note May change due to timeline.
	 */
	const weg_besch_t* get_city_road() const { return city_road; }

	/**
	 * Anzahl steps seit Kartenerzeugung
	 * @author Hj. Malthaner
	 */
	long get_steps() const { return steps; }

	/**
	 * Idle time. Nur zur Anzeige verwenden!
	 * @author Hj. Malthaner
	 */
	uint32 get_schlaf_zeit() const { return idle_time; }

	/**
	 * Anzahl frames in der letzten Sekunde Realzeit
	 * @author prissi
	 */
	uint32 get_realFPS() const { return realFPS; }

	/**
	 * Anzahl Simulationsloops in der letzten Sekunde. Kann sehr ungenau sein!
	 * @author Hj. Malthaner
	 */
	uint32 get_simloops() const { return simloops; }

	/**
	 * Returns the current waterline height.
	 * @author Hj. Malthaner
	 */
	sint8 get_grundwasser() const { return grundwasser; }

	/**
	 * Returns the minimum allowed height on the map.
	 */
	sint8 get_minimumheight() const { return grundwasser-10; }

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
	 * Returns the climate for a given height, ruled by world creation settings.
	 * Used to determine climate when terraforming, loading old games, etc.
	 */
	climate get_climate_at_height(sint16 height) const
	{
		const sint16 h=height-grundwasser;
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
		const planquadrat_t *plan = lookup(k);
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

private:
	/**
	 * Dummy method, to generate compiler error if someone tries to call get_climate( int ),
	 * as the int parameter will silently be cast to koord...
	 */
	climate get_climate(sint16) const;

public:
	/**
	 * Set a new tool as current: calls local_set_werkzeug or sends to server.
	 */
	void set_werkzeug( werkzeug_t *w, spieler_t * sp );

	/**
	 * Set a new tool on our client, calls init.
	 */
	void local_set_werkzeug( werkzeug_t *w, spieler_t * sp );
	werkzeug_t *get_werkzeug(uint8 nr) const { return werkzeug[nr]; }

	/**
	 * Returns the (x,y) map size.
	 * @brief Map size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 * @note These values are exactly one less tham get_grid_size ones.
	 * @see get_grid_size()
	 */
	inline koord const &get_size() const { return cached_grid_size; }

	/**
	 * Maximum size for waiting bars etc.
	 */
	inline int get_size_max() const { return cached_size_max; }

	/**
	 * @return True if the specified coordinate is inside the world tiles(planquadrat_t) limits, false otherwise.
	 * @param k (x,y) coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_limits(koord k) const {
		// prissi: since negative values will make the whole result negative, we can use bitwise or
		// faster, since pentiums and other long pipeline processors do not like jumps
		return (k.x|k.y|(cached_size.x-k.x)|(cached_size.y-k.y))>=0;
		// this is only 67% of the above speed
		//return k.x>=0 &&  k.y>=0  &&  cached_groesse_karte_x>=k.x  &&  cached_groesse_karte_y>=k.y;
	}

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
//		return x>=0 &&  y>=0  &&  cached_groesse_karte_x>=x  &&  cached_groesse_karte_y>=y;
	}

	/**
	 * @return True if the specified coordinate is inside the world height grid limits, false otherwise.
	 * @param k (x,y) coordinate.
	 * @note Inline because called very frequently!
	 */
	inline bool is_within_grid_limits(const koord &k) const {
	// prissi: since negative values will make the whole result negative, we can use bitwise or
	// faster, since pentiums and other long pipeline processors do not like jumps
		return (k.x|k.y|(cached_grid_size.x-k.x)|(cached_grid_size.y-k.y))>=0;
//		return k.x>=0 &&  k.y>=0  &&  cached_groesse_gitter_x>=k.x  &&  cached_groesse_gitter_y>=k.y;
	}

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
//		return x>=0 &&  y>=0  &&  cached_groesse_gitter_x>=x  &&  cached_groesse_gitter_y>=y;
	}

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
	 * @return Planquadrat an koordinate pos
	 * @note Inline because called very frequently!
	 * @author Hj. Malthaner
	 */
	inline const planquadrat_t *lookup(const koord &k) const
	{
		return is_within_limits(k.x, k.y) ? &plan[k.x+k.y*cached_grid_size.x] : 0;
	}

	/**
	 * @return grund an pos/hoehe
	 * @note Inline because called very frequently!
	 * @author Hj. Malthaner
	 */
	inline grund_t *lookup(const koord3d &pos) const
	{
		const planquadrat_t *plan = lookup(pos.get_2d());
		return plan ? plan->get_boden_in_hoehe(pos.z) : NULL;
	}

	/**
	 * This function takes grid coordinates as a parameter and a desired height (koord3d).
	 * Will return the ground_t object that intersects with it in it's north corner if possible.
	 * If that tile doesn't exist, returns the one that interesects with it in other corner.
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
			return lookup(koord(pos.x-1, pos.y-1))->get_kartenboden();
		}
		else if ( pos.x == cached_grid_size.x ) {
			return lookup(koord(pos.x-1, pos.y))->get_kartenboden();
		}
		return lookup(koord(pos.x, pos.y-1))->get_kartenboden();
	}

	/**
	 * @return The corner that needs to be raised/lowered on the given coordinates.
	 * @param pos Grid coordinate to check.
	 * @note Inline because called very frequently!
	 * @note Will always return north-west except on border tiles.
	 * @pre pos has to be a valid grid coordinate, undefined otherwise.
	 */
	inline hang_t::typ get_corner_to_operate(const koord &pos) const
	{
		// Normal tile
		if ( ( pos.x != cached_grid_size.x )  &&  ( pos.y != cached_grid_size.y ) ){
			return hang_t::corner_NW;
		}
		// Border on south-east
		if ( is_within_limits(pos.x-1, pos.y) ) {
			return(hang_t::corner_NE);
		}
		// Border on south-west
		if ( is_within_limits(pos.x, pos.y-1) ) {
			return(hang_t::corner_SW);
		}
		// Border on south
		return (hang_t::corner_SE);
	}

	/**
	 * @return grund at the bottom (where house will be build)
	 * @note Inline because called very frequently!
	 */
	inline grund_t *lookup_kartenboden(const koord &pos) const
	{
		return is_within_limits(pos.x, pos.y) ? plan[pos.x+pos.y*cached_grid_size.x].get_kartenboden() : NULL;
	}

	/**
	 * @return The natural slope at a position.
	 * @note Uses the corner height for the best slope.
	 * @author prissi
	 */
	uint8	recalc_natural_slope( const koord pos, sint8 &new_height ) const;

	/**
	 * Returns the natural slope a a position using the grid.
	 * @note No checking, and only using the grind for calculation.
	 */
	uint8	calc_natural_slope( const koord pos ) const;

	/**
	 * Wird vom Strassenbauer als Orientierungshilfe benutzt.
	 * @author Hj. Malthaner
	 */
	inline void markiere(const grund_t* gr) { marker.markiere(gr); }

	/**
	 * Wird vom Strassenbauer zum Entfernen der Orientierungshilfen benutzt.
	 * @author Hj. Malthaner
	 */
	inline void unmarkiere(const grund_t* gr) { marker.unmarkiere(gr); }

	/**
	 * Entfernt alle Markierungen.
	 * @author Hj. Malthaner
	 */
	inline void unmarkiere_alle() { marker.unmarkiere_alle(); }

	/**
	 * Testet ob der Grund markiert ist.
	 * @return Gibt true zurueck wenn der Untergrund markiert ist sonst false.
	 * @author Hj. Malthaner
	 */
	inline bool ist_markiert(const grund_t* gr) const { return marker.ist_markiert(gr); }

	 /**
	  * Initialize map.
	  * @param sets Game settings.
	  * @param preselected_players Defines which players the user has selected before he started the game.
	  * @author Hj. Malthaner
	  */
	void init(settings_t*, sint8 const* heights);

	void init_felder();

	void enlarge_map(settings_t const*, sint8 const* h_field);

	karte_t();

	~karte_t();

	/**
	 * Returns an index to a halt (or creates a new one)
	 * @note Only used during loading
	 */
	halthandle_t get_halt_koord_index(koord k);

	/**
	 * Checks if the planquadrat at coordinate (x,y)
	 * can be lowered at the specified height.
	 * @author V. Meyer
	 */
	bool can_lower_plan_to(sint16 x, sint16 y, sint8 h) const;

	/**
	 * Checks if the planquadrat at coordinate (x,y)
	 * can be raised at the specified height.
	 * @author V. Meyer
	 */
	bool can_raise_plan_to(sint16 x, sint16 y, sint8 h) const;

	/**
	 * Checks if the whole planquadrat at coordinates (x,y) height can
	 * be changed ( for example, water height can't be changed ).
	 * @author Hj. Malthaner
	 */
	bool is_plan_height_changeable(sint16 x, sint16 y) const;

	/**
	 * Increases the height of the grid coordinate (x, y) by one.
	 * @param pos Grid coordinate.
	 */
	int grid_raise(koord pos);

	/**
	 * Decreases the height of the grid coordinate (x, y) by one.
	 * @param pos Grid coordinate.
	 */
	int grid_lower(koord pos);

	// mostly used by AI: Ask to flatten a tile
	bool can_ebne_planquadrat(koord pos, sint8 hgt, bool keep_water=false, bool make_underwater_hill=false) const;
	bool ebne_planquadrat(spieler_t *sp, koord pos, sint8 hgt, bool keep_water=false, bool make_underwater_hill=false);

	// the convois are also handled each step => thus we keep track of them too
	void add_convoi(convoihandle_t);
	void rem_convoi(convoihandle_t);
	vector_tpl<convoihandle_t> const& convoys() const { return convoi_array; }

	/**
	 * To access the cities array.
	 * @author Hj. Malthaner
	 */
	const weighted_vector_tpl<stadt_t*>& get_staedte() const { return stadt; }
	void add_stadt(stadt_t *s);

	/**
	 * Removes town from map, houses will be left overs.
	 * @author prissi
	 */
	bool rem_stadt(stadt_t *s);

	/* tourist attraction list */
	void add_ausflugsziel(gebaeude_t *gb);
	void remove_ausflugsziel(gebaeude_t *gb);
	const weighted_vector_tpl<gebaeude_t*> &get_ausflugsziele() const {return ausflugsziele; }

	void add_label(koord pos) { if (!labels.is_contained(pos)) labels.append(pos); }
	void remove_label(koord pos) { labels.remove(pos); }
	const slist_tpl<koord>& get_label_list() const { return labels; }

	bool add_fab(fabrik_t *fab);
	bool rem_fab(fabrik_t *fab);
	int get_fab_index(fabrik_t* fab)  const { return fab_list.index_of(fab); }
	fabrik_t* get_fab(unsigned index) const { return index < fab_list.get_count() ? fab_list.at(index) : NULL; }
	const slist_tpl<fabrik_t*>& get_fab_list() const { return fab_list; }

	/**
	 * Returns a list of goods produced by factories that exist in current game.
	 */
	const vector_tpl<const ware_besch_t*> &get_goods_list();

	/**
	 * Seaches and returns the closest city to the supplied coordinates.
	 * @author Hj. Malthaner
	 */
	stadt_t *suche_naechste_stadt(koord pos) const;

	bool cannot_save() const { return nosave; }
	void set_nosave() { nosave = true; nosave_warning = true; }
	void set_nosave_warning() { nosave_warning = true; }

	/// rotate plans by 90 degrees
	void rotate90_plans(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max);

	/// rotate map view by 90 degrees
	void rotate90();

	bool sync_add(sync_steppable *obj);
	bool sync_remove(sync_steppable *obj);
	void sync_step(long delta_t, bool sync, bool display );	// advance also the timer

	bool sync_eyecandy_add(sync_steppable *obj);
	bool sync_eyecandy_remove(sync_steppable *obj);
	void sync_eyecandy_step(long delta_t);	// all stuff, which does not need explicit order (factory smoke, buildings)

	bool sync_way_eyecandy_add(sync_steppable *obj);
	bool sync_way_eyecandy_remove(sync_steppable *obj);
	void sync_way_eyecandy_step(long delta_t);	// currently one smoke from vehicles on ways


	/**
	 * For all stuff, that needs long and can be done less frequently.
	 */
	void step();

	inline planquadrat_t *access(int i, int j) const {
		return is_within_limits(i, j) ? &plan[i + j*cached_grid_size.x] : NULL;
	}

	inline planquadrat_t *access(koord k) const {
		return is_within_limits(k) ? &plan[k.x + k.y*cached_grid_size.x] : NULL;
	}

	/**
	 * @return Height at the grid point i, j
	 * @author Hj. Malthaner
	 */
	inline sint8 lookup_hgt(koord k) const {
		return is_within_grid_limits(k.x, k.y) ? grid_hgts[k.x + k.y*(cached_grid_size.x+1)] : grundwasser;
	}

	/**
	 * Sets grid height.
	 * Never set grid_hgts manually, always use this method!
	 * @author Hj. Malthaner
	 */
	void set_grid_hgt(koord k, sint8 hgt) { grid_hgts[k.x + k.y*(uint32)(cached_grid_size.x+1)] = hgt; }

	/**
	 * @return water height
	 * @author Kieron Green
	 */
	inline sint8 get_water_hgt(koord k) const {
		return is_within_grid_limits( k.x, k.y ) ? water_hgts[k.x + k.y * (cached_grid_size.x)] : grundwasser;
	}

	/**
	 * Sets water height.
	 * @author Kieron Green
	 */
	void set_water_hgt(koord k, sint8 hgt) { water_hgts[k.x + k.y * (uint32)(cached_grid_size.x)] = (hgt); }

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
	 * @return Minimum height of the planquadrats at i, j.
	 * @author Hj. Malthaner
	 */
	sint8 min_hgt(koord pos) const;

	/**
	 * @return Maximum height of the planquadrats at i, j.
	 * @author Hj. Malthaner
	 */
	sint8 max_hgt(koord pos) const;

	/**
	 * @return true, wenn Platz an Stelle pos mit Groesse dim Wasser ist
	 * @author V. Meyer
	 */
	bool ist_wasser(koord pos, koord dim) const;

	/**
	 * @return true, if square in place (i,j) with size w, h is constructible.
	 * @author Hj. Malthaner
	 */
	bool square_is_free(koord pos, sint16 w, sint16 h, int *last_y, climate_bits cl) const;

	/**
	 * @return A list of all buildable squares with size w, h.
	 * @note Only used for town creation at the moment.
	 * @author Hj. Malthaner
	 */
	slist_tpl<koord> * find_squares(sint16 w, sint16 h, climate_bits cl, sint16 old_x, sint16 old_y) const;

	/**
	 * Plays the sound when the position is inside the visible region.
	 * The sound plays lower when the position is outside the visible region.
	 * @param pos Position at wich the event took place.
	 * @param idx Index of the sound
	 * @author Hj. Malthaner
	 */
	bool play_sound_area_clipped(koord pos, uint16 idx) const;

	void mute_sound( bool state ) { is_sound = !state; }

	/**
	 * Saves the map to a file.
	 * @param Filename name of the file to write.
	 * @author Hj. Malthaner
	 */
	void save(const char *filename, const loadsave_t::mode_t savemode, const char *version, bool silent);

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
};

#endif
