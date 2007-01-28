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

#include "simtypes.h"
#include "simlinemgmt.h"

#include "halthandle_t.h"
#include "convoihandle_t.h"

#include "dataobj/koord.h"

#include "boden/wege/weg.h"

#include "besch/weg_besch.h"
#include "besch/vehikel_besch.h"
#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"


#define MAX_COST           12 // Total number of items in array

#define COST_CONSTRUCTION  0 // Construction
#define COST_VEHICLE_RUN   1 // Vehicle running costs
#define COST_NEW_VEHICLE   2 // New vehicles
#define COST_INCOME        3 // Income
#define COST_MAINTENANCE   4 // Upkeep
#define COST_ASSETS    5 // value of all vehicles and buildings
#define COST_CASH          6 // Cash
#define COST_NETWEALTH     7 // Total Cash + Assets
#define COST_PROFIT    8 // 3-(0+1+2+4)
#define COST_OPERATING_PROFIT 9 // 3-(1+4)
#define COST_MARGIN 10 // 9/(1+4)
#define COST_TRANSPORTED_GOODS 11// all transported goods
#define MAX_HISTORY_YEARS  12 // number of years to keep history
#define MAX_HISTORY_MONTHS  12 // number of months to keep history


class karte_t;
class fabrik_t;
class stadt_t;
class gebaeude_t;
class koord3d;
class money_frame_t;
class schedule_list_gui_t;
class ware_production_t;

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

    enum zustand {NEUE_ROUTE,BAUE_VERBINDUNG,BAUE_BUS_START,BAUE_BUS_ZIEL,CHECK_CONVOI};

    enum subzustand {
		NR_INIT,
		NR_SAMMLE_ROUTEN,
		NR_BAUE_ROUTE1,
		NR_BAUE_SIMPLE_SCHIENEN_ROUTE,
		NR_BAUE_SCHIENEN_ROUTE1,
		NR_BAUE_SCHIENEN_ROUTE2,
		NR_BAUE_STRASSEN_ROUTE,
		NR_BAUE_STRASSEN_ROUTE2,
		NR_BAUE_CLEAN_UP,
		NR_RAIL_SUCCESS,
		NR_ROAD_SUCCESS
	};

    enum { MAX_KONTO_VERZUG = 3 };

private:

    char spieler_name_buf[16];

    /*
     * holds total number of all halts, ever built
     * @author hsiegeln
     */
    int haltcount;

    /*
     * if this is true, this AI will try passenger transport only
     * @author prissi
     */
    bool passenger_transport;

    /*
     * if this is false, this AI won't use roads
     * @author prissi
     */
    bool road_transport;

    /*
     * if this is false, this AI won't use rails
     * @author prissi
     */
    bool rail_transport;

    /*
     * if this is false, this AI won't use ships
     * @author prissi
     */
    bool ship_transport;

    /**
     * Finance array, indexed by type
     * @author Owen Rudge
     */
    sint64 finances[MAX_COST];
    sint64 old_finances[MAX_COST];


    /**
    * Finance History - will supercede the finances by Owen Rudge
    * Will hold finances for the most recent 12 years
    * @author hsiegeln
    */

    sint64 finance_history_year[MAX_HISTORY_YEARS][MAX_COST];
    sint64 finance_history_month[MAX_HISTORY_MONTHS][MAX_COST];

    /**
     * Monthly maintenance cost
     * @author Hj. Malthaner
     */
    sint32 maintenance;

    /**
     * Die Welt in der gespielt wird.
     *
     * @author Hj. Malthaner
     */
    karte_t *welt;

    /* Money dialoge, unique for every player
     * @author prissi
     */
    money_frame_t *money_frame;

    /* Line dialoge, unique for every player
     * @author prissi
     */
    schedule_list_gui_t *line_frame;

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

    slist_tpl<halthandle_t> halt_list; ///< Liste der Haltestellen

    char  texte[50][32];
    sint8 text_alter[50];
    koord text_pos[50];

    int last_message_index;

    // vars für die KI

    enum zustand state;
    enum subzustand substate;

  /* test more than one supplier and more than one good *
   * save last factory for building next supplier/consumer *
   * @author prissi
   */
    fabrik_t *start;
    fabrik_t *last_start;
    int start_ware;
    fabrik_t *ziel;
    fabrik_t *last_ziel;

  // we will use this vehicle!
  const vehikel_besch_t *rail_vehicle;
  const vehikel_besch_t *rail_engine;
  const vehikel_besch_t *road_vehicle;
  // and the convoi will run on this track:
  const weg_besch_t *rail_weg ;
  const weg_besch_t *road_weg ;

    int count_rail;
    int count_road;
  // multi-purpose counter
  int count;

    int gewinn;

	// passenger KI
	const stadt_t *start_stadt;
	const stadt_t *end_stadt;	// target is town
	const gebaeude_t *end_ausflugsziel;

    // ende KI vars

    // main functions for KI
    void do_passenger_ki();
    void do_ki();

    bool suche_platz(int x, int y, koord *);
    bool suche_platz(int x, int y, int dx, int dy,
         koord off,
         koord *);

    // all for passenger transport
    bool is_connected(halthandle_t halt, koord upperleft, koord lowerright);
    halthandle_t  get_our_hub( const stadt_t *s );
    koord built_hub( const koord pos, int radius );
    void create_bus_transport_vehikel(koord startpos,int anz_vehikel,koord *stops,int anzahl,bool do_wait);

    int baue_bahnhof(koord3d quelle,koord *p, int anz_vehikel,fabrik_t *fab);

    /* these two routines calculate the income
     * @author prissi
     */
    int rating_transport_quelle_ziel(fabrik_t *qfab,const ware_production_t *ware,fabrik_t *zfab);
    int guess_gewinn_transport_quelle_ziel(fabrik_t *qfab,const ware_production_t *ware, int qware_nr, fabrik_t *zfab);

    /* These two routines calculate, which route next
     * @author Hj. Malthaner
     * @author prissi
     */
  int suche_transport_ziel(fabrik_t *quelle, int *quelle_ware, fabrik_t **ziel);
    int suche_transport_quelle(fabrik_t **quelle,int *quelle_ware, fabrik_t *ziel);


    void create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel);
    void create_rail_transport_vehikel(const koord pos1,const koord pos2, int anz_vehikel, int ladegrad);


    bool suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab);    // neue Transportroute anlegen


    void init_texte();

    void add_message(koord k, int summe);

    /**
     * Kennfarbe (Fahrzeuge, Gebäude) des Speielers
     * @author Hj. Malthaner
     */
    uint8 kennfarbe;

    /**
     * Player number; only player 0 can do interaction
     * @author Hj. Malthaner
     */
	uint8 player_nr;

public:

    /**
     * Ist dieser Spieler ein automatischer Spieler?
     * @author Hj. Malthaner
     */
    bool automat;

	// @author hsiegeln
	simlinemgmt_t simlinemgmt;
	schedule_list_gui_t *get_line_frame();

    /**
     * Age messages (move them upwards)
     * @author Hj. Malthaner
     */
    void age_messages(long delta_t);

	/* Handles player colors ...
	 * @author prissi
	 */
	uint8 get_player_color() const { return kennfarbe; }
	void set_player_color(uint8 col) { kennfarbe=col; }

    /**
     * Name of the player
     * @author player
     */
	const char *gib_name();
	const uint8 get_player_nr() const {return player_nr; }

	/* return true, if the owner is none, myself or player(1)
	 * @author prissi
	 */
	bool check_owner( const spieler_t *sp ) const;

    /**
     * activates and queries player status
     * @author player
     */
     bool is_active() { return automat; }
     bool set_active(bool new_state);

     // true if this can do passenger transport ...
     bool has_passenger() const { return player_nr == 0 || passenger_transport; }

    /**
     * Konstruktor
     * @param welt Die Welt (Karte) des Spiels
     * @param color Kennfarbe des Spielers
     * @author Hj. Malthaner
     */
    spieler_t(karte_t *welt, uint8 color, uint8 player_nr );

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
		sint64 buche(sint64 betrag)
		{
			konto += betrag;
			return konto;
		}

    /**
     * Adds somme amount to the maintenance costs
     * @param change the change
     * @return the new maintenance costs
     * @author Hj. Malthaner
     */
		sint32 add_maintenance(sint32 change)
		{
			maintenance += change;
			return maintenance;
		}

    // Owen Rudge, finances
    sint64 buche(sint64 betrag, koord k, int type);
    sint64 buche(sint64 betrag, int type);

    /**
     * @return Kontostand als double (Gleitkomma) Wert
     * @author Hj. Malthaner
     */
    double gib_konto_als_double() const { return konto / 100.0; }

    /**
     * @return true wenn Konto Überzogen ist
     * @author Hj. Malthaner
     */
    int gib_konto_ueberzogen() const { return konto_ueberzogen; }


    /**
     * Zeigt Meldungen aus der Queue des Spielers auf dem Bildschirm an
     * @author Hj. Malthaner
     */
    void display_messages();


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
     * needed to transfer ownership
     * @author prissi
     */
    void halt_add(halthandle_t h);

    /**
     * Entfernt eine Haltestelle des Spielers aus der Liste
     * @author Hj. Malthaner
     */
    void halt_remove(halthandle_t halt);

    /* returns true for a halt of the player at that position
     * @author prissi
     */
    bool is_my_halt(koord pos) const;
    unsigned is_my_halt(koord3d pos) const;	// return the boden_count+1, 0==fails

    /**
     * Gets haltcount, for naming purposes
     * @author hsiegeln
     */
    int get_haltcount() const { return haltcount; }

    /**
     * Lädt oder speichert Zustand des Spielers
     * @param file die offene Save-Datei
     * @author Hj. Malthaner
     */
    void rdwr(loadsave_t *file);

	/*
	 * called after game is fully loaded;
	 */
	void laden_abschliessen();


    // fuer tests
    koord platz1, platz2;

    bool create_simple_road_transport();    // neue Transportroute anlegen
    bool create_simple_rail_transport();

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
    sint64 get_finance_history_year(int year, int type) { return finance_history_year[year][type]; }

    /**
    * Returns pointer to finance history for player
    * @author hsiegeln
    */
    sint64* get_finance_history_year() { return *finance_history_year; }
  sint64* get_finance_history_month() { return *finance_history_month; }

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
    void roll_finance_history_year();
    void roll_finance_history_month();

    /*
     * returns pointer to our money frame
     * @author prissi
     */
    money_frame_t *gib_money_frame();

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
    void bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel);

private:
	/* undo informations *
	 * @author prissi
	 */
	vector_tpl<koord3d> last_built;
	waytype_t undo_type;

public:
	void init_undo(waytype_t t, unsigned short max );
	void add_undo(koord3d k);
	bool undo();

	// headquarter stuff
private:
	sint32 headquarter_level;
	koord headquarter_pos;

public:
	void add_headquarter(short hq_level, koord hq_pos)
	{
		headquarter_level = hq_level;
		headquarter_pos = hq_pos;
	}
	koord get_headquarter_pos(void) const { return headquarter_pos; }
	short get_headquarter_level(void) const { return headquarter_level; }
};

#endif
