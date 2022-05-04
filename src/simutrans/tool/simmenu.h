/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TOOL_SIMMENU_H
#define TOOL_SIMMENU_H


#include <string>
#include "../descriptor/sound_desc.h"

#include "../dataobj/koord3d.h"
#include "../dataobj/translator.h"

#include "../simtypes.h"
#include "../display/simimg.h"

/// New configurable OOP tool system


template<class T> class vector_tpl;
template<class T> class slist_tpl;

class scr_coord;
class tool_selector_t;
class player_t;
class toolbar_t;
class memory_rw_t;
class karte_ptr_t;
class zeiger_t;

enum {
	// general tools
	TOOL_QUERY=0,
	TOOL_REMOVER,
	TOOL_RAISE_LAND,
	TOOL_LOWER_LAND,
	TOOL_SETSLOPE,
	TOOL_RESTORESLOPE,
	TOOL_MARKER,
	TOOL_CLEAR_RESERVATION,
	TOOL_TRANSFORMER,
	TOOL_ADD_CITY,
	TOOL_CHANGE_CITY_SIZE,
	TOOL_PLANT_TREE,
	TOOL_SCHEDULE_ADD,
	TOOL_SCHEDULE_INS,
	TOOL_BUILD_WAY,
	TOOL_BUILD_BRIDGE,
	TOOL_BUILD_TUNNEL,
	TOOL_WAYREMOVER,
	TOOL_BUILD_WAYOBJ,
	TOOL_BUILD_STATION,
	TOOL_BUILD_ROADSIGN,
	TOOL_BUILD_DEPOT,
	TOOL_BUILD_HOUSE,
	TOOL_BUILD_LAND_CHAIN,
	TOOL_CITY_CHAIN,
	TOOL_BUILD_FACTORY,
	TOOL_LINK_FACTORY,
	TOOL_HEADQUARTER,
	TOOL_LOCK_GAME,
	TOOL_ADD_CITYCAR,
	TOOL_FOREST,
	TOOL_STOP_MOVER,
	TOOL_MAKE_STOP_PUBLIC,
	TOOL_REMOVE_WAYOBJ,
	TOOL_SLICED_AND_UNDERGROUND_VIEW,
	TOOL_BUY_HOUSE,
	TOOL_BUILD_CITYROAD,
	TOOL_ERROR_MESSAGE,
	TOOL_CHANGE_WATER_HEIGHT,
	TOOL_SET_CLIMATE,
	TOOL_ROTATE_BUILDING,
	TOOL_MERGE_STOP,
	TOOL_EXEC_SCRIPT,
	TOOL_EXEC_TWO_CLICK_SCRIPT,
	TOOL_PLANT_GROUNDOBJ,
	TOOL_ADD_MESSAGE,
	TOOL_REMOVE_SIGNAL,
	GENERAL_TOOL_COUNT,
	GENERAL_TOOL = 0x1000
};

enum {
	// simple one click tools
	TOOL_PAUSE = 0,
	TOOL_FASTFORWARD,
	TOOL_SCREENSHOT,
	TOOL_INCREASE_INDUSTRY,
	TOOL_UNDO,
	TOOL_SWITCH_PLAYER,
	TOOL_STEP_YEAR,
	TOOL_CHANGE_GAME_SPEED,
	TOOL_ZOOM_IN,
	TOOL_ZOOM_OUT,
	TOOL_SHOW_COVERAGE,
	TOOL_SHOW_NAME,
	TOOL_SHOW_GRID,
	TOOL_SHOW_TREES,
	TOOL_SHOW_HOUSES,
	TOOL_SHOW_UNDERGROUND,
	TOOL_ROTATE90,
	TOOL_QUIT,
	TOOL_FILL_TREES,
	TOOL_DAYNIGHT_LEVEL,
	TOOL_VEHICLE_TOOLTIPS,
	TOOL_TOOGLE_PAX,
	TOOL_TOOGLE_PEDESTRIANS,
	TOOL_TRAFFIC_LEVEL,
	TOOL_CHANGE_CONVOI,
	TOOL_CHANGE_LINE,
	TOOL_CHANGE_DEPOT,
	UNUSED_WKZ_PWDHASH_TOOL,
	TOOL_CHANGE_PLAYER,
	TOOL_CHANGE_TRAFFIC_LIGHT,
	TOOL_CHANGE_CITY,
	TOOL_RENAME,
	UNUSED_TOOL_ADD_MESSAGE,
	TOOL_TOGGLE_RESERVATION,
	TOOL_VIEW_OWNER,
	TOOL_HIDE_UNDER_CURSOR,
	TOOL_MOVE_MAP,
	TOOL_ROLLUP_ALL_WIN,
	TOOL_RECOLOUR_TOOL,
	TOOL_SHOW_FACTORY_STORAGE,
	TOOL_TOGGLE_CONTROL,
	SIMPLE_TOOL_COUNT,
	SIMPLE_TOOL = 0x2000
};

enum {
	// dialogue tools
	DIALOG_HELP = 0,
	DIALOG_OPTIONS,
	DIALOG_MINIMAP,
	DIALOG_LINEOVERVIEW,
	DIALOG_MESSAGES,
	DIALOG_FINANCES,
	DIALOG_PLAYERS,
	DIALOG_DISPLAYOPTIONS,
	DIALOG_SOUND,
	DIALOG_LANGUAGE,
	DIALOG_PLAYERCOLOR,
	DIALOG_JUMP,
	DIALOG_LOAD,
	DIALOG_SAVE,
	DIALOG_LIST_HALT,
	DIALOG_LIST_CONVOI,
	DIALOG_LIST_TOWN,
	DIALOG_LIST_GOODS,
	DIALOG_LIST_FACTORY,
	DIALOG_LIST_CURIOSITY,
	DIALOG_EDIT_FACTORY,
	DIALOG_EDIT_ATTRACTION,
	DIALOG_EDIT_HOUSE,
	DIALOG_EDIT_TREE,
	DIALOG_ENLARGE_MAP,
	DIALOG_LIST_LABEL,
	DIALOG_CLIMATES,
	DIALOG_SETTINGS,
	DIALOG_GAMEINFO,
	DIALOG_THEMES,
	DIALOG_SCENARIO,
	DIALOG_SCENARIO_INFO,
	DIALOG_LIST_DEPOT,
	DIALOG_LIST_VEHICLE,
	DIALOG_SCRIPT_TOOL,
	DIALOG_EDIT_GROUNDOBJ,
	DIALOGE_TOOL_COUNT,
	DIALOGE_TOOL = 0x4000
};

enum {
	// toolbars
	TOOL_MAINMENU = 0,
	TOOL_LAST_USED = 1022,
	TOOLBAR_TOOL = 0x8000u
};


class tool_t {
protected:
	image_id icon;
private:
	/* value to trigger this command (see documentation) */
	uint16 id;

protected:
	static karte_ptr_t welt;

	const char *default_param;

public:
	uint16 get_id() const { return id; }

	const char *get_name() const { return id_to_string(id); }

	static const char *id_to_string(uint16 id);

	static tool_t *dummy;

	// for key lookup
	static vector_tpl<tool_t *>char_to_tool;

	// true, if the control key should be inverted
	static uint8 control_invert;

	/// cursor image
	image_id cursor;

	/// cursor marks this area
	koord cursor_area;

	/// cursor centered at marked area? default: false
	bool cursor_centered;

	/// cursor offset within marked area (only effective if cursor_centered != false)
	koord cursor_offset;

	/// z-offset of cursor, possible values: Z_PLAN and Z_GRID
	sint8 offset;

	sint16 ok_sound;

	/// a script is waiting for a call-back
	uint32 callback_id;

	enum {
		WFL_SHIFT  = 1 << 0, ///< shift-key was pressed when mouse-click happened
		WFL_CTRL   = 1 << 1, ///< ctrl-key was pressed when mouse-click happened
		WFL_LOCAL  = 1 << 2, ///< tool call was issued by local client
		WFL_SCRIPT = 1 << 3, ///< tool call was issued by script
		WFL_NO_CHK = 1 << 4  ///< tool call needs no password or scenario checks
	};
	uint8 flags; // flags are set before init/work/move is called

	bool is_ctrl_pressed()    const { return flags & WFL_CTRL; }
	bool is_shift_pressed()   const { return flags & WFL_SHIFT; }
	bool is_local_execution() const { return flags & WFL_LOCAL; }
	bool is_scripted()        const { return flags & WFL_SCRIPT; }
	bool no_check()           const { return flags & WFL_NO_CHK; }
	bool can_use_gui()        const { return is_local_execution()  &&  !is_scripted(); }

	uint8  command_flags; // only shift and control
	uint16 command_key;// key to toggle action for this function

	static vector_tpl<tool_t *> general_tool;
	static vector_tpl<tool_t *> simple_tool;
	static vector_tpl<tool_t *> dialog_tool;
	static vector_tpl<toolbar_t *> toolbar_tool;

	static void update_toolbars();

	// since only a single toolstr a time can be visible ...
	static char toolstr[1024];

	static void init_menu();
	static void exit_menu();

	/// Read tool, toolbar configuration and tool shortcuts from @p menuconf
	/// @param menuconf Path to file to read
	static bool read_menu(const std::string &menuconf);

	static uint16 const dummy_id = 0xFFFFU;

	tool_t(uint16 const id) : id(id), cursor_area(1,1)
	{
		cursor = icon = IMG_EMPTY;
		ok_sound = NO_SOUND;
		offset = Z_PLAN;
		default_param = NULL;
		command_key = 0;
		cursor_centered = false;
		flags = 0;
		callback_id = 0;
	}

	virtual ~tool_t() {}

	virtual image_id get_icon(player_t *) const { return icon; }
	void set_icon(image_id i) { icon = i; }

	// returns default_param of this tool for player
	// if player==NULL returns default_param that was used to create the tool
	virtual const char* get_default_param(player_t* = NULL) const { return default_param; }
	void set_default_param(const char* str) { default_param = str; }

	// transfer additional information in networkgames
	virtual void rdwr_custom_data(memory_rw_t*) { }

	// this will draw the tool with some indication, if active
	virtual bool is_selected() const;

	// when true, local execution would do no harm
	virtual bool is_init_keeps_game_state() const { return false; }

	// if is_work_keeps_game_state()==false
	// and is_work_here_keeps_game_state(...)==false
	// then work-command is sent over network or must execute outside a sync_step
	virtual bool is_work_keeps_game_state() const { return false; }
	virtual bool is_work_here_keeps_game_state(player_t *, koord3d) { return false; }

	// will draw a dark frame, if selected
	virtual void draw_after(scr_coord pos, bool dirty) const;

	virtual const char *get_tooltip(const player_t *) const { return NULL; }

	/**
	 * @return true if this tool operates over the grid, not the map tiles.
	 */
	virtual bool is_grid_tool() const {return false;}

	/**
	 * Returning false on init will automatically invoke previous tool.
	 * Returning true will select tool and will make it possible to call work.
	 */
	virtual bool init( player_t * ) { return true; }

	/// initializes cursor (icon, marked area)
	void init_cursor( zeiger_t * ) const;

	// returning true on exit will have tool_selector resets to query-tool on right-click
	virtual bool exit( player_t * ) { return true; }

	/* the return string can have different meanings:
	 * NULL: ok
	 * "": unspecified error
	 * "blabla": errors message, will be handled and translated as appropriate
	 * check: called before work (and move too?) koord3d already valid coordinate, checks visibility
	 * work / move should depend on undergroundmode for not network safe tools
	 */
	virtual const char *check_pos( player_t *, koord3d );
	virtual const char *work( player_t *, koord3d ) { return NULL; }
	virtual const char *move( player_t *, uint16 /* buttonstate */, koord3d ) { return ""; }

	/**
	 * Should be overloaded if derived class implements move,
	 * move will only be called, if this function returns true.
	 */
	virtual bool move_has_effects() const { return false;}

	/**
	 * Returns whether the 2d koordinate passed it's a valid position for this tool to highlight a tile,
	 * just takes into account is_grid_tool. It does not check if work is allowed there, that's check_pos() work.
	 * @see check_pos
	 * @return true is the coordinate it's found valid, false otherwise.
	 */
	bool check_valid_pos( koord k ) const;

	/**
	 * Specifies if the cursor will need a position update after this tool takes effect (ie: changed the height of the tile)
	 * @note only used on lower_raise tools atm.
	 * @return true if the cursor has to be moved.
	 */
	virtual bool update_pos_after_use() const { return false; }

	virtual waytype_t get_waytype() const { return invalid_wt; }
};

/*
 * Class for tools that work only on ground (kartenboden)
 */
class kartenboden_tool_t : public tool_t {
public:
	kartenboden_tool_t(uint16 const id) : tool_t(id) {}

	char const* check_pos(player_t*, koord3d) OVERRIDE;
};


/**
 * Class for tools needing two clicks (e.g. building ways).
 * Dragging is also possible.
 */
class two_click_tool_t : public tool_t {
public:
	two_click_tool_t(uint16 const id) : tool_t(id) {
		MEMZERO(start_marker);
		first_click_var = true;
	}

	void rdwr_custom_data(memory_rw_t*) OVERRIDE;
	bool init(player_t*) OVERRIDE;
	bool exit(player_t* const player) OVERRIDE { return init(player); }

	char const* work(player_t*, koord3d) OVERRIDE;
	char const* move(player_t*, uint16 /* buttonstate */, koord3d) OVERRIDE;
	bool move_has_effects() const OVERRIDE { return true; }

	bool is_work_here_keeps_game_state(player_t *, koord3d) OVERRIDE;

	/**
	 * @returns true if cleanup() needs to be called before another tool can be executed
	 * necessary for all tools that create dummy tiles for preview
	 */
	virtual bool remove_preview_necessary() const { return false; }

	bool is_first_click() const;

	/**
	 * Remove dummy grounds, remove start_marker.
	 */
	void cleanup() { cleanup(true); }

	const koord3d& get_start_pos() const { return start; }

private:
	/**
	 * Remove dummy grounds, remove start_marker if @p delete_start_marker is true.
	 */
	void cleanup(bool delete_start_marker );

	/*
	 * This routine should fill marked_tiles.
	 */
	virtual void mark_tiles( player_t *, const koord3d &start, const koord3d &end ) = 0;

	/*
	 * This routine is called, if the real work should be done.
	 * If the tool supports single clicks, end is sometimes == koord3d::invalid.
	 * Returned string is passed by work/move.
	 */
	virtual const char *do_work( player_t *, const koord3d &start, const koord3d &end ) = 0;

	/*
	 * Can the tool start/end on pos? If it is the second click, start is the position of the first click
	 * 0 = no
	 * 1 = This tool can work on this tile (with single click)
	 * 2 = On this tile can dragging start/end
	 * 3 = Both (1 and 2)
	 * error will contain an error message (if this is != NULL, return value should be 0).
	 */
	virtual uint8 is_valid_pos( player_t *, const koord3d &pos, const char *&error, const koord3d &start ) = 0;

	virtual image_id get_marker_image() const;

	bool first_click_var;
	koord3d start;

	zeiger_t *start_marker;

protected:
	virtual void start_at( koord3d &new_start );

	slist_tpl< zeiger_t* > marked;
};

/* toolbar are a new overclass */
class toolbar_t : public tool_t {
protected:
	const char *helpfile;
	tool_selector_t *tool_selector;
	slist_tpl<tool_t *>tools;
public:
	toolbar_t(uint16 const id, char const* const t, char const* const h) : tool_t(id)
	{
		default_param = t;
		helpfile = h;
		tool_selector = NULL;
	}
	char const* get_tooltip(player_t const*) const OVERRIDE { return translator::translate(default_param); }
	tool_selector_t *get_tool_selector() const { return tool_selector; }
	image_id get_icon(player_t*) const OVERRIDE;
	bool is_selected() const OVERRIDE;
	bool is_init_keeps_game_state() const OVERRIDE { return true; }
	bool is_work_keeps_game_state() const OVERRIDE { return true; }
	// show this toolbar
	bool init(player_t*) OVERRIDE;
	// close this toolbar
	bool exit(player_t*) OVERRIDE;
	virtual void update(player_t *); // just refresh content
	void append(tool_t *tool) { tools.append(tool); }
};

#define MAX_LAST_TOOLS (10)

class toolbar_last_used_t : public toolbar_t {
private:
	slist_tpl<tool_t *>all_tools[MAX_PLAYER_COUNT];
public:
	toolbar_last_used_t(uint16 const id, char const* const t, char const* const h) : toolbar_t(id,t,h) {}
	static toolbar_last_used_t *last_used_tools;
	void update(player_t *) OVERRIDE; // just refresh content
	void append(tool_t *, player_t *);
	void clear();
};

// create new instance of tool
tool_t *create_tool(int toolnr);

#endif
