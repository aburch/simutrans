/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simhalt_h
#define simhalt_h

#include "convoihandle_t.h"
#include "linehandle_t.h"
#include "halthandle_t.h"

#include "simdings.h"
#include "simtypes.h"

#include "bauer/warenbauer.h"

#include "besch/ware_besch.h"

#include "dataobj/koord.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"


#define RESCHEDULING (1)
#define REROUTING (2)

#define MAX_HALT_COST   7 // Total number of cost items
#define MAX_MONTHS     12 // Max history
#define MAX_HALT_NON_MONEY_TYPES 7 // number of non money types in HALT's financial statistic
#define HALT_ARRIVED   0 // the amount of ware that arrived here
#define HALT_DEPARTED 1 // the amount of ware that has departed from here
#define HALT_WAITING		2 // the amount of ware waiting
#define HALT_HAPPY		3 // number of happy passangers
#define HALT_UNHAPPY		4 // number of unhappy passangers
#define HALT_NOROUTE         5 // number of no-route passangers
#define HALT_CONVOIS_ARRIVED             6 // number of convois arrived this month

class cbuffer_t;
class grund_t;
class fabrik_t;
class karte_t;
class koord3d;
#ifdef LAGER_NOT_IN_USE
class lagerhaus_t;
#endif
class loadsave_t;
class schedule_t;
class spieler_t;
class ware_t;

// -------------------------- Haltestelle ----------------------------

/**
 * Haltestellen in Simutrans. Diese Klasse managed das Routing und Verladen
 * von Waren. Eine Haltestelle ist somit auch ein Warenumschlagplatz.
 *
 * @author Hj. Malthaner
 * @see stadt_t
 * @see fabrik_t
 * @see convoi_t
 */
class haltestelle_t
{
public:
	enum station_flags { NOT_ENABLED=0, PAX=1, POST=2, WARE=4, CROWDED=8 };

	enum routine_result_flags { NO_ROUTE=0, ROUTE_OK=1, ROUTE_OVERCROWDED=8 };

	//13-Jan-02     Markus Weber    Added
	enum stationtyp {invalid=0, loadingbay=1, railstation = 2, dock = 4, busstop = 8, airstop = 16, monorailstop = 32, tramstop = 64, maglevstop=128, narrowgaugestop=256 }; //could be combined with or!

private:
	/**
	 * Manche Methoden müssen auf alle Haltestellen angewandt werden
	 * deshalb verwaltet die Klasse eine Liste aller Haltestellen
	 * @author Hj. Malthaner
	 */
	static slist_tpl<halthandle_t> alle_haltestellen;

	/**
	 * finds a stop by its name
	 * @author prissi
	 */
	static stringhashtable_tpl<halthandle_t> all_names;

	/*
	 * struct holds new financial history for line
	 * @author hsiegeln
	 */
	sint64 financial_history[MAX_MONTHS][MAX_HALT_COST];

	/**
	 * initialize the financial history
	 * @author hsiegeln
	 */
	void init_financial_history();

	uint8 status_color;
	uint32 capacity[3]; // passenger, post, goods
	uint8 overcrowded[8];	// bit set, when overcrowded
	void recalc_status();

	static uint8 status_step;	// NONE or SCHEDULING or REROUTING

public:
	/**
	 * Handles changes of schedules and the resulting rerouting
	 */
	static void step_all();

	static uint8 get_rerouting_status() { return status_step; }

	/**
	 * Tries to generate some pedestrians on the sqaure and the
	 * adjacent sqaures. Return actual number of generated
	 * pedestrians.
	 *
	 * @author Hj. Malthaner
	 */
	static int erzeuge_fussgaenger(karte_t *welt, const koord3d pos, int anzahl);

	/* searches for a stop at the given koordinate
	 * @return halthandle_t(), if nothing found
	 * @author prissi
	 */
	static halthandle_t get_halt(karte_t *welt, const koord pos, const spieler_t *sp );

	/* since we allow only for a single stop per planquadrat
	 * this will always return something even if there is not stop some of the ground level
	 */
	static halthandle_t get_halt( karte_t *welt, const koord3d pos, const spieler_t *sp );

	static const slist_tpl<halthandle_t>& get_alle_haltestellen() { return alle_haltestellen; }

	/**
	 * Station factory method. Returns handles instead of pointers.
	 * @author Hj. Malthaner
	 */
	static halthandle_t create(karte_t *welt, koord pos, spieler_t *sp);

	/**
	 * Station factory method. Returns handles instead of pointers.
	 * @author Hj. Malthaner
	 */
	static halthandle_t create(karte_t *welt, loadsave_t *file);

	/*
	* removes a ground tile from a station, deletes the building and, if last tile, also the halthandle
	* @author prissi
	*/
	static bool remove(karte_t *welt, spieler_t *sp, koord3d pos, const char *&msg);

	/**
	 * Station destruction method.
	 * @author Hj. Malthaner
	 */
	static void destroy(halthandle_t &halt);

	/**
	 * destroys all stations
	 * @author Hj. Malthaner
	 */
	static void destroy_all(karte_t *);

private:
	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * @author Hj. Malthaner
	 */
	halthandle_t self;

public:
	/**
	 * Liste aller felder (Grund-Objekte) die zu dieser Haltestelle gehören
	 * @author Hj. Malthaner
	 */
	struct tile_t
	{
		tile_t(grund_t* grund_) : grund(grund_) {}

		bool operator ==(const tile_t& o) { return grund == o.grund; }
		bool operator !=(const tile_t& o) { return grund != o.grund; }

		grund_t*       grund;
		convoihandle_t reservation;
	};

	const slist_tpl<tile_t> &get_tiles() const { return tiles; };

private:
	slist_tpl<tile_t> tiles;

	koord init_pos;	// for halt without grounds, created during game initialisation

	// List with all reachable destinations
	vector_tpl<halthandle_t>* warenziele;

	/**
	 * For each schedule/line, that adds halts to a warenziel array,
	 * this counter is incremented. Each ware category needs a separate
	 * counter. If this counter is more than 1, this halt is a transfer
	 * halt, i.e. contains non_identical_schedules with overlapping
	 * destinations.
	 * Non-transfer stops do not need to be searched for connections
	 * => large speedup possible. (Vector would better though.)
	 * @author Knightly
	 */
	uint8 *non_identical_schedules;

	// Array with different categries that contains all waiting goods at this stop
	vector_tpl<ware_t> **waren;

	/**
	 * Liste der angeschlossenen Fabriken
	 * @author Hj. Malthaner
	 */
	slist_tpl<fabrik_t *> fab_list;

	spieler_t *besitzer_p;
	static karte_t *welt;

	/**
	 * What is that for a station (for the image)
	 * @author prissi
	 */
	stationtyp station_type;

	uint8 rebuilt_destination_counter;	// new schedule, first rebuilt destinations asynchroniously
	uint8 reroute_counter;						// the reroute goods
	// since we do partial routing, we remeber the last offset
	uint8 last_catg_index;
	uint32 last_ware_index;

	/* station flags (most what enabled) */
	uint8 enables;

	void set_pax_enabled(bool yesno)  { yesno ? enables |= PAX  : enables &= ~PAX;  }
	void set_post_enabled(bool yesno) { yesno ? enables |= POST : enables &= ~POST; }
	void set_ware_enabled(bool yesno) { yesno ? enables |= WARE : enables &= ~WARE; }

	/**
	 * Found route and station uncrowded
	 * @author Hj. Malthaner
	 */
	uint32 pax_happy;

	/**
	 * Found no route
	 * @author Hj. Malthaner
	 */
	uint32 pax_no_route;

	/**
	 * Station crowded
	 * @author Hj. Malthaner
	 */
	uint32 pax_unhappy;

	/**
	 * Haltestellen werden beim warenrouting markiert. Jeder durchgang
	 * hat eine eindeutige marke
	 * @author Hj. Malthaner
	 */
	uint32 marke;

#ifdef USE_QUOTE
	// for station rating
	const char * quote_bezeichnung(int quote) const;
#endif

	/**
	 * versucht die ware mit beriets wartender ware zusammenzufassen
	 * @author Hj. Malthaner
	 */
	bool vereinige_waren(const ware_t &ware);

	// add the ware to the internal storage, called only internally
	void add_ware_to_halt(ware_t ware);

	/**
	 * liefert wartende ware an eine Fabrik
	 * @author Hj. Malthaner
	 */
	void liefere_an_fabrik(const ware_t& ware);

	/*
	 * transfers all goods to given station
	 *
	 * @author Dwachs
	 */
	void transfer_goods(halthandle_t halt);

	/*
	* parameter to ease sorting
	* sortierung is local and stores the sortorder for the individual station
	* @author hsiegeln
	*/
	uint8 sortierung;
	bool resort_freight_info;

	haltestelle_t(karte_t *welt, loadsave_t *file);
	haltestelle_t(karte_t *welt, koord pos, spieler_t *sp);
	~haltestelle_t();

public:
	/**
	* Called after schedule calculation of all stations is finished
	* will distribute the goods to changed routes (if there are any)
	* returns true upon completion
	* @author Hj. Malthaner
	*/
	bool reroute_goods(sint16 &units_remaining);

	/**
	 * getter/setter for sortby
	 * @author hsiegeln
	 */
	uint8 get_sortby() { return sortierung; }
	void set_sortby(uint8 sm) { resort_freight_info =true; sortierung = sm; }

	/**
	 * Calculates a status color for status bars
	 * @author Hj. Malthaner
	 */
	COLOR_VAL get_status_farbe() const { return status_color; }

	/**
	 * Draws some nice colored bars giving some status information
	 * @author Hj. Malthaner
	 */
	void display_status(sint16 xpos, sint16 ypos) const;

	/**
	 * sucht umliegende, erreichbare fabriken und baut daraus die
	 * Fabrikliste auf.
	 * @author Hj. Malthaner
	 */
	void verbinde_fabriken();
	void remove_fabriken(fabrik_t *fab);

	/**
	 * Rebuilds the list of reachable destinations
	 * returns the search number of connections
	 * @author Hj. Malthaner
	 */
	sint32 rebuild_destinations();

	uint8 get_rebuild_destination_counter() const  { return rebuilt_destination_counter; }

	void rotate90( const sint16 y_size );

	spieler_t *get_besitzer() const {return besitzer_p;}

	// just for info so far
	sint64 calc_maintenance();

	void make_public_and_join( spieler_t *sp );

	const vector_tpl<halthandle_t> *get_warenziele_passenger() const {return warenziele;}
	const vector_tpl<halthandle_t> *get_warenziele_mail() const {return warenziele+1;}

	// returns the matchin warenziele
	const vector_tpl<halthandle_t> *get_warenziele(uint8 catg_index) const {return warenziele+catg_index;}

	const slist_tpl<fabrik_t*>& get_fab_list() const { return fab_list; }

	/**
	 * called regularily to update status and reroute stuff
	 * @author Hj. Malthaner
	 */
	bool step(sint16 &units_remaining);

	/**
	 * Called every month/every 24 game hours
	 * @author Hj. Malthaner
	 */
	void neuer_monat();

	karte_t* get_welt() const { return welt; }

	/**
	 * Kann die Ware nicht zum Ziel geroutet werden (keine Route), dann werden
	 * Ziel und Zwischenziel auf koord::invalid gesetzt.
	 *
	 * @param ware die zu routende Ware
	 * @author Hj. Malthaner
	 *
	 * for reverse routing, also the next to last stop can be added, if next_to_ziel!=NULL
	 *
	 * if avoid_overcrowding is set, a valid route in only found when there is no overflowing stop in between
	 *
	 * @author prissi
	 */
	int suche_route( ware_t &ware, koord *next_to_ziel, bool avoid_overcrowding );

	int get_pax_enabled()  const { return enables & PAX;  }
	int get_post_enabled() const { return enables & POST; }
	int get_ware_enabled() const { return enables & WARE; }

	// check, if we accepts this good
	// often called, thus inline ...
	int is_enabled( const ware_besch_t *wtyp ) {
		if(wtyp==warenbauer_t::passagiere) {
			return enables&PAX;
		}
		else if(wtyp==warenbauer_t::post) {
			return enables&POST;
		}
		return enables&WARE;
	}

	/**
	 * Found route and station uncrowded
	 * @author Hj. Malthaner
	 */
	void add_pax_happy(int n);

	/**
	 * Found no route
	 * @author Hj. Malthaner
	 */
	void add_pax_no_route(int n);

	/**
	 * Station crowded
	 * @author Hj. Malthaner
	 */
	void add_pax_unhappy(int n);

	int get_pax_happy()    const { return pax_happy;    }
	int get_pax_no_route() const { return pax_no_route; }
	int get_pax_unhappy()  const { return pax_unhappy;  }


#ifdef LAGER_NOT_IN_USE
	void set_lager(lagerhaus_t* l) { lager = l; }
#endif

	bool add_grund(grund_t *gb);
	bool rem_grund(grund_t *gb);

	uint32 get_capacity(uint8 typ) const { return capacity[typ]; }

	bool existiert_in_welt();

	koord get_init_pos() const { return init_pos; }
	koord get_basis_pos() const;
	koord3d get_basis_pos3d() const;

	/* return the closest square that belongs to this halt
	 * @author prissi
	 */
	koord get_next_pos( koord start ) const;

	// true, if this station is overcroded for this category
	bool is_overcrowded( const uint8 idx ) const { return (overcrowded[idx/8] & (1<<(idx%8)))!=0; }

	/**
	 * gibt Gesamtmenge derware vom typ typ zurück
	 * @author Hj. Malthaner
	 */
	uint32 get_ware_summe(const ware_besch_t *warentyp) const;

	/**
	 * returns total number for a certain position (since more than one factory might connect to a stop)
	 * @author Hj. Malthaner
	 */
	uint32 get_ware_fuer_zielpos(const ware_besch_t *warentyp, const koord zielpos) const;

	/**
	 * gibt Gesamtmenge derw are vom typ typ fuer zwischenziel zurück
	 * @author prissi
	 */
	uint32 get_ware_fuer_zwischenziel(const ware_besch_t *warentyp, const halthandle_t zwischenziel) const;

	// true, if we accept/deliver this kind of good
	bool gibt_ab(const ware_besch_t *warentyp) const { return waren[warentyp->get_catg_index()] != NULL; }

	/* retrieves a ware packet for any destination in the list
	 * needed, if the factory in question wants to remove something
	 */
	bool recall_ware( ware_t& w, uint32 menge );

	/**
	 * holt ware ab
	 * @return abgeholte menge
	 * @author Hj. Malthaner
	 */
	ware_t hole_ab( const ware_besch_t *warentyp, uint32 menge, const schedule_t *fpl, const spieler_t *sp );

	/* liefert ware an. Falls die Ware zu wartender Ware dazugenommen
	 * werden kann, kann ware_t gelöscht werden! D.h. man darf ware nach
	 * aufruf dieser Methode nicht mehr referenzieren!
	 *
	 * The second version is like the first, but will not recalculate the route
	 * This is used for inital passenger, since they already know a route
	 *
	 * @return angenommene menge
	 * @author Hj. Malthaner/prissi
	 */
	uint32 liefere_an(ware_t ware);
	uint32 starte_mit_route(ware_t ware);

	/**
	 * wird von Fahrzeug aufgerufen, wenn dieses an der Haltestelle
	 * gehalten hat.
	 * @param typ der beförderte warentyp
	 * @author Hj. Malthaner
	 */
	void hat_gehalten( const ware_besch_t *warentyp, const schedule_t *fpl, const spieler_t *sp );

	const grund_t *find_matching_position(waytype_t wt) const;

	/* checks, if there is an unoccupied loading bay for this kind of thing
	* @author prissi
	*/
	bool find_free_position(const waytype_t w ,convoihandle_t cnv,const ding_t::typ d) const;

	/* reserves a position (caution: railblocks work differently!
	* @author prissi
	*/
	bool reserve_position(grund_t *gr,convoihandle_t cnv);

	/* frees a reserved  position (caution: railblocks work differently!
	* @author prissi
	*/
	bool unreserve_position(grund_t *gr, convoihandle_t cnv);

	/* true, if this can be reserved
	* @author prissi
	*/
	bool is_reservable(const grund_t *gr, convoihandle_t cnv) const;

	void info(cbuffer_t & buf) const;

	/**
	 * @param buf the buffer to fill
	 * @return Goods description text (buf)
	 * @author Hj. Malthaner
	 */
	void get_freight_info(cbuffer_t & buf);

	/**
	 * @param buf the buffer to fill
	 * @return short list of the waiting goods (i.e. 110 Wood, 15 Coal)
	 * @author Hj. Malthaner
	 */
	void get_short_freight_info(cbuffer_t & buf);

	/**
	 * Opens an information window for this station.
	 * @author Hj. Malthaner
	 */
	void zeige_info();

	/**
	 * @returns the type of a station
	 * (combination of: railstation, loading bay, dock)
	 * @author Markus Weber
	 */
	stationtyp get_station_type() const { return station_type; }
	void recalc_station_type();

	/**
	 * fragt den namen der Haltestelle ab.
	 * Der Name ist der text des ersten Untergrundes der Haltestelle
	 * @return der Name der Haltestelle.
	 * @author Hj. Malthaner
	 */
	const char *get_name() const;

	void set_name(const char *name);

	// create an unique name: better to be called with valid handle, althoug it will work without
	char *create_name(const koord k, const char *typ);

	void rdwr(loadsave_t *file);

	void laden_abschliessen();

	/*
	 * called, if a line serves this stop
	 * @author hsiegeln
	 */
	void add_line(linehandle_t line) { registered_lines.append_unique(line); }

	/*
	 * called, if a line removes this stop from it's schedule
	 * @author hsiegeln
	 */
	void remove_line(linehandle_t line) { registered_lines.remove(line); }

	/*
	 * list of line ids that serve this stop
	 * @author hsiegeln
	 */
	vector_tpl<linehandle_t> registered_lines;

	/**
	 * book a certain amount into the halt's financial history
	 * @author hsiegeln
	 */
	void book(sint64 amount, int cost_type);

	/**
	 * return a pointer to the financial history
	 * @author hsiegeln
	 */
	sint64* get_finance_history() { return *financial_history; }

	/**
	 * return a specified element from the financial history
	 * @author hsiegeln
	 */
	sint64 get_finance_history(int month, int cost_type) { return financial_history[month][cost_type]; }

	// flags station for a crowded message at the beginning of next month
	void bescheid_station_voll() { enables |= CROWDED; status_color = COL_RED; }

	/* marks a coverage area
	* @author prissi
	*/
	void mark_unmark_coverage(const bool mark) const;

	/*
	* deletes factory references so map rotation won't segfault
	*/
	void release_factory_links();
};
#endif
