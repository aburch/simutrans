/*
 * Copyright (c) 2008 prissi
 *
 * This file is part of the Simutrans project under the artistic license.
 *
 * New configurable OOP tool system
 */

#ifndef simmenu_h
#define simmenu_h

#include <string>
#include "besch/sound_besch.h"

#include "dataobj/translator.h"

#include "simtypes.h"
#include "simworld.h"


template<class T> class vector_tpl;
template<class T> class slist_tpl;

class werkzeug_waehler_t;
class spieler_t;
class toolbar_t;
class memory_rw_t;

enum {
	// general tools
	WKZ_ABFRAGE=0,
	WKZ_REMOVER,
	WKZ_RAISE_LAND,
	WKZ_LOWER_LAND,
	WKZ_SETSLOPE,
	WKZ_RESTORESLOPE,
	WKZ_MARKER,
	WKZ_CLEAR_RESERVATION,
	WKZ_TRANSFORMER,
	WKZ_ADD_CITY,
	WKZ_CHANGE_CITY_SIZE,
	WKZ_PLANT_TREE,
	WKZ_FAHRPLAN_ADD,
	WKZ_FAHRPLAN_INS,
	WKZ_WEGEBAU,
	WKZ_BRUECKENBAU,
	WKZ_TUNNELBAU,
	WKZ_WAYREMOVER,
	WKZ_WAYOBJ,
	WKZ_STATION,
	WKZ_ROADSIGN,
	WKZ_DEPOT,
	WKZ_BUILD_HAUS,
	WKZ_LAND_CHAIN,
	WKZ_CITY_CHAIN,
	WKZ_BUILD_FACTORY,
	WKZ_LINK_FACTORY,
	WKZ_HEADQUARTER,
	WKZ_LOCK_GAME,
	WKZ_ADD_CITYCAR,
	WKZ_FOREST,
	WKZ_STOP_MOVER,
	WKZ_MAKE_STOP_PUBLIC,
	WKZ_REMOVE_WAYOBJ,
	WKZ_SLICED_AND_UNDERGROUND_VIEW,
	WKZ_BUY_HOUSE,
	WKZ_CITYROAD,
	WKZ_ERR_MESSAGE_TOOL,
	GENERAL_TOOL_COUNT,
	GENERAL_TOOL = 0x1000
};

enum {
	// simple one click tools
	WKZ_PAUSE = 0,
	WKZ_FASTFORWARD,
	WKZ_SCREENSHOT,
	WKZ_INCREASE_INDUSTRY,
	WKZ_UNDO,
	WKZ_SWITCH_PLAYER,
	WKZ_STEP_YEAR,
	WKZ_CHANGE_GAME_SPEED,
	WKZ_ZOOM_IN,
	WKZ_ZOOM_OUT,
	WKZ_SHOW_COVERAGE,
	WKZ_SHOW_NAMES,
	WKZ_SHOW_GRID,
	WKZ_SHOW_TREES,
	WKZ_SHOW_HOUSES,
	WKZ_SHOW_UNDERGROUND,
	WKZ_ROTATE90,
	WKZ_QUIT,
	WKZ_FILL_TREES,
	WKZ_DAYNIGHT_LEVEL,
	WKZ_VEHICLE_TOOLTIPS,
	WKZ_TOOGLE_PAX,
	WKZ_TOOGLE_PEDESTRIANS,
	WKZ_TRAFFIC_LEVEL,
	WKZ_CONVOI_TOOL,
	WKZ_LINE_TOOL,
	WKZ_DEPOT_TOOL,
	UNUSED_WKZ_PWDHASH_TOOL,
	WKZ_SET_PLAYER_TOOL,
	WKZ_TRAFFIC_LIGHT_TOOL,
	WKZ_CHANGE_CITY_TOOL,
	WKZ_RENAME_TOOL,
	WKZ_ADD_MESSAGE_TOOL,
	WKZ_TOGGLE_RESERVATION,
	WKZ_VIEW_OWNER,
	WKZ_HIDE_UNDER_CURSOR,
	SIMPLE_TOOL_COUNT,
	SIMPLE_TOOL = 0x2000
};

enum {
	// dialoge tools
	WKZ_HELP = 0,
	WKZ_OPTIONEN,
	WKZ_MINIMAP,
	WKZ_LINEOVERVIEW,
	WKZ_MESSAGES,
	WKZ_FINANCES,
	WKZ_PLAYERS,
	WKZ_DISPLAYOPTIONS,
	WKZ_SOUND,
	WKZ_LANGUAGE,
	WKZ_PLAYERCOLOR,
	WKZ_JUMP,
	WKZ_LOAD,
	WKZ_SAVE,
	WKZ_LIST_HALT,
	WKZ_LIST_CONVOI,
	WKZ_LIST_TOWN,
	WKZ_LIST_GOODS,
	WKZ_LIST_FACTORY,
	WKZ_LIST_CURIOSITY,
	WKZ_EDIT_FACTORY,
	WKZ_EDIT_ATTRACTION,
	WKZ_EDIT_HOUSE,
	WKZ_EDIT_TREE,
	WKZ_ENLARGE_MAP,
	WKZ_LIST_LABEL,
	WKZ_CLIMATES,
	WKZ_SETTINGS,
	WKZ_GAMEINFO,
	DIALOGE_TOOL_COUNT,
	DIALOGE_TOOL = 0x4000
};

enum {
	// toolbars
	WKZ_MAINMENU = 0,
	TOOLBAR_TOOL = 0x8000u
};

class werkzeug_t {
protected:
	image_id icon;
private:
	/* value to trigger this command (see documentation) */
	uint16 id;

protected:
	const char *default_param;
public:
	uint16 get_id() const { return id; }

	static werkzeug_t *dummy;

	// for key loockup
	static vector_tpl<werkzeug_t *>char_to_tool;

	/// cursor image
	image_id cursor;

	/// cursor marks this area
	koord cursor_area;

	/// cursor centered at marked area? default: false
	bool cursor_centered;

	/// z-offset of cursor, possible values: Z_PLAN and Z_GRID
	sint8 offset;

	sint16 ok_sound;

	enum {
		WFL_SHIFT  = 1, ///< shift-key was pressed when mouse-click happened
		WFL_CTRL   = 2, ///< ctrl-key was pressed when mouse-click happened
		WFL_LOCAL  = 4, ///< tool call was issued by local client
		WFL_SCRIPT = 8  ///< tool call was issued by script (no password checks)
	};
	uint8 flags; // flags are set before init/work/move is called

	bool is_ctrl_pressed()    const { return flags & WFL_CTRL; }
	bool is_shift_pressed()   const { return flags & WFL_SHIFT; }
	bool is_local_execution() const { return flags & WFL_LOCAL; }
	bool is_scripted()        const { return flags & WFL_SCRIPT; }

	uint16 command_key;// key to toggle action for this function

	static vector_tpl<werkzeug_t *> general_tool;
	static vector_tpl<werkzeug_t *> simple_tool;
	static vector_tpl<werkzeug_t *> dialog_tool;
	static vector_tpl<toolbar_t *> toolbar_tool;

	static void update_toolbars(karte_t *welt);

	// since only a single toolstr a time can be visible ...
	static char toolstr[1024];

	static void init_menu();
	static void exit_menu();

	static void read_menu(const std::string &objfilename);

	static uint16 const dummy_id = 0xFFFFU;

	werkzeug_t(uint16 const id) : id(id), cursor_area(1,1) { cursor = icon = IMG_LEER; ok_sound = NO_SOUND; offset = Z_PLAN; default_param = NULL; command_key = 0; cursor_centered = false;}
	virtual ~werkzeug_t() {}

	virtual image_id get_icon(spieler_t *) const { return icon; }
	void set_icon(image_id i) { icon = i; }

	// returns default_param of this tool for player sp
	// if sp==NULL returns default_param that was used to create the tool
	virtual const char* get_default_param(spieler_t* = NULL) const { return default_param; }
	void set_default_param(const char* str) { default_param = str; }

	// transfer additional information in networkgames
	virtual void rdwr_custom_data(uint8 /* player_nr */, memory_rw_t*) { }

	// this will draw the tool with some indication, if active
	virtual bool is_selected(const karte_t *welt) const;

	// when true, local execution would do no harm
	virtual bool is_init_network_save() const { return false; }
	virtual bool is_move_network_save(spieler_t *) const { return true; }

	// if is_work_network_save()==false
	// and is_work_here_network_save(...)==false
	// then work-command is sent over network
	virtual bool is_work_network_save() const { return false; }
	virtual bool is_work_here_network_save(karte_t *, spieler_t *, koord3d) { return false; }

	// will draw a dark frame, if selected
	virtual void draw_after( karte_t *w, koord pos ) const;

	virtual const char *get_tooltip(const spieler_t *) const { return NULL; }

	/**
	 * Returning false on init will automatically invoke previous tool.
	 * Returning true will select tool and will make it possible to call work.
	 */
	virtual bool init( karte_t *, spieler_t * ) { return true; }

	/// initializes cursor (icon, marked area)
	void init_cursor( zeiger_t * ) const;

	// returning true on exit will have werkzeug_waehler resets to query-tool on right-click
	virtual bool exit( karte_t *, spieler_t * ) { return true; }

	/* the return string can have different meanings:
	 * NULL: ok
	 * "": unspecified error
	 * "blabla": errors message, will be handled and translated as appropriate
	 * check: called before work (and move too?) koord3d already valid coordinate, checks visibility
	 * work / move should depend on undergroundmode for not network safe tools
	 */
	virtual const char *check_pos( karte_t *, spieler_t *, koord3d );
	virtual const char *work( karte_t *, spieler_t *, koord3d ) { return NULL; }
	virtual const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d ) { return ""; }

	virtual waytype_t get_waytype() const { return invalid_wt; }
};

/*
 * Class for tools that work only on ground (kartenboden)
 */
class kartenboden_werkzeug_t : public werkzeug_t {
public:
	kartenboden_werkzeug_t(uint16 const id) : werkzeug_t(id) {}

	char const* check_pos(karte_t*, spieler_t*, koord3d) OVERRIDE;
};

/*
 * Class for tools needing two clicks (e.g. building ways).
 * Dragging is also possible.
 * @author Gerd Wachsmuth
 */
class two_click_werkzeug_t : public werkzeug_t {
public:
	two_click_werkzeug_t(uint16 const id) : werkzeug_t(id) {
		MEMZERO(start_marker);
	}

	void rdwr_custom_data(uint8 player_nr, memory_rw_t*) OVERRIDE;
	bool init(karte_t*, spieler_t*) OVERRIDE;
	bool exit(karte_t* const welt, spieler_t* const sp) OVERRIDE { return init(welt, sp); }

	char const* work(karte_t*, spieler_t*, koord3d) OVERRIDE;
	char const* move(karte_t*, spieler_t*, uint16 /* buttonstate */, koord3d) OVERRIDE;

	bool is_work_here_network_save(karte_t*, spieler_t *, koord3d) OVERRIDE;

	/**
	 * @returns true if cleanup() needs to be called before another tool can be executed
	 * necessary for all tools that create dummy tiles for preview
	 */
	virtual bool remove_preview_necessary() const { return false; }

	bool is_first_click() const;
	void cleanup(bool delete_start_marker );

private:

	/*
	 * This routine should fill marked_tiles.
	 */
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &start, const koord3d &end ) = 0;

	/*
	 * This routine is called, if the real work should be done.
	 * If the tool supports single clicks, end is sometimes == koord3d::invalid.
	 * Returned string is passed by work/move.
	 */
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &start, const koord3d &end ) = 0;

	/*
	 * Can the tool start/end on pos? If it is the second click, start is the position of the first click
	 * 0 = no
	 * 1 = This tool can work on this tile (with single click)
	 * 2 = On this tile can dragging start/end
	 * 3 = Both (1 and 2)
	 * error will contain an error message (if this is != NULL, return value should be 0).
	 */
	virtual uint8 is_valid_pos( karte_t *, spieler_t *, const koord3d &pos, const char *&error, const koord3d &start ) = 0;

	virtual image_id get_marker_image();

	bool first_click_var;
	koord3d start;
	void start_at( karte_t *, koord3d &new_start );

	zeiger_t *start_marker;

protected:
	slist_tpl< zeiger_t* > marked;
};

/* toolbar are a new overclass */
class toolbar_t : public werkzeug_t {
public:
	// size of icons
	koord iconsize;
private:
	const char *helpfile;
	werkzeug_waehler_t *wzw;
	slist_tpl<werkzeug_t *>tools;
public:
	toolbar_t(uint16 const id, char const* const t, char const* const h, koord const size) : werkzeug_t(id)
	{
		default_param = t;
		helpfile = h;
		wzw = NULL;
		iconsize = size;
	}
	char const* get_tooltip(spieler_t const*) const OVERRIDE { return translator::translate(default_param); }
	werkzeug_waehler_t *get_werkzeug_waehler() const { return wzw; }
	image_id get_icon(spieler_t*) const OVERRIDE;
	bool is_selected(karte_t const*) const OVERRIDE;
	bool is_init_network_save() const OVERRIDE { return true; }
	bool is_work_network_save() const OVERRIDE { return true; }
	// show this toolbar
	bool init(karte_t*, spieler_t*) OVERRIDE;
	// close this toolbar
	bool exit(karte_t*, spieler_t*) OVERRIDE;
	void update(karte_t *, spieler_t *);	// just refresh content
	void append(werkzeug_t *w) { tools.append(w); }
};

// create new instance of tool
werkzeug_t *create_tool(int toolnr);

#endif
