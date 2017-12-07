/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simhalt_h
#define simhalt_h

#include "convoihandle_t.h"
#include "linehandle_t.h"
#include "halthandle_t.h"

#include "simobj.h"
#include "display/simgraph.h"
#include "simtypes.h"

#include "bauer/goods_manager.h"

#include "descriptor/goods_desc.h"

#include "dataobj/koord.h"

#include "tpl/inthashtable_tpl.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/binary_heap_tpl.h"


#define RECONNECTING (1)
#define REROUTING (2)

#define MAX_HALT_COST   8 // Total number of cost items
#define MAX_MONTHS     12 // Max history
#define MAX_HALT_NON_MONEY_TYPES 7 // number of non money types in HALT's financial statistic
#define HALT_ARRIVED   0 // the amount of ware that arrived here
#define HALT_DEPARTED 1 // the amount of ware that has departed from here
#define HALT_WAITING		2 // the amount of ware waiting
#define HALT_HAPPY		3 // number of happy passengers
#define HALT_UNHAPPY		4 // number of unhappy passengers
#define HALT_NOROUTE         5 // number of no-route passengers
#define HALT_CONVOIS_ARRIVED             6 // number of convois arrived this month
#define HALT_WALKED 7 // could walk to destination

class cbuffer_t;
class grund_t;
class fabrik_t;
class world_t;
class karte_ptr_t;
class koord3d;
class loadsave_t;
class schedule_t;
class player_t;
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
	 * Manche Methoden m�ssen auf alle Haltestellen angewandt werden
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

	PIXVAL status_color, last_status_color;
	sint16 last_bar_count;
	vector_tpl<KOORD_VAL> last_bar_height; // caches the last height of the station bar for each good type drawn in display_status(). used for dirty tile management
	uint32 capacity[3]; // passenger, mail, goods
	uint8 overcrowded[256/8]; ///< bit field for each goods type (max 256)

	static uint8 status_step;	// NONE or SCHEDULING or REROUTING

	slist_tpl<convoihandle_t> loading_here;
	sint32 last_loading_step;

	koord init_pos;	// for halt without grounds, created during game initialisation

	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * @author Hj. Malthaner
	 */
	halthandle_t self;

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

	static uint8 get_rerouting_status() { return status_step; }

	/**
	 * Resets reconnect_counter.
	 * The next call to step_all() will start complete reconnecting.
	 */
	static void reset_routing();

	/**
	 * Tries to generate some pedestrians on the square and the
	 * adjacent squares. Return actual number of generated
	 * pedestrians.
	 *
	 * @author Hj. Malthaner
	 */
	static int generate_pedestrians(const koord3d pos, int count);

	/**
	 * Returns an index to a halt at koord k
   	 * optionally limit to that owned by player sp
   	 * by default create a new halt if none found
	 * Only used during loading.
	 */
	static halthandle_t get_halt_koord_index(koord k);

	/*
	 * this will only return something if this stop belongs to same player or is public, or is a dock (when on water)
	 */
	static halthandle_t get_halt(const koord3d pos, const player_t *player );

	static const vector_tpl<halthandle_t>& get_alle_haltestellen() { return alle_haltestellen; }

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

	/**
	 * Liste aller felder (Grund-Objekte) die zu dieser Haltestelle geh�ren
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

	/**
	 * directly reachable halt with its connection weight
	 * @author Knightly
	 */
	struct connection_t
	{
		/// directly reachable halt
		halthandle_t halt;
		/// best connection weight to reach this destination
		uint16 weight:15;
		/// is halt a transfer halt
		bool is_transfer:1;

		connection_t() : weight(0), is_transfer(false) { }
		connection_t(halthandle_t _halt, uint16 _weight=0) : halt(_halt), weight(_weight), is_transfer(false) { }

		bool operator == (const connection_t &other) const { return halt == other.halt; }
		bool operator != (const connection_t &other) const { return halt != other.halt; }
		static bool compare(const connection_t &a, const connection_t &b) { return a.halt.get_id() < b.halt.get_id(); }
	};

	bool is_transfer(const uint8 catg) const { return all_links[catg].is_transfer; }

private:
	slist_tpl<tile_t> tiles;


	/**
	 * Stores information about link to cargo network of a certain category
	 */
	struct link_t {
		/// List of all directly reachable halts with their respective connection weights
		vector_tpl<connection_t> connections;

		/**
		 * A transfer/interchange is a halt whereby ware can change line or lineless convoy.
		 * Thus, if a halt is served by 2 or more schedules (of lines or lineless convoys)
		 * for a particular ware type, it is a transfer/interchange for that ware type.
		 * Route searching is accelerated by differentiating transfer and non-transfer halts.
		 * @author Knightly
		 */
		bool is_transfer;

		/**
		 * Id of connected component in link graph.
		 * Two halts are connected if and only if they belong to the same connected component.
		 * Exception: if value == UNDECIDED_CONNECTED_COMPONENT, then we are in the middle of
		 * recalculating the link graph.
		 *
		 * The id of the component has to be equal to the halt-id of one of its halts.
		 * This ensures that we always have unique component ids.
		 */
		uint16 catg_connected_component;

#		define UNDECIDED_CONNECTED_COMPONENT (0xffff)

		link_t() { clear(); }

		void clear()
		{
			connections.clear();
			is_transfer = false;
			catg_connected_component = UNDECIDED_CONNECTED_COMPONENT;
		}
	};

	/// All links to networks of all freight categories, filled by rebuild_connected_components.
	link_t* all_links;

	/**
	 * Fills in catg_connected_component values for all halts and all categories.
	 * Uses depth-first search.
	 */
	static void rebuild_connected_components();

	/**
	 * Helper method: This halt (and all its connected neighbors) belong
	 * to the same component.
	 * Also sets connection_t::is_transfer.
	 * @param catg category of cargo network
	 * @param comp number of component
	 */
	void fill_connected_component(uint8 catg, uint16 comp);


	// Array with different categories that contains all waiting goods at this stop
	vector_tpl<ware_t> **cargo;

	/**
	 * Liste der angeschlossenen Fabriken
	 * @author Hj. Malthaner
	 */
	slist_tpl<fabrik_t *> fab_list;

	player_t *owner_p;
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
	uint8 last_catg_index;

	/* station flags (most what enabled) */
	uint8 enables;

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
	void liefere_an_fabrik(const ware_t& ware) const;

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

	haltestelle_t(loadsave_t *file);
	haltestelle_t(koord pos, player_t *player);
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
	PIXVAL get_status_farbe() const { return status_color; }

	/**
	 * Draws some nice colored bars giving some status information
	 * @author Hj. Malthaner
	 */
	void display_status(KOORD_VAL xpos, KOORD_VAL ypos);

	/**
	 * sucht umliegende, erreichbare fabriken und baut daraus die
	 * Fabrikliste auf.
	 * @author Hj. Malthaner
	 */
	void verbinde_fabriken();
	void remove_fabriken(fabrik_t *fab);

	/**
	 * Rebuilds the list of connections to reachable halts
	 * returns the search number of connections
	 * @author Hj. Malthaner
	 */
	sint32 rebuild_connections();

	/**
	 * Rebuilds connections of all halts connected to this halt.
	 * Prepares deletion of this halt without losing connections and routed freight.
	 */
	void rebuild_linked_connections();

	uint8 get_reconnect_counter() const  { return reconnect_counter; }

	void rotate90( const sint16 y_size );

	player_t *get_owner() const {return owner_p;}

	// just for info so far
	sint64 calc_maintenance() const;

	void make_public_and_join( player_t *player );

	vector_tpl<connection_t> const& get_pax_connections()  const { return all_links[goods_manager_t::INDEX_PAS].connections;  }
	vector_tpl<connection_t> const& get_mail_connections() const { return all_links[goods_manager_t::INDEX_MAIL].connections; }

	// returns the matching warenziele (goods objectives/destinations)
	vector_tpl<connection_t> const& get_connections(uint8 const catg_index) const { return all_links[catg_index].connections; }

	/**
	 * Checks if there is connection for certain freight to the other halt.
	 * @param halt the other halt
	 * @param catg_index freight category index
	 * @return 0 - not connected, 1 - connected, -1 - undecided (call again later...)
	 */
	sint8 is_connected(halthandle_t halt, uint8 catg_index) const;

	const slist_tpl<fabrik_t*>& get_fab_list() const { return fab_list; }

	/**
	 * called regularly to update status and reroute stuff
	 * @author Hj. Malthaner
	 */
	bool step(uint8 what, sint16 &units_remaining);

	/**
	 * Called every month/every 24 game hours
	 * @author Hj. Malthaner
	 */
	void new_month();

private:
	/* Node used during route search */
	struct route_node_t
	{
		halthandle_t halt;
		uint16       aggregate_weight;

		route_node_t() : aggregate_weight(0) {}
		route_node_t(halthandle_t h, uint16 w) : halt(h), aggregate_weight(w) {}

		// dereferencing to be used in binary_heap_tpl
		inline uint16 operator * () const { return aggregate_weight; }
	};

	/* Extra data for route search */
	struct halt_data_t
	{
		// transfer halt:
		// in static function search_route():  previous transfer halt (to track back route)
		// in member function search_route_resumable(): first transfer halt to get there
		halthandle_t transfer;
		uint16 best_weight;
		uint16 depth:14;
		bool destination:1;
		bool overcrowded:1;
	};

	// store the best weight so far for a halt, and indicate whether it is a destination
	static halt_data_t halt_data[65536];

	// for efficient retrieval of the node with the smallest weight
	static binary_heap_tpl<route_node_t> open_list;

	/**
	 * Markers used in route searching to avoid processing the same halt more than once
	 * @author Knightly
	 */
	static uint8 markers[65536];
	static uint8 current_marker;

	/**
	 * Remember last route search start and catg to resume search
	 * @author dwachs
	 */
	static halthandle_t last_search_origin;
	static uint8        last_search_ware_catg_idx;
public:
	enum routing_result_flags { NO_ROUTE=0, ROUTE_OK=1, ROUTE_WALK=2, ROUTE_OVERCROWDED=8 };

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
	static int search_route( const halthandle_t *const start_halts, const uint16 start_halt_count, const bool no_routing_over_overcrowding, ware_t &ware, ware_t *const return_ware=NULL );

	/**
	 * A separate version of route searching code for re-calculating routes
	 * Search is resumable, that is if called for the same halt and same goods category
	 * it reuses search history from last search
	 * It is faster than calling the above version on each packet, and is used for re-routing packets from the same halt.
	 */
	void search_route_resumable( ware_t &ware );

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

	int get_pax_happy()    const { return (int)financial_history[0][HALT_HAPPY]; }
	int get_pax_no_route() const { return (int)financial_history[0][HALT_NOROUTE]; }
	int get_pax_unhappy()  const { return (int)financial_history[0][HALT_UNHAPPY]; }


	/**
	 * Add tile to list of station tiles.
	 * @param relink_factories if true call verbinde_fabriken, if not true take care of factory connections yourself
	 */
	bool add_grund(grund_t *gb, bool relink_factories = true);
	bool rem_grund(grund_t *gb);

	uint32 get_capacity(uint8 typ) const { return capacity[typ]; }

	bool existiert_in_welt() const;

	koord get_init_pos() const { return init_pos; }
	koord get_basis_pos() const;
	koord3d get_basis_pos3d() const;

	/* return the closest square that belongs to this halt
	 * @author prissi
	 */
	koord get_next_pos( koord start ) const;

	// true, if this station is overcrowded for this ware
	bool is_overcrowded( const uint8 idx ) const { return (overcrowded[idx/8] & (1<<(idx%8)))!=0; }

	/**
	 * gibt Gesamtmenge derware vom typ typ zur�ck
	 * @author Hj. Malthaner
	 */
	uint32 get_ware_summe(const goods_desc_t *warentyp) const;

	/**
	 * returns total number for a certain position (since more than one factory might connect to a stop)
	 * @author Hj. Malthaner
	 */
	uint32 get_ware_fuer_zielpos(const goods_desc_t *warentyp, const koord zielpos) const;

	/**
	 * total amount of freight with specified next hop
	 * @author prissi
	 */
	uint32 get_ware_fuer_zwischenziel(const goods_desc_t *warentyp, const halthandle_t zwischenziel) const;

	// true, if we accept/deliver this kind of good
	bool gibt_ab(const goods_desc_t *warentyp) const { return cargo[warentyp->get_catg_index()] != NULL; }

	/* retrieves a ware packet for any destination in the list
	 * needed, if the factory in question wants to remove something
	 */
	bool recall_ware( ware_t& w, uint32 menge );

	/**
	 * Fetches goods from this halt
	 * @param load Output parameter. Goods will be put into this list, the vehicle has to load them.
	 * @param good_category Specifies the kind of good (or compatible goods) we are requesting to fetch from this stop.
	 * @param amount How many units of the cargo we can fetch.
	 * @param schedule Schedule of the vehicle requesting the fetch.
	 * @param sp Company that's requesting the fetch.
	 * @author Dwachs
	 */
	void fetch_goods( slist_tpl<ware_t> &load, const goods_desc_t *good_category, uint32 requested_amount, const vector_tpl<halthandle_t>& destination_halts);

	/* liefert ware an. Falls die Ware zu wartender Ware dazugenommen
	 * werden kann, kann ware_t gel�scht werden! D.h. man darf ware nach
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
	void get_short_freight_info(cbuffer_t & buf) const;

	/**
	 * Opens an information window for this station.
	 * @author Hj. Malthaner
	 */
	void open_info_window();

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

	void finish_rd();

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
	 * Register a lineless convoy which serves this stop
	 * @author Knightly
	 */
	void add_convoy(convoihandle_t convoy) { registered_convoys.append_unique(convoy); }

	/**
	 * Unregister a lineless convoy
	 * @author Knightly
	 */
	void remove_convoy(convoihandle_t convoy) { registered_convoys.remove(convoy); }

	/**
	 * A list of lineless convoys serving this stop
	 * @author Knightly
	 */
	vector_tpl<convoihandle_t> registered_convoys;

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
//	void bescheid_station_voll() { enables |= CROWDED; status_color = color_idx_to_rgb(COL_RED); }  // for now report only serious overcrowding on transfer stops

	/* marks a coverage area
	* @author prissi
	*/
	void mark_unmark_coverage(const bool mark) const;

	/*
	* deletes factory references so map rotation won't segfault
	*/
	void release_factory_links();

	/**
	 * Initialise the markers to zero
	 * @author Knightly
	 */
	static void init_markers();

};

ENUM_BITSET(haltestelle_t::stationtyp)

#endif
