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

#ifndef simdebug_h
#include "simdebug.h"
#endif

#ifndef tpl_slist_tpl_h
#include "tpl/slist_tpl.h"
#endif

#ifndef ptrhashtable_tpl_h
#include "tpl/ptrhashtable_tpl.h"
#endif


#ifndef simware_h
#include "simware.h"
#endif

#ifndef dataobj_warenziel_h
#include "dataobj/warenziel.h"
#endif

#ifndef koord3d_h
#include "dataobj/koord3d.h"
#endif

#ifndef simtypes_h
#include "simtypes.h"
#endif


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

#include "halthandle_t.h"

/*
 * struct hold travel details for wares that travel
 * @author hsiegeln
 */
struct travel_details {
	ware_t ware;
	halthandle_t destination;
	halthandle_t via_destination;
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


  /**
   * Sets max number of hops in route calculation
   * @author Hj. Malthaner
   */
  static void set_max_hops(int hops);


private:


    /**
     * Manche Methoden müssen auf alle Haltestellen angewandt werden
     * deshalb verwaltet die Klasse eine Liste aller Haltestellen
     * @author Hj. Malthaner
     */
    static slist_tpl<halthandle_t> alle_haltestellen;

    static int erzeuge_fussgaenger_an(karte_t *welt, koord3d k, int anzahl);

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
    enum stationtyp {invalid=0, loadingbay = 1 ,
                     railstation = 2, dock = 4, busstop = 8}; //could be combined with or!

    // @author hsiegeln added
    enum sort_mode_t { by_name=0, by_via=1, by_via_sum=2, by_amount=3};


    /* sucht Haltestelle an Koordinate pos.
     *
     * @param gr die Position an der gesucht werden soll
     * @return NULL, wenn nichts gefunden, sonst Zeiger auf Haltestelle
     * @author prissi
     */
    static halthandle_t gib_halt(const karte_t *welt, grund_t *gr);

    static halthandle_t gib_halt(const karte_t *welt, const koord pos);
    static halthandle_t gib_halt(const karte_t *welt, const koord * const pos);

    // Hajo: for future compatibility, migrate to this call!
    static halthandle_t gib_halt(const karte_t *welt, const koord3d pos);



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
     * The name is saved with the first ground square. After creating and/
     * or loading a station the name must be copied into the name array.
     * This flag tells wether the name s has been copied yet or not.
     * Set to true in all constructors. Setto false in gib_name() after
     * copying the name.
     * @author Hj. Malthaner
     */
    bool need_name;


    /**
     * Liste aller felder (Grund-Objekte) die zu dieser Haltestelle gehören
     * @author Hj. Malthaner
     */
    slist_tpl<grund_t *> grund;


    /**
     * What is that for a station (for the image)
     * @author prissi
     */
	stationtyp station_type;


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

    bool pax_enabled;
    bool post_enabled;
    bool ware_enabled;


    /**
     * Found route and station uncrowded
     * @author Hj. Malthaner
     */
    int pax_happy;


    /**
     * Found no route
     * @author Hj. Malthaner
     */
    int pax_no_route;


    /**
     * Station crowded
     * @author Hj. Malthaner
     */
    int pax_unhappy;




    /**
     * Haltestellen werden beim warenrouting markiert. Jeder durchgang
     * hat eine eindeutige marke
     * @author Hj. Malthaner
     */
    unsigned int marke;

    const char * quote_bezeichnung(int quote) const;


    halt_info_t *halt_info;


    /**
     * Initialisiert das gui für diese Haltestelle.
     * Muss aus jedem Konstruktor aufgerufen werden.
     * @author Hj. Malthaner
     */
    void init_gui();


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
    * sortby is static and set during rendering of halt_info frame
    * @author hsiegeln
    */
    static sort_mode_t sortby;

    /*
    * parameter to ease sorting
    * sortierung is local and stores the sortorder for the individual station
    * @author hsiegeln
    */
    sort_mode_t sortierung;

public:

    /**
     * getter/setter for sortby
     * @author hsiegeln
     */
    haltestelle_t::sort_mode_t get_sortby() { return sortierung; }
    void set_sortby(sort_mode_t sm) { sortby = sm; sortierung = sm; }


    /**
     * Calculates a status color for status bars
     * @author Hj. Malthaner
     */
    int gib_status_farbe() const;


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


    spieler_t *gib_besitzer() const {return besitzer_p;};

    const slist_tpl<warenziel_t> * gib_warenziele() const {return &warenziele;};

	const slist_tpl<ware_t> * gib_warenliste(const ware_besch_t *ware) { return waren.get(ware); };

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


    /**
     * liefert zu einer Zielkoordinate die Haltestelle
     * an der Zielkoordinate
     * @author Hj. Malthaner
     * @see haltestelle_t::gib_halt
     */
    halthandle_t gib_halt(const koord ziel) const;


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

    void set_pax_enabled(bool yesno) {pax_enabled = yesno;};
    void set_post_enabled(bool yesno){post_enabled = yesno;};
    void set_ware_enabled(bool yesno){ware_enabled = yesno;};

    bool get_pax_enabled() const { return pax_enabled;};
    bool get_post_enabled() const { return post_enabled;};



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
    void rem_grund(grund_t *gb,bool final=false);

    int gib_grund_count() const {return grund.count();};

    bool existiert_in_welt();

    bool ist_da(koord pos) const;
    koord gib_basis_pos() const;

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


    bool nimmt_an(const ware_besch_t *warentyp);
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
    void add_line(simline_t * line) { if (!registered_lines.contains(line)) registered_lines.append(line); rebuild_destinations(); };

    /*
     * called, if a line removes this stop from it's schedule
     * @author hsiegeln
     */
    void remove_line(simline_t * line) { registered_lines.remove(line); rebuild_destinations(); };

    /*
     * list of line ids that serve this stop
     * @author hsiegeln
     */
    slist_tpl<simline_t *> registered_lines;

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

};


#endif
