/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef PLAYER_SIMPLAY_H
#define PLAYER_SIMPLAY_H


#include "../utils/sha1_hash.h"
#include "../simtypes.h"
#include "../simlinemgmt.h"

#include "../halthandle_t.h"
#include "../convoihandle_t.h"

#include "../dataobj/koord.h"

#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"


class karte_ptr_t;
class fabrik_t;
class koord3d;
class tool_t;
class finance_t;


#define HUMAN_PLAYER_NR  (0)
#define PUBLIC_PLAYER_NR (1)


/**
 * Class to hold information about one player/company. AI players are derived from this class.
 */
class player_t
{
public:
	enum {
		EMPTY        = 0,
		HUMAN        = 1,
		AI_GOODS     = 2,
		AI_PASSENGER = 3,
		AI_SCRIPTED  = 4,
		MAX_AI,
		PASSWORD_PROTECTED = 128
	};

protected:
	char player_name_buf[256];

	/* "new" finance history */
	finance_t *finance;

	/**
	 * Die Welt in der gespielt wird.
	 */
	static karte_ptr_t welt;

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

	slist_tpl<income_message_t *> messages;

	/**
	 * Creates new income message entry or merges with existing one if the
	 * most recent one is at the same coordinate
	 */
	void add_message(sint64 amount, koord k);

	/**
	 * Displays amount of money when koordinates are on screen
	 */
	void add_money_message(sint64 amount, koord k);

	/**
	 * Colors of the player
	 */
	uint8 player_color_1, player_color_2;

	/**
	 * Player number; only player 0 can do interaction
	 */
	uint8 player_nr;

	/**
	 * Adds some amount to the maintenance costs.
	 * @param change the change
	 * @return the new maintenance costs
	 */
	sint64 add_maintenance(sint64 change, waytype_t const wt=ignore_wt);

	/**
	 * Is this player an AI player?
	 */
	bool active;

	/**
	 * Are this player allowed to do any changes?
	 */
	bool locked;

	bool unlock_pending;

	// contains the password hash for local games
	pwd_hash_t pwd_hash;

public:
	/**
	 * Sums up "count" with number of convois in statistics,
	 * supersedes buche( count, COST_ALL_CONVOIS).
	 */
	void book_convoi_number(int count);

	/**
	 * Adds construction costs to accounting statistics.
	 * @param amount How much does it cost
	 * @param wt type of transport
	 */
	static void book_construction_costs(player_t * const player, const sint64 amount, const koord k, const waytype_t wt=ignore_wt);

	/**
	 * Accounts bought/sold vehicles.
	 * @param price money used for purchase of vehicle,
	 *              negative value = vehicle bought,
	 *              positive value = vehicle sold
	 * @param wt type of transport for accounting purpose
	 */
	void book_new_vehicle(const sint64 price, const koord k, const waytype_t wt=ignore_wt);

	/**
	 * Adds income to accounting statistics.
	 * @param amount earned money
	 * @param wt transport type used in accounting statistics
	 * @param cathegory parameter
	 *  0 ... passenger
	 *  1 ... mail
	 *  2 ... good (and powerlines revenue)
	 */
	void book_revenue(const sint64 amount, const koord k, const waytype_t wt=ignore_wt, sint32 cathegory=2);

	/**
	 * Adds running costs to accounting statistics.
	 * @param amount How much does it cost
	 * @param wt type of transport used for accounting statistics
	 */
	void book_running_costs(const sint64 amount, const waytype_t wt=ignore_wt);

	/**
	 * Books toll paid by our company to someone else.
	 * @param amount money paid to our company
	 * @param wt type of transport used for accounting statistics
	 */
	void book_toll_paid(const sint64 amount, const waytype_t wt=ignore_wt);

	/**
	 * Books toll paid to our company by someone else.
	 * @param amount money paid for usage of our roads,railway,channels, ... ; positive sign
	 * @param wt type of transport used for accounting statistics
	 */
	void book_toll_received(const sint64 amount, waytype_t wt=ignore_wt);

	/**
	 * Add amount of transported passenger, mail, goods to accounting statistics.
	 * @param amount sum of money
	 * @param wt way type
	 * @param index 0 = passenger, 1 = mail, 2 = goods
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

	/**
	* Test if the player has sufficient funds for an action.
	* Returns true if the player has the funds or does not need to.
	* @param cost the amount of funds that want to be added to the balance
	*/
	bool can_afford(sint64 cost) const;

	finance_t * get_finance() const { return finance; }

	virtual bool set_active( bool b ) { return active = b; }

	bool is_active() const { return active; }

	bool is_locked() const { return locked; }

	bool is_unlock_pending() const { return unlock_pending; }

	void unlock(bool unlock_, bool unlock_pending_=false) { locked = !unlock_; unlock_pending = unlock_pending_; }

	void check_unlock( const pwd_hash_t& hash ) { locked = (pwd_hash != hash); }

	// some routine needs this for direct manipulation
	pwd_hash_t& access_password_hash() { return pwd_hash; }

	// this type of AIs identifier
	virtual uint8 get_ai_id() const { return HUMAN; }

	simlinemgmt_t simlinemgmt;

	/// Age messages (move them upwards)
	void age_messages(uint32 delta_t);

	/// Handles player colors
	uint8 get_player_color1() const { return player_color_1; }
	uint8 get_player_color2() const { return player_color_2; }
	void set_player_color(uint8 col1, uint8 col2);

	/**
	 * @return the name of the player; "player -1" sits in front of the screen
	 */
	const char* get_name() const;
	void set_name(const char *);

	sint8 get_player_nr() const {return player_nr; }

	/**
	 * Test if this player is a public service player.
	 * @return true if the player is a public service player, otherwise false.
	 */
	bool is_public_service() const;

	/**
	 * return true, if the owner is none, myself or player(1), i.e. the ownership can be taken by player test
	 */
	static bool check_owner( const player_t *owner, const player_t *test );

	/**
	 * @param player_nr Number assigned to this player - it's an ID.
	 */
	player_t(uint8 player_nr );

	virtual ~player_t();

	static inline sint64 add_maintenance(player_t *player, sint64 const change, waytype_t const wt=ignore_wt)
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
	 */
	double get_account_balance_as_double() const;

	/**
	 * @return true when account balance is overdrawn
	 */
	int get_account_overdrawn() const;

	/**
	 * Displays messages from the queue of the player on the screen
	 * Show income messages
	 */
	void display_messages();

	/**
	 * Called often by simworld.cc during simulation
	 * @note Any action goes here (only need for AI at the moment)
	 */
	virtual void step();

	/**
	 * Called monthly by simworld.cc during simulation
	 * @returns false if player has to be removed (bankrupt/inactive)
	 */
	virtual bool new_month();

	/**
	 * Called yearly by simworld.cc during simulation
	 */
	virtual void new_year() {}

	/**
	 * Stores/loads the player state
	 * @param file where the data will be saved/loaded
	 */
	virtual void rdwr(loadsave_t *file);

	/*
	 * Called after game is fully loaded;
	 */
	virtual void finish_rd();

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
	 */
	virtual void report_vehicle_problem(convoihandle_t cnv,const koord3d position);

	/**
	 * Tells the player the result of tool-work commands.
	 * If player is active then play sound, popup error msg etc.
	 */
	void tell_tool_result(tool_t *tool, koord3d pos, const char *err);

	/**
	 * Tells the player that the factory
	 * is going to be deleted (flag==0)
	 */
	enum notification_factory_t {
		notify_delete // notified immediately before object is deleted (and before nulled in the slist_tpl<>)!
	};
	virtual void notify_factory(notification_factory_t, const fabrik_t*) {}

private:
	/// undo information
	vector_tpl<koord3d> last_built;
	waytype_t undo_type;

public:
	/**
	 * Function for UNDO
	 */
	void init_undo(waytype_t t, unsigned short max );

	/**
	 * Function for UNDO
	 */
	void add_undo(koord3d k);

	/**
	 * Function for UNDO
	 */
	sint64 undo();

private:
	// headquarters stuff
	sint32 headquarter_level;
	koord headquarter_pos;

public:
	void add_headquarter(short hq_level, koord hq_pos)
	{
		headquarter_level = hq_level;
		headquarter_pos = hq_pos;
	}
	koord get_headquarter_pos() const { return headquarter_pos; }
	short get_headquarter_level() const { return headquarter_level; }

	void ai_bankrupt();
};

#endif
