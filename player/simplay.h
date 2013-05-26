/*
 * Copyright (c) 1997 - 2002 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef simplay_h
#define simplay_h

#include "../dataobj/pwd_hash.h"
#include "../simtypes.h"
#include "../simlinemgmt.h"

#include "../halthandle_t.h"
#include "../convoihandle_t.h"

#include "../dataobj/koord.h"

#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"

#include "../simworld.h"


class karte_t;
class fabrik_t;
class koord3d;
class werkzeug_t;
class finance_t;

/**
 * Class to hold informations about one player/company. AI players are derived from this class.
 */
class spieler_t
{
public:
	enum { EMPTY=0, HUMAN=1, AI_GOODS=2, AI_PASSENGER=3, MAX_AI, PASSWORD_PROTECTED=128 };

	// BG, 2009-06-06: differ between infrastructure and vehicle maintenance 
	enum { MAINT_INFRASTRUCTURE=0, MAINT_VEHICLE=1, MAINT_COUNT };

protected:
	char spieler_name_buf[256];

	/* "new" finance history */
	finance_t *finance;

	/**
	 * Die Welt in der gespielt wird.
	 *
	 * @author Hj. Malthaner
	 */
	static karte_t *welt;

	// when was the company founded
	uint16 player_age;

	class income_message_t {
	public:
		char str[33];
		koord pos;
		sint32 amount;
		sint8 alter;
		income_message_t() { str[0]=0; alter=127; pos=koord::invalid; amount=0; }
		income_message_t( sint32 betrag, koord pos );
		void * operator new(size_t s);
		void operator delete(void *p);
	};

	slist_tpl<income_message_t *>messages;

	void add_message(koord k, sint32 summe);

	/**
	 * Kennfarbe (Fahrzeuge, Geb�ude) des Speielers
	 * @author Hj. Malthaner
	 */
	uint8 kennfarbe1, kennfarbe2;

	/**
	 * Player number
	 * @author Hj. Malthaner
	 */
	uint8 player_nr;

	/**
	 * Adds some amount to the maintenance costs.
	 * @param change the change
	 * @return the new maintenance costs
	 * @author Hj. Malthaner
	 */
	sint32 add_maintenance(sint32 change, waytype_t const wt=ignore_wt);

	/**
	 * Ist dieser Spieler ein automatischer Spieler?
	 * @author Hj. Malthaner
	 */
	bool automat;

	/**
	 * Are this player allowed to make any changes?
	 * @author Hj. Malthaner
	 */
	bool locked;

	bool unlock_pending;

	// contains the password hash for local games
	pwd_hash_t pwd_hash;

	/**
	 * Allow access to the player number
	 * in the array.
	 * @author: jamespetts, October 2011
	 */
	bool access[MAX_PLAYER_COUNT];

public:
	/**
	 * Sums up "count" with number of convois in statistics,
	 * supersedes buche( count, COST_ALL_CONVOIS).
	 * @author jk271
	 */
	void book_convoi_number(int count);

	/**
	 * Adds construction costs to accounting statistics.
	 * @param amount How much does it cost
	 * @param tt type of transport
	 * @author jk271
	 */
	static void book_construction_costs(spieler_t * const sp, const sint64 amount, const koord k, const waytype_t wt=ignore_wt);

	/*
	 * displayes amount of money when koordinates and on screen
	 * reworked function buche()
	 */
	void add_money_message(const sint64 amount, const koord k);

	/**
	 * Accounts bought/sold vehicles.
	 * @param price money used for purchase of vehicle,
	 *              negative value = vehicle bought,
	 *              positive value = vehicle sold
	 * @param tt type of transport for accounting purpose
	 * @author jk271
	 */
	void book_new_vehicle(const sint64 price, const koord k, const waytype_t wt=ignore_wt);

	/**
	 * Adds income to accounting statistics.
	 * @param amount earned money
	 * @param tt transport type used in accounting statistics
	 * @param cathegory parameter
	 * 	0 ... passenger
	 *	1 ... mail
	 *	2 ... good (and powerlines revenue)
	 * @author jk271
	 */
	void book_revenue(const sint64 amount, const koord k, const waytype_t wt=ignore_wt, sint32 cathegory=2);

	/**
	 * Adds running costs to accounting statistics.
	 * @param amount How much does it cost
	 * @param wt
	 * @author jk271
	 */
	void book_running_costs(const sint64 amount, const waytype_t wt=ignore_wt);

	/**
	 * Adds monthly vehicle maintenance to accounting statistics.
	 * @param amount (should be negative, will be adjusted for bits_per_month)
	 * @param wt type of transport for accounting
	 * @author neroden
	 */
	void book_vehicle_maintenance(const sint64 amount, const waytype_t wt=ignore_wt);

	/**
	 * Books toll paid by our company to someone else.
	 * @param amount money paid to our company
	 * @param tt type of transport used for assounting statistisc
	 * @author jk271
	 */
	void book_toll_paid(const sint64 amount, const waytype_t wt=ignore_wt);

	/**
	 * Books toll paid to our company by someone else.
	 * @param amount money paid for usage of our roads,railway,channels, ... ; positive sign
	 * @param tt type of transport used for assounting statistisc
	 * @author jk271
	 */
	void book_toll_received(const sint64 amount, waytype_t wt=ignore_wt);

	/**
	 * Add amount of transported passenger, mail, goods to accounting statistics.
	 * @param amount sum of money
	 * @param wt way type
	 * @param index 0 = passenger, 1 = mail, 2 = goods
	 * @author jk271
	 */
	void book_transported(const sint64 amount, const waytype_t wt=ignore_wt, int index=2);

	/**
	 * Add amount of delivered passenger, mail, goods to accounting statistics.
	 * @param amount sum of money
	 * @param wt way type
	 * @param index 0 = passenger, 1 = mail, 2 = goods
	 */
	void book_delivered(const sint64 amount, const waytype_t wt=ignore_wt, int index=2);

	bool has_money_or_assets() const;

	finance_t * get_finance() { return finance; }

	virtual bool set_active( bool b ) { return automat = b; }

	bool is_active() const { return automat; }

	bool is_locked() const { return locked; }

	bool is_unlock_pending() const { return unlock_pending; }

	void unlock(bool unlock_, bool unlock_pending_=false) { locked = !unlock_; unlock_pending = unlock_pending_; }

	void check_unlock( const pwd_hash_t& hash ) { locked = (pwd_hash != hash); }

	// some routine needs this for direct manipulation
	pwd_hash_t& access_password_hash() { return pwd_hash; }

	// this type of AIs identifier
	virtual uint8 get_ai_id() const { return HUMAN; }

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
	void set_name(const char *);

	sint8 get_player_nr() const {return player_nr; }

	/**
	 * return true, if the owner is none, myself or player(1), i.e. the ownership _can be taken by player test
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

	/**
	 * This is safe to be called with sp==NULL
	 */
	static sint32 add_maintenance(spieler_t *sp, sint32 const change, waytype_t const wt=ignore_wt)
	{
		if(sp) {
			return sp->add_maintenance(change, wt);
		}
		return 0;
	}

	/**
	 * Cached value of scenario completion percentage.
	 * To get correct values for clients call scenario_t::get_completion instead.
	 */
	sint32 get_scenario_completion() const;

	void set_scenario_completion(sint32 percent);

	/**
	 * @return Kontostand als double (Gleitkomma) Wert
	 * @author Hj. Malthaner
	 */
	double get_konto_als_double() const;

	/**
	 * Return the amount of cash that the player has
	 * in SimuCents. Integer method needed where this
	 * method is used in non-GUI code in multi-player games.
	 * @author: jamespetts, December 2012
	 */
	sint64 get_player_cash_int() const { return konto; }

	/**
	 * @return true wenn Konto �berzogen ist
	 * @author Hj. Malthaner
	 */
	int get_account_overdrawn() const;

	/**
	 * Zeigt Meldungen aus der Queue des Spielers auf dem Bildschirm an
	 * @author Hj. Malthaner
	 */
	void display_messages();

	/**
	 * Wird von welt in kurzen abst�nden aufgerufen
	 * @author Hj. Malthaner
	 */
	virtual void step();

	/**
	 * Wird von welt nach jedem monat aufgerufen
	 * @author Hj. Malthaner
	 * @returns false if player has to be removed (bankrupt/inactive)
	 */
	virtual bool neuer_monat();

	/**
	 * Methode fuer jaehrliche Aktionen
	 * @author Hj. Malthaner
	 */
	virtual void neues_jahr() {}

	/**
	 * L�dt oder speichert Zustand des Spielers
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
	* Returns the world the player is in
	* @author hsiegeln
	*/
	static karte_t *get_welt() { return welt; }

	/**
	* Calculates the assets of the player
	*/
	void calc_assets();

	/**
	* Updates the assets value of the player
	*/
	void update_assets(sint64 const delta, const waytype_t wt = ignore_wt);

	/**
	 * R�ckruf, um uns zu informieren, dass ein Vehikel ein Problem hat
	 * @author Hansj�rg Malthaner
	 * @date 26-Nov-2001
	 */
	virtual void bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel);

	/**
	 * Tells the player the result of tool-work commands
	 * If player is active then play sound, popup error msg etc
	 * AI players react upon this call and proceed
	 * local is true if tool was called by player on our client
	 * @author Dwachs
	 */
	virtual void tell_tool_result(werkzeug_t *tool, koord3d pos, const char *err, bool local);

	/**
	 * Tells the player that the factory
	 * is going to be deleted (flag==0)
	 * Bernd Gabriel, Dwachs
	 */
	enum notification_factory_t {
		notify_delete	// notified immediately before object is deleted (and before nulled in the slist_tpl<>)!
	};
	virtual void notify_factory(notification_factory_t, const fabrik_t*) {}

private:
	/* undo informations *
	 * @author prissi
	 */
	vector_tpl<koord3d> last_built;
	waytype_t undo_type;

public:
	void init_undo(waytype_t t, unsigned short max );
	void add_undo(koord3d k);
	sint64 undo();

   /**
     * Is player allowed to purchase something of this price, or is player
     * too deep in debt?  (This routine allows the public service player to
	 * always buy anything.)
     * @returns whether player is allowed to purchase something of cost "price"
     * @params price
     */
	inline bool can_afford(sint64 price) const
	{
		if (player_nr == 1) return true; // Public service can always afford anything
		return finance->can_afford(price);
	}

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

	/**
	 * Used for summing the revenue 
	 * generatedfor this player by other  
	 * players' convoys whilst unloading.
	 * This value is not saved, as it is not
	 * carried over between sync steps.
	 * @author: jamespetts, October 2011
	 */
	sint64 interim_apportioned_revenue;

	bool allows_access_to(uint8 other_player_nr) const { return this == NULL || player_nr == other_player_nr || access[other_player_nr]; }
	void set_allow_access_to(uint8 other_player_nr, bool allow) { access[other_player_nr] = allow; }
};

#endif
