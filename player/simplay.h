/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef simplay_h
#define simplay_h

#include "../simtypes.h"
#include "../simlinemgmt.h"

#include "../halthandle_t.h"
#include "../convoihandle_t.h"

#include "../dataobj/koord.h"

#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"

#include "../simworld.h"


enum player_cost {
	COST_CONSTRUCTION = 0,	// Construction
	COST_VEHICLE_RUN,		// Vehicle running costs
	COST_NEW_VEHICLE,		// New vehicles
	COST_INCOME,			// Income
	COST_MAINTENANCE,		// Upkeep
	COST_ASSETS,			// value of all vehicles and buildings
	COST_CASH,				// Cash
	COST_NETWEALTH,			// Total Cash + Assets
	COST_PROFIT,			// COST_POWERLINES+COST_INCOME-(COST_CONSTRUCTION+COST_VEHICLE_RUN+COST_NEW_VEHICLE+COST_MAINTENANCE)
	COST_OPERATING_PROFIT,	// COST_POWERLINES+COST_INCOME-(COST_VEHICLE_RUN+COST_MAINTENANCE)
	COST_MARGIN,			// COST_OPERATING_PROFIT/(COST_VEHICLE_RUN+COST_MAINTENANCE)
	COST_ALL_TRANSPORTED,	// all transported goods
	COST_POWERLINES,		// revenue from the power grid
	COST_TRANSPORTED_PAS,	// number of passengers that actually reached destination
	COST_TRANSPORTED_MAIL,
	COST_TRANSPORTED_GOOD,
	COST_ALL_CONVOIS,		// number of convois
	COST_SCENARIO_COMPLETED,// scenario success (only useful if there is one ... )
	COST_INTEREST,			// Interest paid servicing debt
	COST_CREDIT_LIMIT,		// Player's credit limit.
	MAX_PLAYER_COST
};

#define MAX_PLAYER_HISTORY_YEARS  (12) // number of years to keep history
#define MAX_PLAYER_HISTORY_MONTHS  (12) // number of months to keep history


class fabrik_t;
class stadt_t;
class gebaeude_t;
class koord3d;
class ware_production_t;

/**
 * play info for simutrans human and AI are derived from this class
 */
class spieler_t
{
public:
	enum { MAX_KONTO_VERZUG = 3 };

	enum { EMPTY=0, HUMAN=1, AI_GOODS=2, AI_PASSENGER=3, MAX_AI };

protected:
	char spieler_name_buf[256];

	/*
	 * holds total number of all halts, ever built
	 * @author hsiegeln
	 */
	sint32 haltcount;

	/**
	* Finance History - will supercede the finances by Owen Rudge
	* Will hold finances for the most recent 12 years
	* @author hsiegeln
	*/
	sint64 finance_history_year[MAX_PLAYER_HISTORY_YEARS][MAX_PLAYER_COST];
	sint64 finance_history_month[MAX_PLAYER_HISTORY_MONTHS][MAX_PLAYER_COST];

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
	static karte_t *welt;

	/**
	 * Der Kontostand.
	 * "The account balance." (Google)
	 *
	 * @author Hj. Malthaner
	 */
	sint64 konto; //"account" (Google)

	/**
	 * Zählt wie viele Monate das Konto schon ueberzogen ist
	 * "Count how many months the account is already overdrawn"  (Google)
	 *
	 * @author Hj. Malthaner
	 */
	sint32 konto_ueberzogen; //"overdrawn account" (Google)

	//slist_tpl<halthandle_t> halt_list; ///< Liste der Haltestellen
	vector_tpl<halthandle_t> halt_list; ///< "List of the stops" (Babelfish)

	class income_message_t {
	public:
		char str[33];
		sint8 alter;
		koord pos;
		income_message_t() { str[0]=0; alter=127; pos==koord::invalid; }
		income_message_t( sint32 betrag, koord pos );
		void * operator new(size_t s);
		void operator delete(void *p);
	};

	slist_tpl<income_message_t *>messages;

	char  texte[50][32];
	sint8 text_alter[50];
	koord text_pos[50];

	int last_message_index;

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

	/**
	 * Ist dieser Spieler ein automatischer Spieler?
	 * @author Hj. Malthaner
	 */
	bool automat;

public:
	virtual bool set_active( bool b ) { return automat = b; }

	bool is_active() const { return automat; }

	// this type of AIs identifier
	virtual uint8 get_ai_id() { return HUMAN; }

	// @author hsiegeln
	simlinemgmt_t simlinemgmt;

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
	const char* get_name() const;
	sint8 get_player_nr() const {return player_nr; }

	/**
	 * return true, if the owner is none, myself or player(1), i.e. the ownership can be taken by player test
	 * @author prissi
	 */
	static bool check_owner( const spieler_t *owner, const spieler_t *test );

	/**
	 * @param welt Die Welt (Karte) des Spiels
	 * @param color Kennfarbe des Spielers
	 * @author Hj. Malthaner
	 */
	spieler_t(karte_t *welt, uint8 player_nr );

	virtual ~spieler_t();

	sint32 get_maintenance() const { return maintenance; }

	static sint32 add_maintenance(spieler_t *sp, sint32 change) {
		if(sp) {
			sp->maintenance += change;
			return sp->maintenance;
		}
		return 0;
	}

	// Owen Rudge, finances
	void buche(sint64 betrag, koord k, enum player_cost type);

	// do the internal accounting (currently only used externally for running costs of convois)
	void buche(sint64 betrag, enum player_cost type);

	// this is also save to be called with sp==NULL, which may happen for unowned objects like bridges, ways, trees, ...
	static void accounting( spieler_t *sp, const sint64 betrag, koord k, enum player_cost pc );
	
	static bool accounting_with_check( spieler_t *sp, const sint64 betrag, koord k, enum player_cost pc );

	/**
	 * @return Kontostand als double (Gleitkomma) Wert
	 * @author Hj. Malthaner
	 */
	double get_konto_als_double() const { return konto / 100.0; }

	/**
	 * @return true wenn Konto Überzogen ist
	 * @author Hj. Malthaner
	 */
	int get_konto_ueberzogen() const { return konto_ueberzogen; }

	/**
	 * Zeigt Meldungen aus der Queue des Spielers auf dem Bildschirm an
	 * @author Hj. Malthaner
	 */
	void display_messages();

	/**
	 * Wird von welt in kurzen abständen aufgerufen
	 * @author Hj. Malthaner
	 */
	virtual void step();

	/**
	 * Wird von welt nach jedem monat aufgerufen
	 * @author Hj. Malthaner
	 */
	virtual void neuer_monat();

	/**
	 * Methode fuer jaehrliche Aktionen
	 * @author Hj. Malthaner
	 */
	virtual void neues_jahr() {}

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
	virtual void rdwr(loadsave_t *file);

	/*
	 * called after game is fully loaded;
	 */
	virtual void laden_abschliessen();

	virtual void rotate90( const sint16 y_size );

	/**
	* Returns the finance history for player
	* @author hsiegeln
	*/
	sint64 get_finance_history_year(int year, int type) { return finance_history_year[year][type]; }
	sint64 get_finance_history_month(int month, int type) { return finance_history_month[month][type]; }

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
	static karte_t *get_welt() { return welt; }

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

	/**
	 * Rückruf, um uns zu informieren, dass ein Vehikel ein Problem hat
	 * @author Hansjörg Malthaner
	 * @date 26-Nov-2001
	 */
	virtual void bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel);

private:
	/* undo informations *
	 * @author prissi
	 */
	vector_tpl<koord3d> last_built;
	waytype_t undo_type;

	// The maximum amount overdrawn that a player can be
	// before no more purchases can be made.
	sint32 base_credit_limit;

protected:
	sint64 calc_credit_limit();

	sint64 get_base_credit_limit();

public:
	void init_undo(waytype_t t, unsigned short max );
	void add_undo(koord3d k);
	bool undo();

	// Checks the affordability of any possible purchase.
	// Check is disapplied to the public service player.
	inline bool can_afford(sint64 price) const
	{
		return player_nr == 1 || (price < (konto + finance_history_month[0][COST_CREDIT_LIMIT]) || welt->get_einstellungen()->insolvent_purchases_allowed() || welt->get_einstellungen()->is_freeplay());
	}

	sint64 get_credit_limit() const { return finance_history_month[0][COST_CREDIT_LIMIT]; }

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

	void ai_bankrupt();
};



/**************************************** AI-sutff from here ******************************************/
#endif
