/*
 * simhalt.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simhalt_h
#define simhalt_h

#include "convoihandle_t.h"
#include "linehandle_t.h"

#include "simdebug.h"
#include "simware.h"
#include "simtypes.h"
#include "simdings.h"

#include "boden/wege/weg.h"

#include "bauer/warenbauer.h"

#include "dataobj/warenziel.h"
#include "dataobj/koord3d.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"
#include "tpl/ptrhashtable_tpl.h"



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

class spieler_t;
class fabrik_t;
class karte_t;
#ifdef LAGER_NOT_IN_USE
class lagerhaus_t;
#endif
class grund_t;
class fahrplan_t;
class koord;
class halt_info_t;
struct event_t;
class simline_t;
class cbuffer_t;
class freight_list_sorter_t;

#include "halthandle_t.h"

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
  /**
   * Sets max number of hops in route calculation
   * @author Hj. Malthaner
   */
  static void set_max_hops(int hops);

	enum station_flags { NOT_ENABLED=0, PAX=1, POST=2, WARE=4, CROWDED=8 };

private:
    /**
     * Manche Methoden müssen auf alle Haltestellen angewandt werden
     * deshalb verwaltet die Klasse eine Liste aller Haltestellen
     * @author Hj. Malthaner
     */
    static slist_tpl<halthandle_t> alle_haltestellen;

    /*
    * declare compare function
    * @author hsiegeln
    */
    static int compare_ware(const void *td1, const void *td2);

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
	uint16 capacity;
	void recalc_status();

public:


    /**
     * Tries to generate some pedestrians on the sqaure and the
     * adjacent sqaures. Return actual number of generated
     * pedestrians.
     *
     * @author Hj. Malthaner
     */
    static int erzeuge_fussgaenger(karte_t *welt, koord3d pos, int anzahl);

    //13-Jan-02     Markus Weber    Added
    enum stationtyp {invalid=0, loadingbay = 1 , railstation = 2, dock = 4, busstop = 8, airstop = 16, monorailstop = 32 }; //could be combined with or!

    /* searches for a stop at the given koordinate
     * this is called damend often, so we should thin about inline it
     * @return hanthandle_t(), if nothing found
     * @author prissi
     */
	static halthandle_t gib_halt(karte_t *welt, const koord pos);

	// Hajo: for future compatibility, migrate to this call
	// but since we allow only for a single stop per planquadrat, this is as good as the above
	static halthandle_t gib_halt(karte_t *welt, const koord3d pos) { return gib_halt(welt,pos.gib_2d()); }

    /**
     * Prueft, ob halt auf eine Haltestelle zeigt
     * @param halt der zu prüfende Zeiger
     * @author Hj. Malthaner
     */
    static bool pruefe_zeiger(halthandle_t halt) {return alle_haltestellen.contains( halt );};

    static const slist_tpl<halthandle_t> & gib_alle_haltestellen() {return alle_haltestellen;};

    /**
     * @return die Anzahl der Haltestellen
     * @author Hj. Malthaner
     */
    static int gib_anzahl() {return alle_haltestellen.count();};


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
    static void destroy_all();

private:

    /**
     * Handle for ourselves. Can be used like the 'this' pointer
     * @author Hj. Malthaner
     */
    halthandle_t self;

    char name[128];
    const char * name_from_ground() const;

    /**
     * Liste aller felder (Grund-Objekte) die zu dieser Haltestelle gehören
     * @author Hj. Malthaner
     */
    slist_tpl<grund_t *> grund;
    vector_tpl<convoihandle_t> reservation;

    // fuer die zielverwaltung
    slist_tpl<warenziel_t> warenziele;

    // loest warte_menge ab
    ptrhashtable_tpl<const ware_besch_t *, slist_tpl<ware_t> *> waren;

    /**
     * Liste der angeschlossenen Fabriken
     * @author Hj. Malthaner
     */
    slist_tpl<fabrik_t *> fab_list;

    spieler_t   *besitzer_p;
    karte_t     *welt;

#ifdef LAGER_NOT_IN_USE
    lagerhaus_t *lager;         // unser lager, falls vorhanden
#endif
    koord pos;

    /**
     * What is that for a station (for the image)
     * @author prissi
     */
	stationtyp station_type;

	uint8 rebuilt_destination_counter;	// new schedule, first rebuilt destinations asynchroniously
	uint8 reroute_counter;						// the reroute goods

	/* station flags (most what enabled) */
	uint8 enables;

    void set_pax_enabled(bool yesno) {yesno ? enables|=PAX : enables&=(~PAX);};
    void set_post_enabled(bool yesno) {yesno ? enables|=POST : enables&=(~POST);};
    void set_ware_enabled(bool yesno) {yesno ? enables|=WARE : enables&=(~WARE);};

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

    halt_info_t *halt_info;

    /**
     * versucht die ware mit beriets wartender ware zusammenzufassen
     * @author Hj. Malthaner
     */
    bool vereinige_waren(const ware_t &ware);

    /**
     * liefert wartende ware an eine Fabrik
     * @author Hj. Malthaner
     */
    void liefere_an_fabrik(ware_t ware);


    haltestelle_t(karte_t *welt, loadsave_t *file);
    haltestelle_t(karte_t *welt, koord pos, spieler_t *sp);

    ~haltestelle_t();

    /*
    * parameter to ease sorting
    * sortierung is local and stores the sortorder for the individual station
    * @author hsiegeln
    */
    uint8 sortierung;
	bool resort_freight_info;

	/**
	 * Called every 255 steps
	 * will distribute the goods to changed routes (if there are any)
	 * @author Hj. Malthaner
	 */
	void reroute_goods();

public:

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
    int gib_status_farbe() const {return status_color; };


    /**
     * Draws some nice colored bars giving some status information
     * @author Hj. Malthaner
     */
    void display_status(int xpos, int ypos) const;


    /**
     * sucht umliegende, erreichbare fabriken und baut daraus die
     * Fabrikliste auf.
     * @author Hj. Malthaner
     */
    void verbinde_fabriken();
    void remove_fabriken(fabrik_t *fab);


    /**
     * Rebuilds the list of reachable destinations
     *
     * @author Hj. Malthaner
     */
    void rebuild_destinations();


    spieler_t *gib_besitzer() const {return besitzer_p;}
    void transfer_to_public_owner();

    const slist_tpl<warenziel_t> * gib_warenziele() const {return &warenziele;}

	const slist_tpl<ware_t> * gib_warenliste(const ware_besch_t *ware) { return waren.get(ware); }

    const slist_tpl<fabrik_t*> & gib_fab_list() const {return fab_list;};


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


    karte_t *gib_welt() const {return welt;};


    /**
     * Kann die Ware nicht zum Ziel geroutet werden (keine Route), dann werden
     * Ziel und Zwischenziel auf koord::invalid gesetzt.
     *
     * @param ware die zu routende Ware
     * @author Hj. Malthaner
     *
     * for reverse routing, also the next to last stop can be added, if next_to_ziel!=NULL
     * @author prissi
     */
    void suche_route(ware_t &ware, koord *next_to_ziel=NULL);

	/* true, if there is a conncetion between these places
	 * @author prissi
	 */
	bool is_connected(const halthandle_t halt, const ware_besch_t * wtyp);

    int get_pax_enabled() const { return enables&PAX;};
    int get_post_enabled() const { return enables&POST;};
    int get_ware_enabled() const { return enables&WARE;};

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

    int get_pax_happy() const {return pax_happy;};
    int get_pax_no_route() const {return pax_no_route;};
    int get_pax_unhappy() const {return pax_unhappy;};


#ifdef LAGER_NOT_IN_USE
    void setze_lager(lagerhaus_t *l) {lager = l;};
#endif
    bool add_grund(grund_t *gb);
    void rem_grund(grund_t *gb);

    int get_capacity() const {return capacity*32;}

    bool existiert_in_welt();

    koord gib_basis_pos() const;
    koord3d gib_basis_pos3d() const;

    /* return the closest square that belongs to this halt
     * @author prissi
     */
     koord get_next_pos( koord start ) const;

    /**
     * gibt Gesamtmenge derware vom typ typ zurück
     * @author Hj. Malthaner
     */
    int gib_ware_summe(const ware_besch_t *warentyp) const;


    /**
     * gibt Gesamtmenge derware vom typ typ fuer ziel zurück
     * @author Hj. Malthaner
     */
    int gib_ware_fuer_ziel(const ware_besch_t *warentyp,
			   const koord ziel) const;

    /**
     * gibt Gesamtmenge derware vom typ typ fuer zwischenziel zurück
     * @author prissi
     */
    int gib_ware_fuer_zwischenziel(const ware_besch_t *warentyp, const koord zwischenziel) const;

    bool gibt_ab(const ware_besch_t *warentyp) const;

    /**
     * holt ware ab
     * @return abgeholte menge
     * @author Hj. Malthaner
     */
    ware_t hole_ab(const ware_besch_t *warentyp, int menge, fahrplan_t *fpl);


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
    int liefere_an(ware_t ware);
    int starte_mit_route(ware_t ware);

    /**
     * wird von Fahrzeug aufgerufen, wenn dieses an der Haltestelle
     * gehalten hat.
     * @param typ der beförderte warentyp
     * @author Hj. Malthaner
     */
    void hat_gehalten(const int number_of_cars,
                      const ware_besch_t *warentyp,
                      const fahrplan_t *fpl);


	/* checks, if there is an unoccupied loading bay for this kind of thing
	 * @author prissi
	 */
	bool find_free_position(const weg_t::typ w ,convoihandle_t cnv,const ding_t::typ d) const;

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
	bool is_reservable(grund_t *gr, convoihandle_t cnv);


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
     * Opens the details window for this station.
     * @author Hj. Malthaner
     */
    void open_detail_window();


    /**
     * @returns station number
     * @author Markus Weber
     */
    int index_of() const {return alle_haltestellen.index_of(self);}   //13-Feb-02     Markus Added


    /**
     * @returns the sum of all waiting goods (100t coal + 10
     * passengers + 2000 liter oil = 2110)
     * @author Markus Weber
     */
    int sum_all_waiting_goods() const;


    /**
     * @returns true, if goods or passengers are waiting
     * @author Markus Weber
     */
    bool is_something_waiting() const;       //12-Jan-2002    Markus Weber    Added


    /**
     * @returns the type of a station
     * (combination of: railstation, loading bay, dock)
     * @author Markus Weber
     */
    stationtyp get_station_type() const {return station_type; };
    void recalc_station_type();


    /**
     * fragt den namen der Haltestelle ab.
     * Der Name ist der text des ersten Untergrundes der Haltestelle
     * @return der Name der Haltestelle.
     * @author Hj. Malthaner
     */
    const char * gib_name() const;

    char * access_name();

    void setze_name(const char *name);

    void rdwr(loadsave_t *file);

    void laden_abschliessen();

    /*
     * called, if a line serves this stop
     * @author hsiegeln
     */
    void add_line(linehandle_t line) { registered_lines.append_unique(line,4); }

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
    sint64* get_finance_history() { return *financial_history; };

    /**
     * return a specified element from the financial history
     * @author hsiegeln
     */
    sint64 get_finance_history(int month, int cost_type) { return financial_history[month][cost_type]; };

	// flags station for a crowded message at the beginning of next month
	void bescheid_station_voll() { enables|=CROWDED; };
};

#endif
