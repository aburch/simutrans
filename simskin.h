#ifndef __SIMSKIN_H
#define __SIMSKIN_H

template<class T> class slist_tpl;
class skin_besch_t;


class skinverwaltung_t {
public:
	enum skintyp_t { nothing, menu, cursor, symbol, misc, field };

	// new menu system
	static const skin_besch_t *werkzeuge_general;
	static const skin_besch_t *werkzeuge_simple;
	static const skin_besch_t *werkzeuge_dialoge;
	static const skin_besch_t *werkzeuge_toolbars;

	/**
	 * Window skin images (menus too!)
	 * @author Hj. Malthaner
	 */
	static const skin_besch_t *window_skin;

	// symbols
	static const skin_besch_t *logosymbol;
	static const skin_besch_t *biglogosymbol;
	static const skin_besch_t *neujahrsymbol;
	static const skin_besch_t *neueweltsymbol;
	static const skin_besch_t *flaggensymbol;
	static const skin_besch_t *meldungsymbol;

	// for the stops in the list
	static const skin_besch_t *zughaltsymbol;
	static const skin_besch_t *autohaltsymbol;
	static const skin_besch_t *schiffshaltsymbol;
	static const skin_besch_t *airhaltsymbol;
	static const skin_besch_t *monorailhaltsymbol;
	static const skin_besch_t *maglevhaltsymbol;
	static const skin_besch_t *narrowgaugehaltsymbol;
	static const skin_besch_t *bushaltsymbol;
	static const skin_besch_t *tramhaltsymbol;

	// haltstatus
	static const skin_besch_t *passagiere;
	static const skin_besch_t *post;
	static const skin_besch_t *waren;

	// for the attraction/factory
	static const skin_besch_t *electricity;
	static const skin_besch_t *intown;

	// seasons
	static const skin_besch_t *seasons_icons;

	// dialog options
	static const skin_besch_t *message_options;
	static const skin_besch_t *color_options;

	// cursors
	static const skin_besch_t *cursor_general;
	static const skin_besch_t *mouse_cursor;	// for allegro: Emulate mouse cursor
	static const skin_besch_t *belegtzeiger;
	static const skin_besch_t *bauigelsymbol;

	// misc images
	static const skin_besch_t *construction_site;
	static const skin_besch_t *fussweg;
	static const skin_besch_t *pumpe;	// since those are no buildings => change to buildings soon
	static const skin_besch_t *senke;
	static const skin_besch_t *tunnel_texture;

	static slist_tpl<const skin_besch_t *>extra_obj;

	static bool register_besch(skintyp_t type, const skin_besch_t *besch);
	static bool alles_geladen(skintyp_t type);

	static const skin_besch_t *get_extra( const char *str, int len );
};

#endif
