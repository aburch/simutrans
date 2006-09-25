#ifndef __SIMSKIN_H
#define __SIMSKIN_H

class skin_besch_t;

class skinverwaltung_t {
public:
    enum skintyp_t { nothing, menu, cursor, symbol, misc };

    static const skin_besch_t *hauptmenu;
    static const skin_besch_t *schienen_werkzeug;
    static const skin_besch_t *schiffs_werkzeug;
    static const skin_besch_t *hang_werkzeug;
    static const skin_besch_t *special_werkzeug;
    static const skin_besch_t *listen_werkzeug;
    static const skin_besch_t *farbmenu;
    static const skin_besch_t *edit_werkzeug;

    static const skin_besch_t *logosymbol;
    static const skin_besch_t *biglogosymbol;
    static const skin_besch_t *neujahrsymbol;
    static const skin_besch_t *neueweltsymbol;
    static const skin_besch_t *flaggensymbol;
    static const skin_besch_t *meldungsymbol;
    static const skin_besch_t *bauigelsymbol;

    // for the stops in the list
    static const skin_besch_t *zughaltsymbol;
    static const skin_besch_t *autohaltsymbol;
    static const skin_besch_t *schiffshaltsymbol;
    static const skin_besch_t *airhaltsymbol;
    static const skin_besch_t *monorailhaltsymbol;
    static const skin_besch_t *bushaltsymbol;

    static const skin_besch_t *fragezeiger;
    static const skin_besch_t *signalzeiger;
    static const skin_besch_t *downzeiger;
    static const skin_besch_t *upzeiger;
    static const skin_besch_t *belegtzeiger;
    static const skin_besch_t *killzeiger;
    static const skin_besch_t *slopezeiger;
    static const skin_besch_t *fahrplanzeiger;
    static const skin_besch_t *werftNSzeiger;
    static const skin_besch_t *werftOWzeiger;
    static const skin_besch_t *stadtzeiger;
    static const skin_besch_t *baumzeiger;
    static const skin_besch_t *undoc_zeiger;
    static const skin_besch_t *mouse_cursor;	// for allegro: Emulate mouse cursor

    static const skin_besch_t *construction_site;
    static const skin_besch_t *fussweg;
    static const skin_besch_t *pumpe;
    static const skin_besch_t *senke;

	// for the attraction/factory
    static const skin_besch_t *electricity;
    static const skin_besch_t *intown;

	// haltstatus
    static const skin_besch_t *passagiere;
    static const skin_besch_t *post;
    static const skin_besch_t *waren;

	// seasons
    static const skin_besch_t *seasons_icons;

    static const skin_besch_t *message_options;

    /**
     * Window skin images
     * @author Hj. Malthaner
     */
    static const skin_besch_t *window_skin;

    static bool register_besch(skintyp_t type, const skin_besch_t *besch);
    static bool alles_geladen(skintyp_t type);

};

#endif // __SIMSKIN_H
