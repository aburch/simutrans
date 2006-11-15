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
class tunnel_besch_t;
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

	static const kreuzung_besch_t *gib_kreuzung(const waytype_t ns, const waytype_t ow);

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
	static const weg_besch_t *  weg_search(const waytype_t wtyp,const uint32 speed_limit,const uint16 time=0);

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
			const waytype_t wtyp,
			int (* wz1)(spieler_t *, karte_t *, koord, value_t),
			const int sound_ok,
			const int sound_ko,
			const uint16 time=0,
			uint8 styp = 0);


	enum bautyp_t {
		strasse=1,
		schiene=2,
		schiene_tram=3, // Dario: Tramway
		monorail=4,
		maglev=6,
		wasser=7,
		luft=8,
		leitung=9,
		bautyp_mask=15,
		bot_flag=32,					// do not connect to other ways
		elevated_flag=64,			// elevated structure
		tunnel_flag=128				// underground structure
	};

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
	enum bautyp_t bautyp;

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
	 * Type of bridges to build (zero=>no bridges)
	 * @author Hj. Malthaner
	 */
	const tunnel_besch_t * tunnel_besch;

	/**
	 * If a way is built on top of another way, should the type
	 * of the former way be kept or replced (true == keep)
	 * @author Hj. Malthaner
	 */
	bool keep_existing_ways;
	bool keep_existing_faster_ways;
	bool keep_existing_city_roads;

	karte_t *welt;
	uint32 maximum;    // hoechste Suchtiefe

	vector_tpl<koord3d> *route;

	inline const koord3d position_bei(unsigned i) const { return route->get(i); }

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

	long intern_calc_route(koord3d start, const koord3d ziel);
	void intern_calc_straight_route(const koord3d start, const koord3d ziel);

	// runways need to meet some special conditions enforced here
	bool intern_calc_route_runways(koord3d start, const koord3d ziel);

	ribi_t::ribi calc_ribi(int step);

	static koord3d bruecke_finde_ende(karte_t *welt, koord3d pos, koord zv);
	static koord3d tunnel_finde_ende(karte_t *welt, koord3d pos, const koord zv);

	void baue_tunnel_und_bruecken();

	// adds the ground before underground construction (always called before the following construction routines)
	bool baue_tunnelboden();

	// adds the grounds for elevated tracks
	void baue_elevated();

	void baue_strasse();
	void baue_schiene();
	void baue_leitung();

public:
	koord gib_route_bei(int i) const { return route->at(i).gib_2d(); }

	int n, max_n;

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
	void set_keep_city_roads(bool yesno) { keep_existing_city_roads = yesno; }

	void route_fuer(enum bautyp_t wt, const weg_besch_t * besch, const tunnel_besch_t *tunnel_besch=NULL, const bruecke_besch_t *bruecke_besch=NULL);

	void set_maximum(uint32 n) { maximum = n; }

	wegbauer_t(karte_t *welt, spieler_t *spl);
	~wegbauer_t();

	void calc_straight_route(koord3d start, const koord3d ziel);
	void calc_route(koord3d start3d, koord3d ziel);

	/* returns the amount needed to built this way
	* author prissi
	*/
	long calc_costs();

	bool check_crossing(const koord zv, const grund_t *bd,waytype_t wtyp) const;
	bool check_for_leitung(const koord zv, const grund_t *bd) const;

	void baue();
};

#endif
