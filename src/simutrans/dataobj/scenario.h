/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SCENARIO_H
#define DATAOBJ_SCENARIO_H


/** @file scenario.h declarations for scenario interface */

#include "koord3d.h"
#include "../utils/plainstring.h"
#include "../script/dynamic_string.h"
#include "../dataobj/ribi.h"
#include "../convoihandle.h"

class loadsave_t;
class stadt_t;
class fabrik_t;
class karte_t;
class schedule_t;
class depot_t;

/**
 * @class scenario_t
 * Controls scenarios in connection to a simutrans world.
 *
 * Scenarios are scripted. In network games, only the server has access to the script,
 * clients will be sent some results of the script.
 *
 * Each instance of karte_t carries a non-NULL pointer to a scenario_t
 * thus also the need for inactive scenarios.
 */
class scenario_t
{
private:
	/// possible states of scenario
	enum scenario_state_t {
		INACTIVE         = 0, ///< scenario inactive
		SCRIPTED         = 7, ///< scenario active (non-network game or at server)
		SCRIPTED_NETWORK = 8  ///< scenario active, network game at client
	};

	/// state of the current scenario @see scenario_state_t
	uint16 what_scenario;

	/// the world we are scripting in
	karte_t *welt;

	/// name of scenario, files are searched in scenario_path/scenario_name/...
	/// e.g. my_scenario
	plainstring scenario_name;

	/// path to scenario directory (relative to env_t::user_dir)
	/// e.g. pak/scenario/my_scenario/
	plainstring scenario_path;


	/**
	 * loads scenario file with the given name
	 * @param filename name scenario script file (including .nut extension)
	 */
	bool load_script(const char* filename);

	/// is set, if an error occurred during loading of savegame
	/// e.g. re-starting of scenario failed due to script error
	bool rdwr_error;


	/// pointer to virtual machine
	script_vm_t *script;

	/// @{
	/// @name Interface to forbid tools in-game
	/**
	 * Struct to store information about forbidden tools
	 *
	 * Necessary in network games: there, the list of forbidden tools
	 * is transferred to clients. Needed to apply conditions to e.g. way-building
	 * tools or to have toolbars reflect allowed tools.
	 */
	struct forbidden_t {
		enum forbid_type {
			forbid_tool      = 1,
			forbid_tool_rect = 2
		};

		forbid_type type;
		uint8 player_nr;
		/// id of tool to be forbidden, as set by constructors of classes derived from
		/// tool_t, @see tool/simtool.h
		uint16 toolnr;
		/// waytype of tool, @see waytype_t
		sint16 waytype;
		koord pos_nw, pos_se;
		sint8 hmin, hmax;
		/// error message to be displayed if user tries to work with the tool
		plainstring error;

		/// constructor: forbid tool/etc for a certain player
		forbidden_t(forbid_type type_=forbid_tool, uint8 player_nr_=255, uint16 toolnr_=0, sint16 waytype_=invalid_wt) :
			type(type_), player_nr(player_nr_), toolnr(toolnr_), waytype(waytype_),
			pos_nw(koord::invalid), pos_se(koord::invalid), hmin(-128), hmax(127), error() {}

		/// constructor: forbid tool for a certain player at certain locations (and heights)
		forbidden_t(uint8 player_nr_, uint16 toolnr_, sint16 waytype_, koord nw, koord se, sint8 hmin_=-128, sint8 hmax_=127) :
			type(forbid_tool_rect), player_nr(player_nr_), toolnr(toolnr_), waytype(waytype_),
			pos_nw(nw), pos_se(se), hmin(hmin_), hmax(hmax_), error() {}

		// copy constructor
		forbidden_t(const forbidden_t&);

		/**
		 * @returns if this < other, compares: type, playernr, tool, wt
		 * DIRTY: (a <= b)  &&  (b <= a)  DOES NOT imply  a == b
		 */
		bool operator <(const forbidden_t &) const;

		bool operator <=(const forbidden_t &other) const { return !(other < *this); }

		static bool compare(const forbidden_t *a, const forbidden_t *b)
		{
			return *a < *b;
		}

		/**
		 * compares everything (including coordinates)
		 * DIRTY: (a <= b)  &&  (b <= a)  DOES NOT imply  a == b
		 */
		bool operator ==(const forbidden_t &other) const;

		/**
		 * templated load/save support
		 */
		template<class T> void rdwr(T *file)
		{
			uint8 t = (uint8)type;
			file->rdwr_byte(t);
			type= (forbid_type)t;
			file->rdwr_byte(player_nr);
			file->rdwr_short(toolnr);
			file->rdwr_short(waytype);
			file->rdwr_short(pos_nw.x); file->rdwr_short(pos_nw.y);
			file->rdwr_short(pos_se.x); file->rdwr_short(pos_se.y);
			file->rdwr_byte(hmin);
			file->rdwr_byte(hmax);
			file->rdwr_str(error);
		}

		void rotate90(const sint16 y_size);

	private:
		const forbidden_t& operator=(const forbidden_t&);
	};

	/// list of forbidden tools
	vector_tpl<forbidden_t*>forbidden_tools;

	/// set to true if rules changed to update toolbars,
	/// toolbars and active tools will be updated in next call to step()
	bool need_toolbar_update;

	/**
	 * helper function:
	 * @param other given record
	 * @returns first index i such that
	 *          forbidden_tools[i-1] < other <= forbidden_tools[i] <= other
	 *          or returns  forbidden_tools.get_count() if no such index is found
	 */
	uint32 find_first(const forbidden_t &other) const;

	/**
	 * Helper function:
	 * Puts/removes new record into/from forbidden_tools list, checks for identical entries.
	 * Only call this method from call_forbid_tool(forbidden_t *,bool)
	 * @param test must be pointer to allocated memory, will be invalid after call
	 * @param forbid if true puts, if false removes into/from list
	 */
	void intern_forbid(forbidden_t *test, bool forbid);

	/**
	 * Helper function: works on forbidden_tools directly (if not in network-mode)
	 * or sends information over network (if at server)
	 * @param test must be pointer to allocated memory, will be invalid after call
	 * @param forbid if true forbids, if false allows the record
	 */
	void call_forbid_tool(forbidden_t *test, bool forbid);
	/// @}

	/// bit set if player has won / lost
	uint16 won;
	uint16 lost;

	/// function to update the won / lost bitset
	/// called if this information changes for some players
	void update_won_lost(uint16 new_won, uint16 new_lost);

public:

	scenario_t(karte_t *w);
	~scenario_t();

	/**
	 * Initializes scripted scenario
	 */
	const char* init( const char *scenario_base, const char *scenario_name, karte_t *welt );

	/**
	 * Load file with translations. Tries to load files in the following order
	 * (1) script_addon_path/iso/filename
	 * (2) script_addon_path/en/filename
	 * (3) script_path/iso/filename
	 *
	 * Here, iso refers to iso-abbreviation of currently active language
	 * @return content of loaded file
	 */
	plainstring load_language_file(const char* filename);

	/// Load/save support
	void rdwr(loadsave_t *file);

	/// @returns true if loading succeed, false if script failed during loading
	bool rdwr_ok() const { return !rdwr_error; }

	/**
	 * Stop scenario
	 */
	void stop() { what_scenario = INACTIVE; }

	/// @return true if a scenario is present
	bool active() const { return what_scenario != INACTIVE; }

	/// @return true if scenario is scripted
	bool is_scripted() const { return what_scenario == SCRIPTED  ||  what_scenario == SCRIPTED_NETWORK; }

	/**
	 * compiles and executes given string
	 * @returns error msg (or NULL if succeeded)
	 */
	const char* eval_string(const char* squirrel_string) const;

	/**
	 * Get percentage of scenario completion. Does not call script to update this value.
	 * On clients: call server for update via dynamic_string logic.
	 * Returns percentage of scenario completion.
	 * @param player_nr player
	 * @returns percentage of scenario completion:
	 * if >= 100 then scenario is won
	 * if < 0 then scenario is lost
	 */
	sint32 get_completion(int player_nr);

	/**
	 * Sets percentage of scenario completion. Used as callback if script call got suspended.
	 * @param player_nr player
	 * @returns dummy return value
	 */
	bool set_completion(sint32 player_nr, sint32 percentage);

	void rotate90(const sint16 y_size);

	/**
	 * rotate original coordinates to actual world coordinates
	 * uses the methods in script_api
	 */
	void koord_sq2w(koord &) const;

	/**
	 * Text to be displayed in the finance info window
	 * i.e. short description of scenario
	 */
	dynamic_string description_text;

	/// @{
	/// @name Text to be displayed in the scenario info window
	dynamic_string info_text;
	dynamic_string goal_text;
	dynamic_string rule_text;
	dynamic_string result_text;
	dynamic_string about_text;
	dynamic_string debug_text;
	/// @}

	/**
	 * Called to update the scenario texts
	 * @see dynamic_string::update
	 */
	void update_scenario_texts();

	/**
	 * opens scenario info window at tab @p tab.
	 */
	bool open_info_win(const char* tab = "result") const;


	/**
	 * Last error of script
	 */
	const char* get_error_text();


	/**
	 * Calls scripted is_scenario_completed. Caches this value in statistics of player_t.
	 * Server sends update of won/lost if necessary.
	 */
	void step();

	/**
	 * Called upon month change: at 0:00 of the first day of the new month.
	 */
	void new_month();

	/**
	 * Called upon new year: at 0:00 January 1st.
	 */
	void new_year();

	/// @{
	/// @name Interface to forbid tools in-game
	/**
	 * Forbid tool
	 * @ingroup squirrel-api
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to MAX_PLAYER_COUNT then this acts for all players except public player
	 * @param tool_id id of tool
	 */
	void forbid_tool(uint8 player_nr, uint16 tool_id);

	/**
	 * @ingroup squirrel-api
	 * @see forbid_tool
	 */
	void allow_tool(uint8 player_nr, uint16 tool_id);

	/**
	 * Forbid tool with certain waytype
	 * @ingroup squirrel-api
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to MAX_PLAYER_COUNT then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 */
	void forbid_way_tool(uint8 player_nr, uint16 tool_id, waytype_t wt);

	/**
	 * @ingroup squirrel-api
	 * @see forbid_way_tool
	 */
	void allow_way_tool(uint8 player_nr, uint16 tool_id, waytype_t wt);

	/**
	 * Forbid tool with certain waytype within rectangular region on the map
	 * @ingroup squirrel-api
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to MAX_PLAYER_COUNT then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @param pos_nw coordinate of north-western corner of rectangle
	 * @param pos_se coordinate of south-eastern corner of rectangle
	 * @param err error message presented to user when trying to apply this tool
	 */
	void forbid_way_tool_rect(uint8 player_nr, uint16 tool_id, waytype_t wt, koord pos_nw, koord pos_se, plainstring err);

	/**
	 * @ingroup squirrel-api
	 * @see forbid_way_tool_rect
	 */
	void allow_way_tool_rect(uint8 player_nr, uint16 tool_id, waytype_t wt, koord pos_nw, koord pos_se);

	/**
	 * Forbid tool with certain waytype within cubic region on the map.
	 * @ingroup squirrel-api
	 *
	 * @param player_nr number of player this rule applies to,
	 *                  if this is set to MAX_PLAYER_COUNT then this acts for all players except public player
	 * @param tool_id id of tool
	 * @param wt waytype
	 * @param pos_nw coordinate of north-western corner of cube
	 * @param pos_se coordinate of south-eastern corner of cube
	 * @param err error message presented to user when trying to apply this tool
	 */
	void forbid_way_tool_cube(uint8 player_nr, uint16 tool_id, waytype_t wt, koord3d pos_nw, koord3d pos_se, plainstring err);

	/**
	 * @ingroup squirrel-api
	 * @see forbid_way_tool_cube
	 */
	void allow_way_tool_cube(uint8 player_nr, uint16 tool_id, waytype_t wt, koord3d pos_nw, koord3d pos_se);

	/**
	 * Clears all rules.
	 * @ingroup squirrel-api
	 */
	void clear_rules();

	/**
	 * Toolbars/active tools need an update due to changed rules; update is done in step().
	 * @ingroup squirrel-api
	 */
	void gui_needs_update() { need_toolbar_update = true; }

	/**
	 * Checks if player can use this tool at all.
	 * Called for instance in karte_t::local_set_tool to change active tool or when filling toolbars.
	 * @return true if player can use this tool.
	 */
	bool is_tool_allowed(const player_t* player, uint16 tool_id, sint16 wt = invalid_wt);

	/**
	 * Checks if player can use the tool at this position.
	 * @return NULL if allowed otherwise error message
	 */
	const char* is_work_allowed_here(const player_t* player, uint16 tool_id, sint16 wt, koord3d pos);

	/**
	 * Checks if player can use this schedule.
	 *
	 * @param player player
	 * @param schedule the schedule
	 *
	 * @return null if allowed, an error message otherwise
	 */
	const char* is_schedule_allowed(const player_t* player, const schedule_t* schedule);

	/**
	 * Checks if player can use this convoy.
	 * Called when player wants to start convoy at depot.
	 *
	 * @param player player
	 * @param cnv convoy
	 * @param depot depot
	 *
	 * @return null if allowed, an error message otherwise
	 */
	const char* is_convoy_allowed(const player_t* player, convoihandle_t cnv, depot_t* depot);

	/**
	 * Called when player click link in scenario windows, after position changed.
	 *
	 * @param pos coordinate go to in link
	 *
	 * @return an error message otherwise or null
	 */
	const char* jump_to_link_executed(koord3d pos);

	/// @return debug dump of forbidden tools
	const char* get_forbidden_text();
	/// @}

	friend class nwc_scenario_t; ///< to access vm, update_won_lost()
	friend class nwc_scenario_rules_t; ///< to access forbidden_tool stuff
};

#endif
