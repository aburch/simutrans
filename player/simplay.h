/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef PLAYER_SIMPLAY_H
#define PLAYER_SIMPLAY_H


#include "../network/pwd_hash.h"
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
class tool_t;
class finance_t;

/**
 * Class to hold informations about one player/company. AI players are derived from this class.
 */
class player_t
{
public:
	enum { EMPTY=0, HUMAN=1, AI_GOODS=2, AI_PASSENGER=3, MAX_AI, PASSWORD_PROTECTED=128 };

protected:
	char player_name_buf[256];

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

	/**
	* Floating massages for all players here
	*/
	class income_message_t {
	public:
		char str[33];
		koord pos;
		sint64 amount;
		sint8 alter;
		income_message_t() { str[0]=0; alter=127; pos=koord::invalid; amount=0; }
		income_message_t( sint64 betrag, koord pos );
		void * operator new(size_t s);
		void operator delete(void *p);
	};

	slist_tpl<income_message_t *>messages;

	/**
	 * Creates new income message entry or merges with existing one if the
	 * most recent one is at the same coordinate
	 */
	void add_message(sint64 amount, koord k);

public:
	/**
	 * Displays amount of money when koordinates are on screen
	 */
	void add_money_message(sint64 amount, koord k);

	/*
	* This defines the three possible solvency states of a player. Solvent is normal.
	* A company in administration can keep playing, but may be taken over. A company
	* in liquidation ceases operation and its assets are put up for sale. After a
	* period, if they be not sold, the assets will be scrapped.
	*/
	enum solvency_status { solvent, in_administration, in_liquidation };

protected:
	/**
	 * Colors of the player
	 * @author Hj. Malthaner
	 */
	uint8 player_color_1, player_color_2;

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
	sint64 add_maintenance(sint64 change, waytype_t const wt=ignore_wt);

	/**
	 * Is this player an AI player?
	 * @author Hj. Malthaner
	 */
	bool active;

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


	/* This flag is set if the player has already been
	 * warned this month that there is insufficient money
	 * for automatic way renewals*/
	bool has_been_warned_about_no_money_for_renewals;

	/// Any player may take over this company at any time if
	/// this evaluates to true.
	bool allow_voluntary_takeover;

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
	 * @param wt type of transport
	 * @author jk271
	 */
	static void book_construction_costs(player_t * const player, const sint64 amount, const koord k, const waytype_t wt=ignore_wt);

	/**
	 * Accounts bought/sold vehicles.
	 * @param price money used for purchase of vehicle,
	 *              negative value = vehicle bought,
	 *              positive value = vehicle sold
	 * @param wt type of transport for accounting purpose
	 * @author jk271
	 */
	void book_new_vehicle(const sint64 price, const koord k, const waytype_t wt=ignore_wt);

	/**
	 * Adds income to accounting statistics.
	 * @param amount earned money
	 * @param wt transport type used in accounting statistics
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
	 * Adds way renewals to accounting statistics.
	 * @param amount (should be negative, will be adjusted for bits_per_month)
	 * @param wt type of transport for accounting
	 * @author jamespetts
	 */

	void book_way_renewal(const sint64 amount, const waytype_t wt = ignore_wt);

	/**
	 * Books toll paid by our company to someone else.
	 * @param amount money paid to our company
	 * @param wt type of transport used for accounting statistiscs
	 * @author jk271
	 */
	void book_toll_paid(const sint64 amount, const waytype_t wt=ignore_wt);

	/**
	 * Books toll paid to our company by someone else.
	 * @param amount money paid for usage of our roads,railway,channels, ... ; positive sign
	 * @param wt type of transport used for accounting statistiscs
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

   /**
     * Is player allowed to purchase something of this price, or is player
     * too deep in debt?  (This routine allows the public service player to
	 * always buy anything.)
     * @returns whether player is allowed to purchase something of cost "price"
     * @params price
     */
	bool can_afford(sint64 price) const ;
	/**
	 * Static version.  If player is NULL, player can afford anything.
	 */
	static bool can_afford(player_t* player, sint64 price);

	bool has_money_or_assets() const;

	finance_t * get_finance() { return finance; }

	/**
	 * Is this the public service player?
	 * This is a subroutine to allow the public service player to be redefined in the future
	 */
	bool is_public_service() const { return player_nr == 1; }

	virtual bool set_active( bool b ) { return active = b; }

	bool is_active() const { return active; }

	bool is_locked() const { return locked; }

	bool is_unlock_pending() const { return unlock_pending; }

	void unlock(bool unlock_, bool unlock_pending_=false) { if(check_solvency() != in_liquidation) locked = !unlock_; unlock_pending = unlock_pending_; }

	void check_unlock( const pwd_hash_t& hash ) { locked = (pwd_hash != hash) || check_solvency() == in_liquidation; }

	bool get_has_been_warned_about_no_money_for_renewals() const { return has_been_warned_about_no_money_for_renewals; }
	void set_has_been_warned_about_no_money_for_renewals(bool value) { has_been_warned_about_no_money_for_renewals = value; }

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
	void age_messages(uint32 delta_t);

	/* Handles player colors ...
	* @author prissi
	*/
	uint8 get_player_color1() const { return player_color_1; }
	uint8 get_player_color2() const { return player_color_2; }
	// Change and report message
	void set_player_color(uint8 col1, uint8 col2);
	// Change, do not report message
	// Used for setting default colors
	void set_player_color_no_message(uint8 col1, uint8 col2);

	/**
	 * @return the name of the player; "player -1" sits in front of the screen
	 * @author prissi
	 */
	const char* get_name() const;
	void set_name(const char *);

	sint8 get_player_nr() const {return player_nr; }

	/**
	 * return true, if the owner is none, myself or player(1), i.e. the ownership _can be taken by player test
	 * @author prissi
	 */
	static bool check_owner( const player_t *owner, const player_t *test );

	/**
	 * @param welt World this players belong to.
	 * @param player_nr Number assigned to this player - it's an ID.
	 * @author Hj. Malthaner
	 */
	player_t(karte_t *welt, uint8 player_nr );

	virtual ~player_t();

	/**
	 * This is safe to be called with player==NULL
	 */
	static sint64 add_maintenance(player_t *player, sint64 const change, waytype_t const wt=ignore_wt)
	{
		if(player) {
			return player->add_maintenance(change, wt);
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
	 * @return Account balance as a double (floating point) value
	 * @author Hj. Malthaner
	 */
	double get_account_balance_as_double() const;

	sint64 get_account_balance() const;

	/**
	 * @return true when account balance is overdrawn
	 * @author Hj. Malthaner
	 */
	int get_account_overdrawn() const;

	/**
	 * Displays messages from the queue of the player on the screen
	 * Show income messages
	 * @author prissi
	 */
	void display_messages();

	/**
	 * Called often by simworld.cc during simulation
	 * @note Any action goes here (only need for AI at the moment)
	 * @author Hj. Malthaner
	 */
	virtual void step();

	/**
	 * Called monthly by simworld.cc during simulation
	 * @author Hj. Malthaner
	 * @returns false if player has to be removed (inactive)
	 */
	virtual bool new_month();

	/**
	 * Called yearly by simworld.cc during simulation
	 * @author Hj. Malthaner
	 */
	virtual void new_year() {}

	/**
	 * Stores/loads the player state
	 * @param file where the data will be saved/loaded
	 * @author Hj. Malthaner
	 */
	virtual void rdwr(loadsave_t *file);

	/*
	 * Called after game is fully loaded;
	 */
	virtual void load_finished();

	virtual void rotate90( const sint16 y_size );

	/**
	* Calculates the assets of the player
	*/
	void calc_assets();

	/**
	* Updates the assets value of the player
	*/
	void update_assets(sint64 const delta, const waytype_t wt = ignore_wt);

	/**
	 * Report the player one of his vehicles has a problem
	 * @author Hansjörg Malthaner
	 * @date 26-Nov-2001
	 */
	virtual void report_vehicle_problem(convoihandle_t cnv,const koord3d ziel);

	/**
	 * Tells the player the result of tool-work commands
	 * If player is active then play sound, popup error msg etc
	 * AI players react upon this call and proceedplayer The owner of the city
	 * local is true if tool was called by player on our client
	 * @author Dwachs
	 */
	virtual void tell_tool_result(tool_t *tool, koord3d pos, const char *err, bool local);

	/**
	 * Tells the player that the factory
	 * is going to be deleted (flag==0)
	 * Bernd Gabriel, Dwachs
	 */
	enum notification_factory_t {
		notify_delete	// notified immediately before object is deleted (and before nulled in the slist_tpl<>)!
	};
	virtual void notify_factory(notification_factory_t, const fabrik_t*) {}

	solvency_status check_solvency() const;

private:
	/* undo informations *
	 * @author prissi
	 */
	vector_tpl<koord3d> last_built;
	waytype_t undo_type;

public:
	/**
	 * Function for UNDO
	 * @date 7-Feb-2005
	 * @author prissi
	 */
	void init_undo(waytype_t t, unsigned short max );
	/**
	 * Function for UNDO
	 * @date 7-Feb-2005
	 * @author prissi
	 */
	void add_undo(koord3d k);
	/**
	 * Function for UNDO
	 * @date 7-Feb-2005
	 * @author prissi
	 */
	sint64 undo();

private:
	// headquarters stuff
	sint32 headquarter_level;
	koord headquarter_pos;

	// The signalbox last selected. Used for placing signals attached to this box.
	// Local only: this datum is transmitted over the network when the tool is used.
	// Do not load/save.
	signalbox_t* selected_signalbox;

public:
	void add_headquarter(short hq_level, koord hq_pos)
	{
		headquarter_level = hq_level;
		headquarter_pos = hq_pos;
	}
	koord get_headquarter_pos() const { return headquarter_pos; }
	short get_headquarters_level() const { return headquarter_level; }

	void begin_liquidation();
	void complete_liquidation();

	bool allows_access_to(uint8 other_player_nr) const { return player_nr == other_player_nr || access[other_player_nr]; }
	void set_allow_access_to(uint8 other_player_nr, bool allow) { access[other_player_nr] = allow; }

	bool get_allow_voluntary_takeover() const { return allow_voluntary_takeover; }
	void set_allow_voluntary_takeover(bool value) { allow_voluntary_takeover = value; }

	bool available_for_takeover() const { return allow_voluntary_takeover || check_solvency() != solvent; }

	void set_selected_signalbox(signalbox_t* sb);
	const signalbox_t* get_selected_signalbox() const { return selected_signalbox; }
	signalbox_t* get_selected_signalbox() { return selected_signalbox; }

	const char* can_take_over(player_t* target_player);
	void take_over(player_t* target_player);
	/// Returns *negative* number constituting the cost
	sint64 calc_takeover_cost() const;
};

#endif
