#ifndef __SIMSKIN_H
#define __SIMSKIN_H

#include "simcolor.h"

// Max Kielland
// Classic helper macro to transform a #define value into a "string"
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// For test purposes themes can be disabled or an alternative theme.tab file can be used.
//  -1 = No theme, use interrnal fallback
//   0 = Normal use, read theme.tab
// n>0 = Use alternative file named theme_n.tab
#define THEME 0

template<class T> class slist_tpl;
class skin_besch_t;

enum skin_images_t {
	SKIN_WINDOW_BACKGROUND, // 0
	SKIN_GADGET_CLOSE,
	SKIN_GADGET_HELP,
	SKIN_GADGET_MINIMIZE,
	SKIN_BUTTON_PREVIOUS,
	SKIN_BUTTON_NEXT,
	SKIN_BUTTON_CHECKBOX,
	SKIN_BUTTON_CHECKBOX_CHECKED,
	SKIN_BUTTON_ARROW_LEFT,
	SKIN_BUTTON_ARROW_LEFT_PRESSED,
	SKIN_BUTTON_ARROW_RIGHT, // 10
	SKIN_BUTTON_ARROW_RIGHT_PRESSED,
	SKIN_BUTTON_SIDE_LEFT,
	SKIN_BUTTON_SIDE_RIGHT,
	SKIN_BUTTON_BODY,
	SKIN_BUTTON_SIDE_LEFT_PRESSED,
	SKIN_BUTTON_SIDE_RIGHT_PRESSED,
	SKIN_BUTTON_BODY_PRESSED,
	SKIN_BUTTON_ARROW_UP,
	SKIN_BUTTON_ARROW_UP_PRESSED,
	SKIN_BUTTON_ARROW_DOWN, // 20
	SKIN_BUTTON_ARROW_DOWN_PRESSED,
	SKIN_GADGET_NOTPINNED,
	SKIN_GADGET_PINNED,
	SKIN_SCROLLBAR_H_BACKGROUND_LEFT,
	SKIN_SCROLLBAR_H_BACKGROUND_RIGHT,
	SKIN_SCROLLBAR_H_BACKGROUND,
	SKIN_SCROLLBAR_H_KNOB_LEFT,
	SKIN_SCROLLBAR_H_KNOB_RIGHT,
	SKIN_SCROLLBAR_H_KNOB_BODY,
	SKIN_SCROLLBAR_V_BACKGROUND_TOP, // 30
	SKIN_SCROLLBAR_V_BACKGROUND_BOTTOM,
	SKIN_SCROLLBAR_V_BACKGROUND,
	SKIN_SCROLLBAR_V_KNOB_TOP,
	SKIN_SCROLLBAR_V_KNOB_BOTTOM,
	SKIN_SCROLLBAR_V_KNOB_BODY,
	SKIN_WINDOW_RESIZE,
	SKIN_GADGET_GOTO,
	SKIN_GADGET_BUTTON
};

class skinverwaltung_t {
public:
	enum skintyp_t { nothing, menu, cursor, symbol, misc };

	/// @name system colours used by gui components
	/// @{
	static COLOR_VAL theme_color_highlight;           //@< Colour to draw highlighs in dividers, buttons etc... MN_GREY4
	static COLOR_VAL theme_color_shadow;              //@< Colour to draw shadows in dividers, buttons etc...   MN_GREY0
	static COLOR_VAL theme_color_face;                //@< Colour to draw surface in buttons etc...             MN_GREY2
	static COLOR_VAL theme_color_button_text;         //@< Colour to draw button text
	static COLOR_VAL theme_color_text;                //@< Colour to draw interactive text checkbox etc...
	static COLOR_VAL theme_color_static_text;         //@< Colour to draw non interactive text in labels etc...
	static COLOR_VAL theme_color_disabled_text;       //@< Colour to draw disabled text in buttons etc...

	// Not implemented yet
	static COLOR_VAL theme_color_selected_text;       //@< Colour to draw selected text in edit box etc...
	static COLOR_VAL theme_color_selected_background; //@< Colour to draw selected text background in edit box etc...
	/// @}

	/// @name icons used in the toolbars
	/// @{

	/// icon images for advanced tools (everything that involves clicking on the map)
	static const skin_besch_t *werkzeuge_general;
	/// icon images for simple tools (eg pause, fast forward)
	static const skin_besch_t *werkzeuge_simple;
	/// icon images for GUI tools (which open windows)
	static const skin_besch_t *werkzeuge_dialoge;
	/// icon images for toolbars
	static const skin_besch_t *werkzeuge_toolbars;
	/// @}

	/**
	 * Window skin images (menus too!)
	 * @author Hj. Malthaner
	 */
	static const skin_besch_t *window_skin;

	/// @name pictures used in the GUI
	/// @{

	/// image shown in welcome screen @see banner.cc
	static const skin_besch_t *logosymbol;
	/// image shown when loading pakset
	static const skin_besch_t *biglogosymbol;
	/// image shown with 'happy new year' message
	static const skin_besch_t *neujahrsymbol;
	/// image shown when creating new world
	static const skin_besch_t *neueweltsymbol;
	/// image shown in language selection
	static const skin_besch_t *flaggensymbol;
	/// image shown in message boxes @see gui/messagebox.h
	static const skin_besch_t *meldungsymbol;
	/// image shown in message options window
	static const skin_besch_t *message_options;
	/// image shown in color selection window
	static const skin_besch_t *color_options;
	/// @}

	/// @name icons used for the tabs in the line management window
	/// @{
	static const skin_besch_t *zughaltsymbol;
	static const skin_besch_t *autohaltsymbol;
	static const skin_besch_t *schiffshaltsymbol;
	static const skin_besch_t *airhaltsymbol;
	static const skin_besch_t *monorailhaltsymbol;
	static const skin_besch_t *maglevhaltsymbol;
	static const skin_besch_t *narrowgaugehaltsymbol;
	static const skin_besch_t *bushaltsymbol;
	static const skin_besch_t *tramhaltsymbol;
	///@}

	/// @name icons shown in status bar at the bottom of the screen
	/// @{
	static const skin_besch_t *networksymbol;
	static const skin_besch_t *timelinesymbol;
	static const skin_besch_t *fastforwardsymbol;
	static const skin_besch_t *pausesymbol;
	static const skin_besch_t *seasons_icons;
	/// @}

	/// @name icons used to show which halt serves passengers / mail / freight
	/// @{
	static const skin_besch_t *passagiere;
	static const skin_besch_t *post;
	static const skin_besch_t *waren;
	/// @}

	/// images shown in display of lines in mini-map
	static const skin_besch_t *station_type;

	/// image to indicate power supply of factories
	static const skin_besch_t *electricity;
	/// image to indicate that an attraction is inside a town (attraction list window)
	static const skin_besch_t *intown;


	/// @name cursors
	/// @{

	/// cursors for tools
	static const skin_besch_t *cursor_general;
	/// for allegro: emulate mouse cursor
	static const skin_besch_t *mouse_cursor;
	/// symbol to mark tiles by AI players and text labels
	static const skin_besch_t *belegtzeiger;
	/// icon to mark start tiles for construction (ie the bulldozer image)
	static const skin_besch_t *bauigelsymbol;
	/// @}

	/// shown in hidden-buildings mode instead of buildings images
	static const skin_besch_t *construction_site;
	/// texture to be shown beneath city roads to indicate pavements
	static const skin_besch_t *fussweg;
	/// transformer image: supply
	static const skin_besch_t *pumpe;
	/// transformer image: consumer
	static const skin_besch_t *senke;
	/// texture to be shown beneath ways in tunnel
	static const skin_besch_t *tunnel_texture;

	static bool register_besch(skintyp_t type, const skin_besch_t *besch);
	static bool alles_geladen(skintyp_t type);

	/**
	 * retrieves objects with type=menu and given name
	 * @param str pointer to beginning of name string (not null-terminated)
	 * @param len length of string
	 * @return pointer to skin object or NULL if nothing found
	 */
	static const skin_besch_t *get_extra( const char *str, int len );

private:
	/// holds objects from paks with type 'menu'
	static slist_tpl<const skin_besch_t *>extra_obj;
};

#endif
