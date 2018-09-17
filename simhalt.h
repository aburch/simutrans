/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simhalt_h
#define simhalt_h

#include "convoihandle_t.h"
#include "linehandle_t.h"
#include "halthandle_t.h"
#include "simware.h"

#include "simobj.h"
#include "display/simgraph.h"
#include "simtypes.h"
#include "simconst.h"

#include "bauer/goods_manager.h"

#include "descriptor/goods_desc.h"

#include "dataobj/koord.h"

#include "tpl/inthashtable_tpl.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/binary_heap_tpl.h"

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
#define HALT_HAPPY					3 // number of happy passengers
#define HALT_UNHAPPY				4 // number of unhappy passengers
#define HALT_NOROUTE				5 // number of no-route passengers
#define HALT_CONVOIS_ARRIVED        6 // number of convois arrived this month
#define HALT_TOO_SLOW		        7 // The number of passengers whose estimated journey time exceeds their tolerance.
/* NOTE - Standard has HALT_WALKED here as no. 7. In Extended, this is in cities, not stops.*/

// This should be network safe multi-threadedly (this has been considered carefully and tested somewhat,
// although the test was run at a time (March 2017) when there was another known bug, hard to find, causing
// network desyncs when multiple clients connect to a server, so the test is not a perfect proof of being
// network safe)
// It is faster enabled than disabled. 
#define ALWAYS_CACHE_SERVICE_INTERVAL

class cbuffer_t;
class grund_t;
class fabrik_t;
class karte_t;
class karte_ptr_t;
class koord3d;
class loadsave_t;
class schedule_t;
class player_t;
class ware_t;
class transferring_cargo_t;

// elements of the lines_loaded vector
struct lines_loaded_t
{
	linehandle_t line;
	bool reversed;
	uint8 current_stop;
	uint8 catg_index;
};

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
	 * Manche Methoden müssen auf all Haltestellen angewandt werden
	 * deshalb verwaltet die Klasse eine Liste aller Haltestellen
	 * @author Hj. Malthaner
	 */
	static vector_tpl<halthandle_t> alle_haltestellen;

	/**
	 * finds a stop by its name
	 * @author prissi
	 */
	static stringhashtable_tpl<halthandle_t> all_names;

	/**
	 * Finds a stop by coordinate.
	 * only used during loading.
	 * @author prissi
	 */
	static inthashtable_tpl<sint32,halthandle_t> *all_koords;
	
	/**
	 * A list of lines and freight categories that have already been loaded with all available freight at the halt.
	 * Reset each step.
	 */
	static vector_tpl<lines_loaded_t> lines_loaded;

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

	COLOR_VAL status_color, last_status_color;
	sint16 last_bar_count;
	vector_tpl<KOORD_VAL> last_bar_height; // caches the last height of the station bar for each good type drawn in display_status(). used for dirty tile management
	uint32 capacity[3]; // passenger, mail, goods
	uint8 overcrowded[256/8]; ///< bit field for each goods type (max 256)

	slist_tpl<convoihandle_t> loading_here;
	sint32 last_loading_step;

	// A list of halts within walking distance
	// @author: jamespetts, July 2011
	vector_tpl<halthandle_t> halts_within_walking_distance;

	void add_halt_within_walking_distance(halthandle_t halt);
	void remove_halt_within_walking_distance(halthandle_t halt);

	void check_nearby_halts();

	uint8 control_towers;
	koord init_pos;	// for halt without grounds, created during game initialisation

	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * @author Hj. Malthaner
	 */
	halthandle_t self;

	/*
	 * The time (in 10ths of seconds)
	 * that it takes passengers to walk
	 * through this stop from one
	 * connexion to another. This is
	 * based on the size of the stop.
	 */
	uint32 transfer_time;

	/**
	 * The time (in 10ths of seconds)
	 * that it takes goods to be trans-shipped
	 * inside this stop. This assumes a fixed
	 * rate of 1km/h and is based on the size
	 * of the stop.
	 */
	uint32 transshipment_time;

	/* This is called by the path explorer
	 * when this halt needs to re-route goods.
	 * This cannot be done from within the 
	 * path explorer when it is multi-threaded.
	 */
	vector_tpl<uint8> categories_to_refresh_next_step;

	/**
	* This is the list of passengers/mail/goods that
	* have arrived at this stop but are in the process
	* of transferring either to catch the next service,
	* or walking/being carted to their ultimate 
	* destination.
	*
	* This is an array of these vectors: one per thread,
	* indexed by thread number.
	*/
	vector_tpl<transferring_cargo_t> *transferring_cargoes;

public:
	const slist_tpl<convoihandle_t> &get_loading_convois() const { return loading_here; }

	// add convoi to loading queue
	void request_loading( convoihandle_t cnv );

	/* recalculates the station bar */
	void recalc_status();

	/**
	 * Handles changes of schedules and the resulting re-routing.
	 */
	static void step_all();

	/**
	 * Resets reconnect_counter.
	 * The next call to step_all() will start complete reconnecting.
	 */
	static void reset_routing();

	/**
	 * Returns an index to a halt at koord k
   	 * optionally limit to that owned by player player
   	 * by default create a new halt if none found
	 * Only used during loading.
	 */
	static halthandle_t get_halt_koord_index(koord k);

	/*
	 * this will only return something if this stop belongs to same player or is public, or is a dock (when on water)
	 */
	static halthandle_t get_halt(const koord3d pos, const player_t *player );
	static halthandle_t get_halt(const koord pos, const player_t *player );

//	static slist_tpl<halthandle_t>& get_alle_haltestellen() { return alle_haltestellen; }
	static const vector_tpl<halthandle_t>& get_alle_haltestellen() { return alle_haltestellen; }

	static vector_tpl<lines_loaded_t>& access_lines_loaded() { return lines_loaded; }

	/**
	 * Station factory method. Returns handles instead of pointers.
	 * @author Hj. Malthaner
	 */
	static halthandle_t create(koord pos, player_t *player);

	/**
	 * Station factory method. Returns handles instead of pointers.
	 * @author Hj. Malthaner
	 */
	static halthandle_t create(loadsave_t *file);

	/*
	* removes a ground tile from a station, deletes the building and, if last tile, also the halthandle
	* @author prissi
	*/
	static bool remove(player_t *player, koord3d pos);

	/**
	 * Station destruction method.
	 * @author Hj. Malthaner
	 */
	static void destroy(halthandle_t);

	/**
	 * destroys all stations
	 * @author Hj. Malthaner
	 */
	static void destroy_all();

	uint32 get_number_of_halts_within_walking_distance() const;

	halthandle_t get_halt_within_walking_distance(uint32 index) const { return halts_within_walking_distance[index]; }

	static uint8 pedestrian_limit;

	/**
	 * Liste aller felder (Grund-Objekte) die zu dieser Haltestelle gehören
	 * @author Hj. Malthaner
	 */
	struct tile_t
	{
		tile_t() {}
		tile_t(grund_t* grund_) : grund(grund_) {}

		bool operator ==(const tile_t& o) { return grund == o.grund; }
		bool operator !=(const tile_t& o) { return grund != o.grund; }

		grund_t*       grund;
		convoihandle_t reservation[2];
	};

	// Data on direct connexions from one station to the next.
	// @author: jamespetts
	struct connexion
	{
		// Times in tenths of minutes
		uint32 journey_time;
		uint32 waiting_time;
		uint32 transfer_time;

		// Convoy only used if line not used
		// (i.e., if the best route involves using a convoy without a line)
		linehandle_t best_line;
		convoihandle_t best_convoy;

		// TODO: Consider whether to add comfort

		uint16 alternative_seats; // used in overcrowd calculations in fetch_goods, updated by update_alternative_seats

		// For the memory pool
		void* operator new(size_t size);
		void operator delete(void *p);
	};
	bool do_alternative_seats_calculation; //for optimisations purpose

	const slist_tpl<tile_t> &get_tiles() const { return tiles; };

	bool is_within_walking_distance_of(halthandle_t halt) const;

	typedef quickstone_hashtable_tpl<haltestelle_t, connexion*> connexions_map;

	struct waiting_time_set
	{
		fixed_list_tpl<uint32, 32> times;
		uint8 month;
	};

	typedef inthashtable_tpl<uint32, waiting_time_set > waiting_time_map;

	void add_control_tower() { control_towers ++; }
	void remove_control_tower() { if(control_towers > 0) control_towers --; }

	struct service_frequency_specifier
	{
		// Category
		uint16 x;
		// Halt ID
		uint8 y;

		service_frequency_specifier operator - (service_frequency_specifier s)
		{
			service_frequency_specifier result;
			result.x = x - s.x;
			result.y = y - s.y;
			return result;
		}
	};

	bool is_transfer(const uint8 catg) const { return non_identical_schedules[catg] > 1u; }
//	bool is_transfer(const uint8 catg) const { return all_links[catg].is_transfer; }

	typedef inthashtable_tpl<uint16, sint64> arrival_times_map;
#ifdef MULTI_THREAD
	uint32 get_transferring_cargoes_count() const;
#else
	uint32 get_transferring_cargoes_count() const { return transferring_cargoes->get_count(); }
#endif

private:
	slist_tpl<tile_t> tiles;

	// Table of all direct connexions to this halt, with routing information.
	// Array: one entry per goods type.
	// Knightly : Change into an array of pointers to connexion hash tables
	connexions_map **connexions;

	// loest warte_menge ab
	// "solves wait mixes off" (Babelfish); "solves warte volume from" (Google)

	/**
	 * For each line/lineless convoy which serves the current halt, this
	 * counter is incremented. Each ware category needs a separate counter.
	 * If this counter is more than 1, this halt is a transfer halt.

	 * @author Knightly
	 */
	uint8 *non_identical_schedules;


	// Array with different categories that contains all waiting goods at this stop
	vector_tpl<ware_t> **cargo;

	/**
	 * Liste der angeschlossenen Fabriken
	 * @author Hj. Malthaner
	 */
	slist_tpl<fabrik_t *> fab_list;

	player_t *owner;
	static karte_ptr_t welt;

	/**
	 * What is that for a station (for the image)
	 * @author prissi
	 */
	stationtyp station_type;

	// private helper function for recalc_station_type()
	void add_to_station_type( grund_t *gr );

	/**
	 * Reconnect and reroute if counter different from welt->get_schedule_counter()
	 */
	static uint8 reconnect_counter;
	// since we do partial routing, we remember the last offset

	// since we do partial routing, we remeber the last offset
	uint8 last_catg_index;
	uint32 last_goods_index;

	/* station flags (most what enabled) */
	uint16 enables;

	/**
	* 0 = North; 1 = South; 2 = East 3 = West
	* Used for the time interval system
	*/
	sint64 train_last_departed[4]; 

	/**
	* Used for the time interval system
	*/
	vector_tpl<koord3d> station_signals;

#ifdef USE_QUOTE
	// for station rating
	//const char * quote_bezeichnung(int quote, convoihandle_t cnv) const;
	const char * quote_bezeichnung(int quote) const;
#endif

	// add the ware to the internal storage, called only internally
	void add_ware_to_halt(ware_t ware, bool from_saved = false);

	// Add cargoes to the waiting list
	void add_to_waiting_list(ware_t ware, sint64 ready_time);

	/**
	* This calculates how long that it will take any given
	* cargo packet to be ready to transfer onwards from
	* this stop.
	*/
	sint64 calc_ready_time(ware_t ware, bool arriving_from_vehicle, koord origin_pos = koord::invalid) const;

	/*
	 * transfers all goods to given station
	 *
	 * @author Dwachs
	 */
	void transfer_goods(halthandle_t halt);

	/*
	* parameter to ease sorting
	* sortierung is local and stores the sort order for the individual station
	* @author hsiegeln
	*/
	uint8 sortierung;
	bool resort_freight_info;

	haltestelle_t(loadsave_t *file);
	haltestelle_t(koord pos, player_t *player);
	~haltestelle_t();

	// Record of waiting times. Takes a list of the last 16 waiting times per type of goods.
	// Getter method will need to average the waiting times.
	// @author: jamespetts

	waiting_time_map **waiting_times;

	// Store the service frequencies to all other halts so that this does not need to be
	// recalculated frequently. These are used as proxies for waiting times when no
	// recent (or any) waiting time data are available. 
	koordhashtable_tpl<service_frequency_specifier, uint32> service_frequencies;

	static const sint64 waiting_multiplication_factor = 3ll;
	static const sint64 waiting_tolerance_ratio = 50ll;

	uint8 check_waiting;

	// Added by : Knightly
	// Purpose	: To store the time at which this halt is created
	//			  This is *not* saved in save games.
	//			  When loading halts from save game, this is set to 0
	sint64 inauguration_time;

	/**
	* Arrival times of convoys bound for this stop, estimated based on 
	* convoys' point to point timings, indexed by the convoy's ID
	*/
	arrival_times_map estimated_convoy_arrival_times;
	arrival_times_map estimated_convoy_departure_times;

	/**
	 * Calculates the earliest time in ticks that passengers/mail/goods can arrive
	 * at the given halt in light of the current estimated departure times.
	 */
	sint64 calc_earliest_arrival_time_at(halthandle_t halt, convoihandle_t &convoi, uint8 catg_index, uint8 g_class) const;

	/**
	* This will check the list of transferring cargoes
	* and register at their destination those that
	* are ready to go there.
	*/
	void check_transferring_cargoes();

public:
#ifdef DEBUG_SIMRAND_CALLS
	bool loading;
	vector_tpl<ware_t> *get_warray(uint8 catg) { return goods[catg]; }
#endif

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

	// Added by : Knightly
	uint8 get_schedule_count(const uint8 category) const { return non_identical_schedules[category]; }
	void set_schedule_count(const uint8 category, const uint8 schedule_count) { non_identical_schedules[category] = schedule_count; }

	void reset_connexions(uint8 category);

	/**
	* Called every 255 steps
	* will distribute the goods to changed routes (if there are any)
	* @author Hj. Malthaner
	*/
	//uint32 reroute_goods();

	// Added by : Knightly
	// Purpose	: Re-routing goods of a single ware category
	uint32 reroute_goods(uint8 catg);


	/**
	 * getter/setter for sortby
	 * @author hsiegeln
	 */
	uint8 get_sortby() { return sortierung; }
	void set_sortby(uint8 sm) { resort_freight_info =true; sortierung = sm; }

	void force_resort() { resort_freight_info = true; }


	/**
	 * Calculates a status color for status bars
	 * @author Hj. Malthaner
	 */
	COLOR_VAL get_status_farbe() const { return status_color; }

	/**
	 * Draws some nice colored bars giving some status information
	 * @author Hj. Malthaner
	 */
	void display_status(KOORD_VAL xpos, KOORD_VAL ypos);

	/**
	 * sucht umliegende, erreichbare fabriken und baut daraus die
	 * Fabrikliste auf.
	 * "Surrounding searches, achievable factories and builds the
	 * factory list." (Google)
	 * @author Hj. Malthaner
	 */
	void verbinde_fabriken();
	void add_factory(fabrik_t* fab); 
	void remove_fabriken(fabrik_t *fab);

	void rotate90( const sint16 y_size );

	player_t *get_owner() const {return owner;}

	// just for info so far
	sint64 calc_maintenance() const;

	bool make_public_and_join( player_t *player );

	/**
	 * Checks if there is connection for certain freight to the other halt.
	 * @param halt the other halt
	 * @param catg_index freight category index
	 * @return 0 - not connected, 1 - connected, -1 - undecided (call again later...)
	 */
	// TODO: Check whetehr this can be removed entirely
	sint8 is_connected(halthandle_t halt, uint8 catg_index) const;

	const slist_tpl<fabrik_t*>& get_fab_list() const { return fab_list; }

	/**
	 * called regularly to update status and reroute stuff
	 * @author Hj. Malthaner
	 */

	void step();

	/**
	 * Called every month/every 24 game hours
	 * @author Hj. Malthaner
	 */
	void new_month();

	// @author: jamespetts, although much is borrowed from suche_route
	// Returns the journey time of the best possible route from this halt. Time == UINT32_MAX_VALUE when there is no route.
	uint32 find_route(ware_t &ware, const uint32 journey_time = UINT32_MAX_VALUE) const;
	void get_destination_halts_of_ware(ware_t &ware, vector_tpl<halthandle_t>& destination_halts_list) const;
	uint32 find_route(const vector_tpl<halthandle_t>& ziel_list, ware_t & ware, const uint32 journey_time = UINT32_MAX_VALUE, const koord destination_pos = koord::invalid) const;

	bool get_pax_enabled()  const { return enables & PAX;  }
	bool get_mail_enabled() const { return enables & POST; }
	bool get_ware_enabled() const { return enables & WARE; }

	// check, if we accepts this good
	// often called, thus inline ...
	bool is_enabled( const goods_desc_t *wtyp ) const {
		return is_enabled(wtyp->get_catg_index());
	}

	// a separate version for checking with goods category index
	bool is_enabled( const uint8 catg_index ) const
	{
		if (catg_index == goods_manager_t::INDEX_PAS) {
			return enables&PAX;
		}
		else if(catg_index == goods_manager_t::INDEX_MAIL) {
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
	 * Station in walking distance
	 * @author prissi
	 */
	void add_pax_walked(int n);

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

	int get_pax_happy()    const { return (int)financial_history[0][HALT_HAPPY]; }
	int get_pax_no_route() const { return (int)financial_history[0][HALT_NOROUTE]; }
	int get_pax_unhappy()  const { return (int)financial_history[0][HALT_UNHAPPY]; }
	int get_pax_too_slow()  const { return (int)financial_history[0][HALT_TOO_SLOW]; }

	/**
	 * Add tile to list of station tiles.
	 * @param relink_factories if true call verbinde_fabriken, if not true take care of factory connections yourself
	 */
	bool add_grund(grund_t *gb, bool relink_factories = true, bool recalc_nearby_halts = true);
	bool rem_grund(grund_t *gb);

	uint32 get_capacity(uint8 typ) const { return capacity[typ]; }

	bool existiert_in_welt() const;

	koord get_init_pos() const { return init_pos; }
	koord get_basis_pos() const;
	koord3d get_basis_pos3d() const;

	/* return the closest square that belongs to this halt
	 * @author prissi
	 */
	koord get_next_pos( koord start, bool square = false ) const;

	// true, if this station is overcrowded for this category
	bool is_overcrowded( const uint8 idx ) const { return (overcrowded[idx/8] & (1<<(idx%8)))!=0; }

	/**
	 * gibt Gesamtmenge derware vom typ typ zurück
	 * @author Hj. Malthaner
	 */
	uint32 get_ware_summe(const goods_desc_t *warentyp) const;

	/**
	 * returns total number for a certain position (since more than one factory might connect to a stop)
	 * @author Hj. Malthaner
	 */
	uint32 get_ware_fuer_zielpos(const goods_desc_t *warentyp, const koord zielpos) const;

	/**
	* True if we accept/deliver this kind of good
	*/
	bool gibt_ab(const goods_desc_t *warentyp) const { return cargo[warentyp->get_catg_index()] != NULL; }

	/* retrieves a ware packet for any destination in the list
	 * needed, if the factory in question wants to remove something
	 */
	bool recall_ware( ware_t& w, uint32 menge );

	/**
	 * Fetches goods from this halt. Returns true if other convois on the same line should try loading these goods because this convoi is awaiting spacing, if not preferred, or is overcrowded
	 * Fetches goods from this halt
	 * @param load Output parameter. Goods will be put into this list, the vehicle has to load them.
	 * @param good_category Specifies the kind of good (or compatible goods) we are requesting to fetch from this stop.
	 * @param amount How many units of the cargo we can fetch.
	 * @param schedule Schedule of the vehicle requesting the fetch.
	 * @param player Company that's requesting the fetch.
	 * @author Dwachs
	 */
	bool fetch_goods( slist_tpl<ware_t> &load, const goods_desc_t *good_category, uint32 requested_amount, const schedule_t *schedule, const player_t *player, convoi_t* cnv, bool overcrowd, const uint8 g_class, const bool use_lower_classes, bool& other_classes_available);

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
	void liefere_an(ware_t ware, uint8 walked_between_stations = 0);
	void starte_mit_route(ware_t ware, koord origin_pos);

	const grund_t *find_matching_position(waytype_t wt) const;

	/* checks, if there is an unoccupied loading bay for this kind of thing
	* @author prissi
	*/
	bool find_free_position(const waytype_t w ,convoihandle_t cnv,const obj_t::typ d) const;

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
	uint8 get_empty_lane(const grund_t *gr, convoihandle_t cnv) const;

	void info(cbuffer_t & buf, bool dummy = false) const;

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
	void get_short_freight_info(cbuffer_t & buf) const;

	/**
	 * Opens an information window for this station.
	 * @author Hj. Malthaner
	 */
	void show_info();

	/**
	 * @return the type of a station
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

	// create an unique name: better to be called with valid handle, although it will work without
	char* create_name(koord k, char const* typ);

	void rdwr(loadsave_t *file);

	void finish_rd(bool need_recheck_for_walking_distance);

	/**
	 * Called before savegame will be loaded.
	 * Creates all_koords table.
	 */
	static void start_load_game();

	/**
	 * Called after loading of savegame almost finished,
	 * i.e. after finish_rd is finished.
	 * Deletes all_koords table.
	 */
	static void end_load_game();


	/*
	 * called, if a line serves this stop
	 * @author hsiegeln
	 */
	void add_line(linehandle_t line);

	/*
	 * called, if a line removes this stop from it's schedule
	 * @author hsiegeln
	 */
	void remove_line(linehandle_t line);		

	/*
	 * list of line ids that serve this stop
	 * @author hsiegeln
	 */
	vector_tpl<linehandle_t> registered_lines;

	/**
	 * Register a lineless convoy which serves this stop
	 * @author Knightly
	 */
	void add_convoy(convoihandle_t convoy);

	/**
	 * Unregister a lineless convoy
	 * @author Knightly
	 */
	void remove_convoy(convoihandle_t convoy);

	/**
	 * A list of lineless convoys serving this stop
	 * @author Knightly
	 */
	vector_tpl<convoihandle_t> registered_convoys;

	// Update and recalculate the cached service intervals
	void update_service_intervals(schedule_t* sch);

#ifdef ALWAYS_CACHE_SERVICE_INTERVAL
	// This is a selective clear of the service intervals:
	// this will clear the service intervals to stops
	// on this schedule only. To clear all service intervals,
	// run service_intervals.clear(). 
	void clear_service_intervals(schedule_t* sch);
#endif

	/**
	 * It will calculate number of free seats in all other (not cnv) convoys at stop
	 * @author Inkelyad
	 */
	void update_alternative_seats(convoihandle_t cnv);

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
//	void desceid_station_voll() { enables |= CROWDED; status_color = COL_RED; }

	/* marks a coverage area
	* @author prissi
	*/
	void mark_unmark_coverage(const bool mark, const bool factories = false) const;

	// @author: jamespetts
	// Returns the percentage of unhappy people
	// out of the total of happy and unhappy people.
	uint16 get_unhappy_percentage(uint8 month) const
	{
		sint64 happy_count = financial_history[month][HALT_HAPPY];
		sint64 unhappy_count = financial_history[month][HALT_UNHAPPY];
		if (happy_count > 0) {
 			return (uint16) (unhappy_count * 100 / (happy_count + unhappy_count) );
		}
		else {
			return 0;
		}
	}

	// Getting and setting average waiting times in minutes
	// @author: jamespetts
	uint32 get_average_waiting_time(halthandle_t halt, uint8 category, uint8 g_class);

	void add_waiting_time(uint32 time, halthandle_t halt, uint8 category, uint8 g_class, bool do_not_reset_month = false);

	typedef quickstone_hashtable_tpl<haltestelle_t, connexion*>* connexions_map_single;
	connexions_map_single get_connexions(uint8 c) { return connexions[c]; }

	linehandle_t get_preferred_line(halthandle_t transfer, uint8 category) const;
	convoihandle_t get_preferred_convoy(halthandle_t transfer, uint8 category) const;

	// Added by		: Knightly
	// Adapted from : Jamespetts' code
	// Purpose		: To notify relevant halts to rebuild connexions and to notify all halts to recalculate paths
	// @jamespetts: modified the code to combine with previous method and provide options about partially delayed refreshes for performance.

	static void refresh_routing(const schedule_t *const sched, const minivec_tpl<uint8> &categories, const minivec_tpl<uint8> *passenger_classes, const minivec_tpl<uint8> *mail_classes, const player_t *const player);

	// Added by		: Knightly
	// Adapted from : haltestelle_t::add_connexion()
	// Purpose		: Create goods list of specified goods category if it is not already present
	void prepare_goods_list(uint8 category)
	{
		if (cargo[category] == NULL )
		{
			// indicates that this can route those goods
			cargo[category] = new vector_tpl<ware_t>(0);
		}
	}

	// Addedy by : Knightly
	// Purpose	 : Return the time at which the halt was first created
	sint64 get_inauguration_time() { return inauguration_time; }

	/*
	* deletes factory references so map rotation won't segfault
	*/
	void release_factory_links();

	/**
	 * Get queue position for spacing. It is *not* same as index in 'loading_here'.
	 * @author Inkelyad
	 */
	int get_queue_pos(convoihandle_t cnv) const;

	bool check_access(const player_t* player) const;

	bool has_no_control_tower() const;

	/**
	* Get the time that it takes for passengers to transfer within this stop
	* in 1/10ths of minutes.
	*/
	inline uint32 get_transfer_time() const { return transfer_time; }

	/**
	* Get the time that it takes for goods to be transferred within this stop
	* in 1/10ths of minutes.
	*/
	inline uint32 get_transshipment_time() const { return transshipment_time; }

	void set_reroute_goods_next_step(uint8 catg) { categories_to_refresh_next_step.append(catg); }

	/**
	* Calculate the transfer and transshipment time values.
	*/
	void calc_transfer_time();

	/**
	* The average time in 10ths of minutes between convoys to this destination
	*/
	uint32 get_service_frequency(halthandle_t destination, uint8 category) const;

	uint32 calc_service_frequency(halthandle_t destination, uint8 category) const;

	void set_estimated_arrival_time(uint16 convoy_id, sint64 time);
	void set_estimated_departure_time(uint16 convoy_id, sint64 time);

	/** 
	* Removes a convoy from the time estimates.
	* Used when deleting a convoy.
	*/
	void clear_estimated_timings(uint16 convoy_id);

	const arrival_times_map& get_estimated_convoy_arrival_times() { return estimated_convoy_arrival_times; }
	const arrival_times_map& get_estimated_convoy_departure_times() { return estimated_convoy_departure_times; }

	private: 

	sint32 translate_direction(ribi_t::ribi direction) const
	{
		sint32 dir;
		switch(direction)
		{
		default:
		case ribi_t::north:
			dir = 0;
			break;
		case ribi_t::south:
			dir = 1;
			break;
		case ribi_t::east:
			dir = 2;
			break;
		case ribi_t::west:
			dir = 3;
		};
		return dir;
	}

	public:

	sint64 get_train_last_departed(ribi_t::ribi direction) const { return train_last_departed[translate_direction(direction)]; }
	sint64 get_train_last_departed(uint32 dir) const { return train_last_departed[dir]; }
	void set_train_last_departed(sint64 time, ribi_t::ribi direction) { train_last_departed[translate_direction(direction)] = time; }

	void add_station_signal(koord3d pos) { station_signals.append(pos); }
	void remove_station_signal(koord3d pos) { station_signals.remove(pos); }
	uint32 get_station_signals_count() const { return station_signals.get_count(); }
	koord3d get_station_signal(uint32 value) const { return station_signals[value]; }
	bool is_station_signal_contained(koord3d pos) const { return station_signals.is_contained(pos); }
};

ENUM_BITSET(haltestelle_t::stationtyp)

#endif
