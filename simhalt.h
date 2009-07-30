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
#include "simware.h"

#include "simdings.h"
#include "simtypes.h"

#include "bauer/warenbauer.h"

#include "besch/ware_besch.h"

#include "dataobj/koord.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/quickstone_hashtable_tpl.h"
#include "tpl/koordhashtable_tpl.h"
#include "tpl/fixed_list_tpl.h"
#include "tpl/binary_heap_tpl.h"
#include "tpl/minivec_tpl.h"

#define MAX_HALT_COST				8 // Total number of cost items
#define MAX_MONTHS					12 // Max history
#define MAX_HALT_NON_MONEY_TYPES	7 // number of non money types in HALT's financial statistic
#define HALT_ARRIVED				0 // the amount of ware that arrived here
#define HALT_DEPARTED				1 // the amount of ware that has departed from here
#define HALT_WAITING				2 // the amount of ware waiting
#define HALT_HAPPY					3 // number of happy passangers
#define HALT_UNHAPPY				4 // number of unhappy passangers
#define HALT_NOROUTE				5 // number of no-route passangers
#define HALT_CONVOIS_ARRIVED        6 // number of convois arrived this month
#define HALT_TOO_SLOW		        7 // The number of passengers whose estimated journey time exceeds their tolerance.

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

public:
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
	
	// Data on direct connexions from one station to the next.
	// @author: jamespetts
	struct connexion
	{
		// Times in tenths of minutes
		uint16 journey_time;
		uint16 waiting_time;
		
		// Convoy only used if line not used 
		// (i.e., if the best route involves using a convoy without a line)
		linehandle_t best_line;
		convoihandle_t best_convoy;
		// TODO: Think about whether to have an array of best lines/convoys
		// Note: this is difficult because of the way in which the best line/
		// convoy is selected.

		// TODO: Consider whether to add comfort

		// Used for the memory pool only.
#ifdef USE_INDEPENDENT_PATH_POOL
		connexion* link;
		connexion() { link = NULL; }
#endif

		// For the memory pool
		void* operator new(size_t size);
		void operator delete(void *p);
	};

	// Data on paths to ultimate destinations
	struct path
	{
		halthandle_t halt;
		uint16 journey_time;
#ifdef USE_INDEPENDENT_PATH_POOL
		path* link;
#endif
		//TODO: Consider whether to add comfort

		path() { journey_time = 65535; }

		// Necessary for sorting in a binary heap
		inline bool operator <= (const path &p) const { return journey_time <= p.journey_time; }

		// For the memory pool
		void* operator new(size_t size);
		void operator delete(void *p);
	};

	// Added by : Knightly
	// For use during path search
	struct path_node
	{
		halthandle_t halt;
		uint16 journey_time;
		union
		{
			path* link;
			path_node* node_link;
		};
		linehandle_t previous_best_line;
		convoihandle_t previous_best_convoy;

		path_node() { journey_time = 65535; link = NULL; }

		// Necessary for sorting in a binary heap
		inline bool operator <= (const path_node &p_node) const { return journey_time <= p_node.journey_time; }

		// For the memory pool
		void* operator new(size_t size);
		void operator delete(void *p);
	};

	const slist_tpl<tile_t> &get_tiles() const { return tiles; };

private:
	slist_tpl<tile_t> tiles;

	koord init_pos;	// for halt without grounds, created during game initialisation

	// Memory pool for paths and connexions
#ifdef USE_INDEPENDENT_PATH_POOL
	static bool first_run_path;
	static bool first_run_connexion;
	static bool first_run_path_node;
	static const uint16 chunk_quantity;
	static path* head_path;
	static path_node* head_path_node;
	static connexion* head_connexion;

	inline static void path_pool_push(path* p);
	inline static path* path_pool_pop();

	inline static void path_node_pool_push(path_node* p);
	inline static path_node* path_node_pool_pop();

	inline static void connexion_pool_push(connexion* p);
	inline static connexion* connexion_pool_pop();
#endif
	// Table of all direct connexions to this halt, with routing information.
	// Array: one entry per goods type.
	// Knightly : Change into an array of pointers to connexion hash tables
	quickstone_hashtable_tpl<haltestelle_t, connexion*> **connexions;

	quickstone_hashtable_tpl<haltestelle_t, path*> *paths;

	// The number of iterations of paths currently traversed. Used for
	// detecting when max_transfers has been reached.
	uint32 *iterations;

	// loest warte_menge ab
	// "solves wait mixes off" (Babelfish); "solves warte volume from" (Google)
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
	// uint8 reroute_counter;						// the reroute goods

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

	// Number of passengers for whom the shortest journey
	// exceeded their time tolerance.
	// @author: jamespetts
	uint32 pax_too_slow;

	/**
	 * Haltestellen werden beim warenrouting markiert. Jeder durchgang
	 * hat eine eindeutige marke
	 *
	 * "Stops are at the routing were highlighted. Each passage has a unique brand" (Google)
	 * @author Hj. Malthaner
	 */
	uint32 marke;

#ifdef USE_QUOTE
	// for station rating
	//const char * quote_bezeichnung(int quote, convoihandle_t cnv) const;
	const char * quote_bezeichnung(int quote) const;
#endif

	/**
	 * versucht die ware mit beriets wartender ware zusammenzufassen
	 * @author Hj. Malthaner
	 */
	bool vereinige_waren(const ware_t &ware);

	// add the ware to the internal storage, called only internally
	void add_ware_to_halt(ware_t ware, bool from_saved = false);

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

	// Record of waiting times. Takes a list of the last 16 waiting times per type of goods.
	// Getter method will need to average the waiting times. 
	// @author: jamespetts
	koordhashtable_tpl<koord, fixed_list_tpl<uint16, 16> >* waiting_times;

	// Used for pathfinding. The list is stored on the heap so that it can be re-used
	// if searching is aborted part-way through.
	// @author: jamespetts
	binary_heap_tpl<path_node*> *open_list;

	// void flush_open_list(uint8 category);
	void flush_paths(uint8 category);

	// Whether the search for the destination has completed: if so, the search will not
	// re-run unless the results are stale.
	bool *search_complete;

	// When the connexions were last recalculated. Needed for checking whether they need to
	// be recalculated again. 
	// @author: jamespetts
	uint16 *connexions_timestamp;

	// Likewise for paths
	// @author: jamespetts
	uint16 *paths_timestamp;

	uint8 check_waiting;

public:

	// Added by : Knightly
	void swap_connexions(const uint8 category, quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*>* &cxns)
	{
		// swap the connexion hashtables
		quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> *temp = connexions[category];
		connexions[category] = cxns;
		cxns = temp;

		// since this swap is equivalent to having the connexions rebuilt
		resort_freight_info = true;
	}


	void reset_connexions(uint8 category);

	/**
	* Called every 255 steps
	* will distribute the goods to changed routes (if there are any)
	* @author Hj. Malthaner
	*/
	void reroute_goods();

	// Added by : Knightly
	// Purpose	: Re-routing goods of a single ware category
	bool reroute_goods(uint8 catg);

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


	uint8 get_rebuild_destination_counter() const  { return rebuilt_destination_counter; }

	// New routing method: finds shortest route in *time*, not necessarily distance
	// @ author: jamespetts

	// Direct connexions from this station. Replaces rebuild_destinations()
	void rebuild_connexions(uint8 category);

	// Ultimate paths from this station. Packets searching for a path need only
	// grab a pre-calculated path from the hashtable generated by this method.
	void calculate_paths(const halthandle_t goal, const uint8 category);

	uint32 max_iterations;

	void rotate90( const sint16 y_size );

	spieler_t *get_besitzer() const {return besitzer_p;}

	// just for info so far
	sint64 calc_maintenance();

	bool make_public_and_join( spieler_t *sp );

	const slist_tpl<fabrik_t*>& get_fab_list() const { return fab_list; }

	/**
	 * Haltestellen messen regelmaessig die Fahrplaene pruefen
	 * @author Hj. Malthaner
	 */
	void step();

	/**
	 * Called every month/every 24 game hours
	 * @author Hj. Malthaner
	 */
	void neuer_monat();

	karte_t* get_welt() const { return welt; }

	// @author: jamespetts, although much is borrowed from suche_route
	// Returns the journey time of the best possible route from this halt. Time == 65535 when there is no route.
	uint16 find_route(ware_t &ware, const uint16 journey_time = 65535);
	minivec_tpl<halthandle_t>* build_destination_list(ware_t &ware);
	uint16 find_route(minivec_tpl<halthandle_t> *ziel_list, ware_t & ware, const uint16 journey_time = 65535);

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

	// The number of passengers for whom the shortest
	// route exceeds their time tolerance.
	// @author: jamespetts
	void add_pax_too_slow(int n);

	int get_pax_happy()    const { return pax_happy;    }
	int get_pax_no_route() const { return pax_no_route; }
	int get_pax_unhappy()  const { return pax_unhappy;  }
	int get_pax_too_slow()  const { return pax_too_slow;  }


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

	/**
	 * @returns the sum of all waiting goods (100t coal + 10
	 * passengers + 2000 liter oil = 2110)
	 * @author Markus Weber
	 */
	uint32 sum_all_waiting_goods() const;

	// true, if we accept/deliver this kind of good
	bool gibt_ab(const ware_besch_t *warentyp) const { return waren[warentyp->get_catg_index()] != NULL; }

	/* retrieves a ware packet for any destination in the list
	 * needed, if the factory in question wants to remove something
	 */
	bool recall_ware( ware_t& w, uint32 menge );

	/**
	 * holt ware ab
	 * fetches ware from (Google)
	 *
	 * @return abgeholte menge
	 * @return collected volume (Google)
	 * @author Hj. Malthaner
	 */

	ware_t hole_ab( const ware_besch_t *warentyp, uint32 menge, const schedule_t *fpl, const spieler_t *sp, convoi_t* cnv);

	/* liefert ware an. Falls die Ware zu wartender Ware dazugenommen
	 * werden kann, kann ware_t gelöscht werden! D.h. man darf ware nach
	 * aufruf dieser Methode nicht mehr referenzieren!
	 *
	 * Ware to deliver. If the goods to waiting to be taken product 
	 * can be ware_t may be deleted! I.e. we must, after calling this 
	 * method no longer refer! (Google)
	 *
	 * The second version is like the first, but will not recalculate the route
	 * This is used for inital passenger, since they already know a route
	 *
	 * @return angenommene menge
	 * @author Hj. Malthaner/prissi
	 */
	uint32 liefere_an(ware_t ware);
	uint32 starte_mit_route(ware_t ware);

	// Adding method for the new routing system. Equivalent to
	// hat_gehalten with the old system. 
	void add_connexion(const uint8 category, const convoihandle_t cnv, const linehandle_t line, const minivec_tpl<halthandle_t> &halt_list, const uint8 self_halt_idx);

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
	sint64 get_finance_history(int month, int cost_type) const { return financial_history[month][cost_type]; }

	// flags station for a crowded message at the beginning of next month
	void bescheid_station_voll() { enables |= CROWDED; status_color = COL_RED; }

	/* marks a coverage area
	* @author prissi
	*/
	void mark_unmark_coverage(const bool mark) const;

	// @author: jamespetts
	// Returns the proportion of unhappy people of the total of
	// happy and unhappy people.
	float get_unhappy_proportion(uint8 month) const { return financial_history[month][HALT_HAPPY] > 0 ? (float)financial_history[month][HALT_UNHAPPY] / (float)(financial_history[month][HALT_HAPPY] + financial_history[month][HALT_UNHAPPY]) : 0; }
 
	// Getting and setting average waiting times in minutes
	// @author: jamespetts
	uint16 get_average_waiting_time(halthandle_t halt, uint8 category) const;

	void add_waiting_time(uint16 time, halthandle_t halt, uint8 category)
	{
		if(halt.is_bound())
		{
			
			fixed_list_tpl<uint16, 16> *tmp;
			if(waiting_times[category].access(halt->get_basis_pos()) == NULL)
			{
				tmp = new fixed_list_tpl<uint16, 16>;
				waiting_times[category].put(halt->get_basis_pos(), *tmp);
			}
			waiting_times[category].access(halt->get_basis_pos())->add_to_tail(time);
		}
	
	}

	inline uint16 get_waiting_minutes(uint32 waiting_ticks) const;

	quickstone_hashtable_tpl<haltestelle_t, connexion*>* get_connexions(uint8 c); 

	// Finds the best path from here to the goal halt.
	// Looks up the paths in the hashtable - if the table
	// is not stale, and the path is in it, use that, or else
	// search for a new path.
	path* get_path_to(halthandle_t goal, uint8 category);

	linehandle_t get_preferred_line(halthandle_t transfer, uint8 category) const;
	convoihandle_t get_preferred_convoy(halthandle_t transfer, uint8 category) const;

	// Set to true if a schedule that serves this stop has changed since
	// the connexions were last recalculated. Used for spreading the load
	// of the recalculating connexions algorithm with the pathfinding 
	// algorithm.
	// @author: jamespetts
	bool *reschedule;

	// Added by : Knightly
	// Purpose	: To keep track of any need for re-routing existing goods packets in the halt
	bool *reroute;

	// Makes the paths recalculate, even if it would not otherwise be time for
	// them to do so. Does this by making sure that the timestamp is lower than
	// the counter.
	// @author: jamespetts
	void force_paths_stale(const uint8 category);

	// Added by		: Knightly
	// Adapted from : Jamespetts' code
	// Purpose		: To notify relevant halts to rebuild connexions and to notify all halts to recalculate paths
	// @jamespetts: modified the code to combine with previous method and provide options about partially delayed refreshes for performance.

	static void refresh_routing(const schedule_t *const sched, const minivec_tpl<uint8> &categories, const spieler_t *const player, const uint8 path_option);

	// Added by		: Knightly
	// Adpated from : rebuild_connexions()
	// Purpose		: To create a list of reachable halts with a line/convoy
	// Return		: -1 if self halt is not found; or position of self halt in halt list if found
	// Caution		: halt_list will be overwritten
	sint16 create_reachable_halt_list(const schedule_t *const sched, const spieler_t *const sched_owner, minivec_tpl<halthandle_t> &halt_list);

	// Added by : Knightly
	// Purpose	: To keep track if pathing data structures are present or not
	bool has_pathing_data_structures;

	// Added by : Knightly
	// Purpose	: For all halts, check if pathing data structures are present; if not, create and initialize them
	static void prepare_pathing_data_structures();


	// Added by		: Knightly
	// Adapted from : haltestelle_t::add_connexion()
	// Purpose		: Create goods list of specified goods category if it is not already present
	void prepare_goods_list(uint8 category)
	{
		if ( waren[category] == NULL ) 
		{
			// indicates that this can route those goods
			waren[category] = new vector_tpl<ware_t>(0);
		}
	}

};
#endif
