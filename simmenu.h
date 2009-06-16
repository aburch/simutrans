/*
 * Copyright (c) 2008 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 * New configurable OOP tool system
 */

#ifndef simmenu_h
#define simmenu_h

#include "besch/sound_besch.h"

#include "dataobj/translator.h"

#include "simtypes.h"
#include "simworld.h"

template<class T> class vector_tpl;
template<class T> class slist_tpl;

class werkzeug_waehler_t;
class karte_t;
class spieler_t;
class toolbar_t;

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

public:
	static werkzeug_t *dummy;

	// for sorting: compare tool key
	static bool compare_werkzeug( const werkzeug_t *a, const werkzeug_t *b) {
		uint16 ac = a->command_key & ~32;
		uint16 bc = b->command_key & ~32;
		return ac==bc ? a->command_key < b->command_key : ac < bc;
	}

	// for key loockup
	static vector_tpl<werkzeug_t *>char_to_tool;

	image_id cursor;
	int ok_sound;
	int failed_sound;
	sint8 offset;
	const char *default_param;

	uint16 command_key;// key to toggle action for this function
	uint16 id;			// value to trigger this command (see documentation)

	static vector_tpl<werkzeug_t *> general_tool;
	static vector_tpl<werkzeug_t *> simple_tool;
	static vector_tpl<werkzeug_t *> dialog_tool;
	static vector_tpl<toolbar_t *> toolbar_tool;

	static void update_toolbars(karte_t *welt);

	// since only a single toolstr a time can be visible ...
	static char toolstr[1024];

	static void init_menu(cstring_t objfilename);

	werkzeug_t() { id = 0xFFFFu; cursor = icon = IMG_LEER; ok_sound = failed_sound = NO_SOUND; offset = Z_PLAN; default_param = NULL; command_key = 0; }
	virtual ~werkzeug_t() {}

	virtual image_id get_icon(spieler_t *) const { return icon; }
	void set_icon(image_id i) { icon = i; }

	// this will draw the tool with some indication, if active
	virtual bool is_selected(karte_t *welt) const { return welt->get_werkzeug()==this; }

	// will draw a dark frame, if selected
	virtual void draw_after( karte_t *w, koord pos ) const;

	/* could be used for player dependent images
	 * will be called, when a toolbar is opened/updated
	 * return false to avoid inclusion
	 */
	virtual bool update_image(spieler_t *) { return true; }

	virtual const char *get_tooltip(spieler_t *) { return NULL; }

	// returning false on init will automatically invoke previous tool
	virtual bool init( karte_t *, spieler_t * ) { return true; }
	virtual bool exit( karte_t *, spieler_t * ) { return true; }

	/* the return string can have different meanings:
	 * NULL: ok
	 * "": unspecified error
	 * "balbal": errors message, will be handled and translated as appropriate
	 */
	virtual const char *work( karte_t *, spieler_t *, koord3d ) { return NULL; }
	virtual const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d ) { return ""; }
};

/*
 * Class for tools needing two clicks (e.g. building ways).
 * Dragging is also possible.
 * @author Gerd Wachsmuth
 */
class two_click_werkzeug_t : public werkzeug_t {
public:
	two_click_werkzeug_t() : werkzeug_t() { start_marker = NULL; };

	virtual bool init( karte_t *, spieler_t * );
	virtual bool exit( karte_t *welt, spieler_t *sp ) { return init( welt, sp ); }

	virtual const char *work( karte_t *, spieler_t *, koord3d );
	virtual const char *move( karte_t *, spieler_t *, uint16 /* buttonstate */, koord3d );

private:
	/*
	 * These two routines have to be implemented in inherited classes.
	 * mark_tiles: fill marked_tiles.
	 * work: do the real work.
	 * returned string is passed by work/move.
	 */
	virtual void mark_tiles( karte_t *, spieler_t *, const koord3d &start, const koord3d &end ) = 0;
	virtual const char *do_work( karte_t *, spieler_t *, const koord3d &start, const koord3d &end ) = 0;

	/*
	 * Can the tool start/end on this koord3d?
	 * NULL = yes, other return values are passed by work/move.
	 */
	virtual const char *valid_pos( karte_t *, spieler_t *, const koord3d &start ) = 0;

	virtual image_id get_marker_image();

	bool first_click;
	koord3d start;
	void start_at( karte_t *, spieler_t *, koord3d &new_start );
	void cleanup( bool delete_start_marker );

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
	toolbar_t( const char *t, const char *h, koord size ) : werkzeug_t()
	{
		default_param = t;
		helpfile = h;
		wzw = NULL;
		iconsize = size;
	}
	const char *get_tooltip(spieler_t *) { return translator::translate(default_param); }
	werkzeug_waehler_t *get_werkzeug_waehler() const { return wzw; }
	virtual image_id get_icon(spieler_t *);
	bool is_selected(karte_t *welt) const;
	// show this toolbar
	virtual bool init(karte_t *w, spieler_t *sp);
	void update(karte_t *, spieler_t *);	// just refresh content
	void append(werkzeug_t *w) { tools.append(w); }
};

#endif
