/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
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
	 * Are this player allowed to do any changes?
	 * @author Hj. Malthaner
	 */
	bool locked;

	bool unlock_pending;

	// contains the password hash for local games
	pwd_hash_t pwd_hash;

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
	 *              negative value = vehicle sold
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
	 * @return true wenn Konto Überzogen ist
	 * @author Hj. Malthaner
	 */
	int get_account_overdrawn() const;

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
	 * @returns false if player has to be removed (bankrupt/inactive)
	 */
	virtual bool neuer_monat();

	/**
	 * Methode fuer jaehrliche Aktionen
	 * @author Hj. Malthaner
	 */
	virtual void neues_jahr() {}

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
	 * Rückruf, um uns zu informieren, dass ein Vehikel ein Problem hat
	 * @author Hansjörg Malthaner
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

#endif
