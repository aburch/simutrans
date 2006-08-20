/*
 * simplay.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simplay_h
#define simplay_h

#ifndef simtypes_h
#include "simtypes.h"
#endif

#ifndef halthandle_t_h
#include "halthandle_t.h"
#endif

#ifndef convoihandle_t_h
#include "convoihandle_t.h"
#endif

#ifndef koord_h
#include "dataobj/koord.h"
#endif


#define MAX_COST           10 // Total number of items in array

#define COST_CONSTRUCTION  0 // Construction
#define COST_VEHICLE_RUN   1 // Vehicle running costs
#define COST_NEW_VEHICLE   2 // New vehicles
#define COST_INCOME        3 // Income
#define COST_MAINTENANCE   4 // Upkeep
#define COST_ASSETS	   5 // value of all vehicles and buildings
#define COST_CASH          6 // Cash
#define COST_NETWEALTH     7 // Total Cash + Assets
#define COST_PROFIT	   8 // 3-(0+1+2+4)
#define COST_OPERATING_PROFIT 9 // 3-(1+4)
#define MAX_HISTORY_YEARS  12 // number of years to keep history
#define MAX_HISTORY_MONTHS  12 // number of months to keep history


// forward dekl

class karte_t;
class fabrik_t;
class wegbauer_t;
class slist_t;
class Stack;
class koord3d;

template <class T> class slist_tpl;

/**
 * Spieler in Simutrans. Diese Klasse enthält Routinen für die KI
 * als auch Informatuionen über den Spieler selbst zB die Kennfarbe
 * und den Kontostand.
 *
 * @author Hj. Malthaner
 */
class spieler_t
{
public:

    enum zustand {NEUE_ROUTE};

    enum subzustand {NR_INIT, NR_SAMMLE_ROUTEN,
                     NR_BAUE_ROUTE1, NR_BAUE_ROUTE2,
		     NR_BAUE_SCHIENEN_ROUTE1,
		     NR_BAUE_SCHIENEN_ROUTE2,
                     NR_BAUE_BAHNHOF,
		     NR_BAUE_SCHIENEN_VEHIKEL,
		     NR_BAUE_STRASSEN_ROUTE,
		     NR_BAUE_STRASSEN_ROUTE2,
                     NR_BAUE_STRASSEN_VEHIKEL
		    };

    enum { MAX_KONTO_VERZUG = 3 };

private:
    /*
     * holds total number of all halts, ever built
     * @author hsiegeln
     */
    int haltcount;

    /**
     * Finance array, indexed by type
     * @author Owen Rudge
     */
    sint64 finances[MAX_COST];
    sint64 old_finances[MAX_COST];


    /**
    * Finance History - will supercede the finances by Owen Rudge
    * Will hold finances for the most recent 12 years
    * the MAX_COST+1st element holds the sum of the other elements
    * @author hsiegeln
    */

    sint64 finance_history_year[MAX_HISTORY_YEARS][MAX_COST];
    sint64 finance_history_month[MAX_HISTORY_MONTHS][MAX_COST];

    /**
     * Monthly maintenance cost
     * @author Hj. Malthaner
     */
    uint32 maintenance;

    /**
     * Die Welt in der gespielt wird.
     *
     * @author Hj. Malthaner
     */
    karte_t *welt;


    /**
     * Der Kontostand.
     *
     * @author Hj. Malthaner
     */
    sint64 konto;


    /**
     * Zählt wie viele Monate das Konto schon ueberzogen ist
     *
     * @author Hj. Malthaner
     */
    int konto_ueberzogen;


    /**
     * Zählt die steps
     * @author Hj. Malthaner
     */
    int steps;


    /**
     * Liste der Haltestellen
     * @author Hj. Malthaner
     */
    slist_tpl<halthandle_t> * halt_list;


    char  texte[50][32];
    sint8 text_alter[50];
    koord text_pos[50];

    int last_message_index;

    // vars für die KI

    enum zustand state;
    enum subzustand substate;

    fabrik_t *start;
    fabrik_t *ziel;
    int gewinn;
    int count;

    // ende KI vars

    void do_ki();

    bool suche_platz(int x, int y, koord *);
    bool suche_platz(int x, int y, int dx, int dy,
		     koord off,
		     koord *);


    bool checke_bahnhof_bau(koord p);
    int baue_bahnhof(koord &p, int anz_vehikel);

    /**
     * @return gewinn für eine Fahrt per lkw auf dieser strecke
     * @author Hj. Malthaner
     */
    int suche_transport_quelle_ziel(fabrik_t **quelle, fabrik_t **ziel);

    void create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel);
    void create_rail_transport_vehikel(fabrik_t *qfab, int anz_vehikel);


    bool suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab);    // neue Transportroute anlegen

    bool create_simple_road_transport();    // neue Transportroute anlegen


    void create_bussing();      // neue Busroute anlegen

    void init_texte();

    void add_message(koord k, int summe);

    /**
     * Test if a bridge can be build here
     *
     * @param p position of obstacle
     * @param index index in route where obstacle was hit
     * @param ribi ribi of obstacle
     * @param bauigel the waybilder used to build the way
     * @param list list of bridge start/end koordinates
     * @author Hj. Malthaner
     */
    bool versuche_brueckenbau(koord p, int index, ribi_t::ribi ribi,
                              wegbauer_t &bauigel,
			      slist_tpl <koord> &list);

    bool checke_streckenbau(wegbauer_t &bauigel, slist_tpl<koord> &list);
    void baue_strecke(wegbauer_t &bauigel, slist_tpl<koord> &list);



public:

    /**
     * Age messages (move them upwards)
     * @author Hj. Malthaner
     */
    void age_messages(long delta_t);


    /**
     * Gets haltcount
     * @author hsiegeln
     */
    int get_haltcount() const;


    /**
     * Kennfarbe (Fahrzeuge, Gebäude) des Speielers
     * @author Hj. Malthaner
     */
    int kennfarbe;


    /**
     * Ist dieser Spieler ein automatischer Spieler?
     * @author Hj. Malthaner
     */
    bool automat;


    /**
     * Konstruktor
     * @param welt Die Welt (Karte) des Spiels
     * @param color Kennfarbe des Spielers
     * @author Hj. Malthaner
     */
    spieler_t(karte_t *welt, int color);


    /**
     * Destruktor
     * @author Hj. Malthaner
     */
    ~spieler_t();


    /**
     * Methode fuer jaehrliche Aktionen
     * @author Hj. Malthaner
     */
    void neues_jahr();


    /**
     * Bucht einen Betrag auf das Konto des Spielers
     * @param betrag zu verbuchender Betrag
     * @author Hj. Malthaner
     */
    sint64 buche(long betrag) {konto += betrag; return konto;};

    /**
     * Adds somme amount to the maintenance costs
     * @param change the change
     * @return the new maintenance costs
     * @author Hj. Malthaner
     */
    uint32 add_maintenance(sint32 change) {maintenance += change; return maintenance;};

    // Owen Rudge, finances
    sint64 buche(long betrag, koord k, int type);
    sint64 buche(long betrag, int type);


    /**
     * @return Kontostand als double (Gleitkomma) Wert
     * @author Hj. Malthaner
     */
    double gib_konto_als_double() const {return konto/100.0;};


    /**
     * @return true wenn Konto Überzogen ist
     * @author Hj. Malthaner
     */
    int gib_konto_ueberzogen() const {return konto_ueberzogen;};


    /**
     * Zeigt Meldungen aus der Queue des Spielers auf dem Bildschirm an
     * @author Hj. Malthaner
     */
    void display_messages(int xoff, int yoff, int width);


    /**
     * Wird von welt in kurzen abständen aufgerufen
     * @author Hj. Malthaner
     */
    void step();


    /**
     * Wird von welt nach jedem monat aufgerufen
     * @author Hj. Malthaner
     */

    void neuer_monat();


    /**
     * Erzeugt eine neue Haltestelle des Spielers an Position pos
     * @author Hj. Malthaner
     */
    halthandle_t halt_add(koord pos);


    /**
     * Entfernt eine Haltestelle des Spielers aus der Liste
     * @author Hj. Malthaner
     */
    void halt_remove(halthandle_t halt);


    /**
     * Ermittelt, ob an Position i,j eine Haltestelle des Spielers
     * vorhanden ist.
     *
     * @return die gefundene Haltestelle oder NULL wenn keine Haltestelle
     * an Position i,j
     * @author Hj. Malthaner
     */
    halthandle_t ist_halt(koord k) const;

    /**
     * Lädt oder speichert Zustand des Spielers
     * @param file die offene Save-Datei
     * @author Hj. Malthaner
     */
    void rdwr(loadsave_t *file);


    // fuer tests
    koord platz1, platz2;
    bool create_complex_rail_transport();
    bool create_complex_road_transport();    // neue Transportroute anlegen


    /**
     * Returns the amount of money for a certain finance section
     * @author Owen Rudge
     */
    sint64 get_finance_info(int type);


    /**
     * Returns the amount of money for a certain finance section from previous year
     *
     * @author Owen Rudge
     */
    sint64 get_finance_info_old(int type);

    /**
    * Returns the finance history for player
    * @author hsiegeln
    */
    sint64 get_finance_history_year(int year, int type) {return finance_history_year[year][type];};

    /**
    * Returns pointer to finance history for player
    * @author hsiegeln
    */
    sint64* get_finance_history_year() {return *finance_history_year;};
	sint64* get_finance_history_month() {return *finance_history_month;};

    /**
    * Returns the world the player is in
    * @author hsiegeln
    */
    karte_t *gib_welt() { return welt; }

    /**
    * Calculates the finance history for player
    * @author hsiegeln
    */
    void calc_finance_history();

    /**
    * rolls the finance history for player (needed when neues_jahr() or neuer_monat()) triggered
    * @author hsiegeln
    */
    void spieler_t::roll_finance_history_year();
    void spieler_t::roll_finance_history_month();

    /**
     * Rückruf, um uns zu informieren, dass eine Station voll ist
     * @author Hansjörg Malthaner
     * @date 25-Nov-2001
     */
    void bescheid_station_voll(halthandle_t halt);


    /**
     * Rückruf, um uns zu informieren, dass ein Vehikel ein Problem hat
     * @author Hansjörg Malthaner
     * @date 26-Nov-2001
     */
    void bescheid_vehikel_problem(convoihandle_t cnv);

};

#endif
