/*
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


enum {
	COST_CONSTRUCTION=0,// Construction
	COST_VEHICLE_RUN,   // Vehicle running costs
	COST_NEW_VEHICLE,   // New vehicles
	COST_INCOME,        // Income
	COST_MAINTENANCE,   // Upkeep
	COST_ASSETS,        // value of all vehicles and buildings
	COST_CASH,          // Cash
	COST_NETWEALTH,     // Total Cash + Assets
	COST_PROFIT,        // COST_POWERLINES+COST_INCOME-(COST_CONSTRUCTION+COST_VEHICLE_RUN+COST_NEW_VEHICLE+COST_MAINTENANCE)
	COST_OPERATING_PROFIT, // COST_POWERLINES+COST_INCOME-(COST_VEHICLE_RUN+COST_MAINTENANCE)
	COST_MARGIN,        // COST_OPERATING_PROFIT/(COST_VEHICLE_RUN+COST_MAINTENANCE)
	COST_TRANSPORTED_GOODS, // all transported goods
	COST_POWERLINES,	  // revenue from the power grid
	MAX_COST
};
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
	enum { MAX_KONTO_VERZUG = 3 };

private:
	char spieler_name_buf[16];

	/*
	 * holds total number of all halts, ever built
	 * @author hsiegeln
	 */
	sint32 haltcount;

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
	sint32 konto_ueberzogen;

	/**
	 * Zählt die steps
	 * @author Hj. Malthaner
	 */
	sint32 steps;

	slist_tpl<halthandle_t> halt_list; ///< Liste der Haltestellen

	char  texte[50][32];
	sint8 text_alter[50];
	koord text_pos[50];

	int last_message_index;

	void init_texte();

	void add_message(koord k, int summe);

	/**
	 * Kennfarbe (Fahrzeuge, Gebäude) des Speielers
	 * @author Hj. Malthaner
	 */
	uint8 kennfarbe1, kennfarbe2;

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
	uint8 get_player_color1() const { return kennfarbe1; }
	uint8 get_player_color2() const { return kennfarbe2; }
	void set_player_color(uint8 col1, uint8 col2);

	/**
	 * Name of the player
	 * @author player
	 */
	const char* gib_name() const;
	const sint8 get_player_nr() const {return player_nr; }

	/* return true, if the owner is none, myself or player(1)
	* @author prissi
	*/
	bool check_owner( const spieler_t *sp ) const;

	/**
	 * @param welt Die Welt (Karte) des Spiels
	 * @param color Kennfarbe des Spielers
	 * @author Hj. Malthaner
	 */
	spieler_t(karte_t *welt, uint8 player_nr );

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
	karte_t *get_welt() { return welt; }

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

	// true if this can do passenger transport ...
	bool has_passenger() const;

/**************************************** AI-sutff from here ******************************************/
private:
	enum zustand {
		NR_INIT,
		NR_SAMMLE_ROUTEN,
		NR_BAUE_ROUTE1,
		NR_BAUE_SIMPLE_SCHIENEN_ROUTE,
		NR_BAUE_STRASSEN_ROUTE,
		NR_BAUE_WATER_ROUTE,
		NR_BAUE_CLEAN_UP,
		NR_RAIL_SUCCESS,
		NR_ROAD_SUCCESS,
		NR_WATER_SUCCESS,
		CHECK_CONVOI
	};

	// vars für die KI
	enum zustand state;

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

	/* test more than one supplier and more than one good *
	 * save last factory for building next supplier/consumer *
	 * @author prissi
	 */
	fabrik_t *root;

	// actual route to be built between those
	fabrik_t *start;
	fabrik_t *ziel;
	const ware_besch_t *freight;

	// we will use this vehicle!
	const vehikel_besch_t *rail_vehicle;
	const vehikel_besch_t *rail_engine;
	const vehikel_besch_t *road_vehicle;
	const vehikel_besch_t *ship_vehicle;

	// and the convoi will run on this track:
	const weg_besch_t *rail_weg ;
	const weg_besch_t *road_weg ;

	sint32 count_rail;
	sint32 count_road;

	// multi-purpose counter
	sint32 count;

	// time to wait before next contruction
	uint32 next_contruction_steps;

	/* start and end stop position (and their size) */
	koord platz1, size1, platz2, size2;

	// KI helper class
	class fabconnection_t{
		koord fab1;
		koord fab2;	// koord1 must be always "smaller" than koord2
		const ware_besch_t *ware;

	public:
		fabconnection_t( koord k1, koord k2, const ware_besch_t *w ) : fab1(k1), fab2(k2), ware(w) {}

		const bool operator != (const fabconnection_t & k) { return fab1 != k.fab1  ||  fab2 !=  k.fab2  ||  ware != k.ware; }
		const bool operator == (const fabconnection_t & k) { return (fab1 == k.fab1  &&  fab2 ==  k.fab2  &&  ware == k.ware); }
//		const bool operator < (const fabconnection_t & k) { return (abs(fab1.x)+abs(fab1.y)) - (abs(k.fab1.x)+abs(k.fab1.y)) < 0; }
	};

	slist_tpl<fabconnection_t> forbidden_conections;

	// return true, if this a route to avoid (i.e. we did a consturction without sucess here ...)
	bool is_forbidden(const koord start_pos, const koord end_pos, const ware_besch_t *w ) const;

	// return true, if there is already a connection
	bool is_connected(const koord star_pos, const koord end_pos, const ware_besch_t *wtyp) const;

	/* recursive lookup of a factory tree:
	 * sets start and ziel to the next needed supplier
	 * start always with the first branch, if there are more goods
	 */
	bool get_factory_tree_lowest_missing( fabrik_t *fab );

	/* recursive lookup of a tree and how many factories must be at least connected
	 * returns -1, if this tree is incomplete
	 */
	int get_factory_tree_missing_count( fabrik_t *fab );

	/**
	 * Find the first water tile using line algorithm von Hajo
	 * start MUST be on land!
	 **/
	koord find_shore(koord start, koord end) const;
	bool find_harbour(koord &start, koord &size, koord target);

	// find space for stations
	bool suche_platz(koord pos, koord &size, koord *dirs) const;
	bool suche_platz(koord &start, koord &size, koord target, koord off);
	bool suche_platz1_platz2(fabrik_t *qfab, fabrik_t *zfab, int length);

	int baue_bahnhof(koord3d quelle,koord *p, int anz_vehikel,fabrik_t *fab);

	// create way and stops for these routes
	bool create_ship_transport_vehikel(fabrik_t *qfab, int anz_vehikel);
	void create_road_transport_vehikel(fabrik_t *qfab, int anz_vehikel);
	void create_rail_transport_vehikel(const koord pos1,const koord pos2, int anz_vehikel, int ladegrad);

	bool create_simple_road_transport();    // neue Transportroute anlegen
	bool create_simple_rail_transport();

	void do_ki();

	// all for passenger transport
	const stadt_t *start_stadt;
	const stadt_t *end_stadt;	// target is town
	const gebaeude_t *end_ausflugsziel;

	halthandle_t  get_our_hub( const stadt_t *s );
	koord built_hub( const koord pos, int radius );
	void create_bus_transport_vehikel(koord startpos,int anz_vehikel,koord *stops,int anzahl,bool do_wait);

	void do_passenger_ki();

public:
	/**
	 * activates and queries player status
	 * @author player
	 */
	bool is_active() { return automat; }
	bool set_active(bool new_state);
};

#endif
