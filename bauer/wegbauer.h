/*
 * simbau.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simbau_h
#define simbau_h

#include "../boden/wege/weg.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/array2d_tpl.h"
#include "../tpl/slist_tpl.h"
#include "../simtypes.h"


class weg_besch_t;
class kreuzung_besch_t;
class bruecke_besch_t;
class karte_t;
class spieler_t;
class grund_t;

class werkzeug_parameter_waehler_t;


/**
 * Diese Klasse übernimmt Wegsuche und Bau von Strassen, Schienen etc.
 * @author Hj. Malthaner
 */
class wegbauer_t
{
    static slist_tpl<const kreuzung_besch_t *> kreuzungen;
public:
    static const weg_besch_t *leitung_besch;

    static const kreuzung_besch_t *gib_kreuzung(const weg_t::typ ns, const weg_t::typ ow);

    static bool register_besch(const weg_besch_t *besch);
    static bool register_besch(const kreuzung_besch_t *besch);
    static bool alle_wege_geladen();
    static bool alle_kreuzungen_geladen();

    // generates timeline message
	static void neuer_monat(karte_t *welt);


    /**
     * Finds a way with a given speed limit for a given waytype
     * @author prissi
     */
    static const weg_besch_t *  weg_search(const weg_t::typ wtyp,const int speed_limit,const uint16 time=0);

    /**
     * Tries to look up description for way, described by way type,
     * system type and construction type
     * @author Hj. Malthaner
     */
    static const weg_besch_t * gib_besch(const char * way_name,const uint16 time=0);


    /**
     * Fill menu with icons of given waytype
     * @author Hj. Malthaner
     */
    static void fill_menu(werkzeug_parameter_waehler_t *wzw,
			  const weg_t::typ wtyp,
			  int (* wz1)(spieler_t *, karte_t *, koord, value_t),
			  const int sound_ok,
			  const int sound_ko,
			  const uint16 time=0,
			  uint8 styp = 0);


    enum bautyp {
        strasse,
	schiene,
	schiene_bot,
	schiene_bot_bau,
	strasse_bot,
	schiene_tram, // Dario: Tramway
	schiene_monorail,
	leitung,
	wasser,
	luft
    };

    bool kann_ribis_setzen(const grund_t *bd, const koord zv);
    static bool kann_mit_strasse_erreichen(const grund_t *bd, const koord zv);
    static bool ist_bruecke_tunnel_ok(grund_t *bd_von, koord zv, grund_t *bd_nach);

private:

	typedef struct
	{
		grund_t *gr;
		long		cost;
	} next_gr_t;
	vector_tpl<next_gr_t> next_gr;

    enum { unseen = 9999999 };
    enum { max_route_laenge = 1024 };

    spieler_t *sp;

    /**
     * Type of building operation
     * @author Hj. Malthaner
     */
    enum bautyp bautyp;

    /**
     * Type of way to build
     * @author Hj. Malthaner
     */
    const weg_besch_t * besch;

    /**
     * Type of bridges to build (zero=>no bridges)
     * @author Hj. Malthaner
     */
    const bruecke_besch_t * bruecke_besch;

    /**
     * If a way is built on top of another way, should the type
     * of the former way be kept or replced (true == keep)
     * @author Hj. Malthaner
     */
    bool keep_existing_ways;
    bool keep_existing_faster_ways;
    bool keep_existing_city_roads;

    karte_t *welt;
    int maximum;    // hoechste Suchtiefe

    vector_tpl<koord3d> *route;

    inline const koord position_bei(unsigned i) const { return route->get(i).gib_2d(); };

	// allowed owner?
	bool check_owner( const spieler_t *sp1, const spieler_t *sp2 );

	// allowed slope?
	bool check_slope( const grund_t *from, const grund_t *to );

	/* This is the core routine for the way search
	 * it will check
	 * A) allowed step
	 * B) if allowed, calculate the cost for the step from from to to
	 * @author prissi
	 */
	bool is_allowed_step( const grund_t *from, const grund_t *to, long *costs );

	// checks, if we can built a bridge here ...
	// may modify next_gr array!
	int check_for_bridge( const grund_t *parent_from, const grund_t *from, koord3d ziel );

    long intern_calc_route(koord start, const koord ziel);
    void intern_calc_straight_route(const koord start, const koord ziel);

	// runways need to meet some special conditions enforced here
    bool intern_calc_route_runways(koord start, const koord ziel);

    ribi_t::ribi calc_ribi(int step);

    static koord3d bruecke_finde_ende(karte_t *welt, koord3d pos, koord zv);
    static koord3d tunnel_finde_ende(karte_t *welt, koord3d pos, const koord zv);

    void baue_tunnel_und_bruecken();

    void baue_strasse();
    void baue_schiene();
    void baue_leitung();
    void baue_monorail();
    void baue_kanal();
    void baue_runway();

public:


    koord gib_route_bei(int i) const {return route->at(i).gib_2d();};

    int n,max_n;

    bool baubaer;


    /**
     * If a way is built on top of another way, should the type
     * of the former way be kept or replced (true == keep)
     * @author Hj. Malthaner
     */
    void set_keep_existing_ways(bool yesno);

    /* If a way is built on top of another way, should the type
     * of the former way be kept or replaced, if the current way is faster (true == keep)
     * @author Hj. Malthaner
     */
    void set_keep_existing_faster_ways(bool yesno);

    /* Always keep city roads (for AI)
     * @author prissi
     */
    void set_keep_city_roads(bool yesno) {keep_existing_city_roads=yesno;};


    void route_fuer(enum bautyp wt, const weg_besch_t * besch, const bruecke_besch_t *bruecke_besch=NULL);

    void set_maximum(int n);

    wegbauer_t(karte_t *welt, spieler_t *spl);
    ~wegbauer_t();


    void calc_straight_route(koord start, const koord ziel);
    void calc_route(koord start, const koord ziel);

	/* returns the amount needed to built this way
	 * author prissi
	 */
	long calc_costs();

  bool check_crossing(const koord zv, const grund_t *bd,weg_t::typ wtyp) const;
  bool check_for_leitung(const koord zv, const grund_t *bd) const;

/* built a corrdinate list
 * @author prissi
 */
void baue_strecke( slist_tpl <koord> &list );

    void baue();
};

#endif
