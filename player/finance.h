/*
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef finance_h
#define finance_h

#include <assert.h>

#include "../simtypes.h"

/// for compatibility with old versions
#define OLD_MAX_PLAYER_COST (19)

/// number of years to keep history
#define MAX_PLAYER_HISTORY_YEARS  (25)

/// number of months to keep history
#define MAX_PLAYER_HISTORY_MONTHS (25)


/**
 * Type of transport used in accounting statistics.
 * waytype_t was not used because of values assigned to air_wt and powerline_wt.
 * There are also buildings like railway station that can be distinguished
 * by transport_type and can not be distinguished by waytype_t.
 * @author jk271
 */
enum transport_type {
	TT_ALL=0,
	TT_ROAD,
	TT_RAILWAY,
	TT_SHIP,
	TT_MONORAIL,
	TT_MAGLEV,
	TT_TRAM,
	TT_NARROWGAUGE,
	TT_AIR,
	TT_OTHER,  ///< everything else that can not be differentiated (e.g. houses), not powerlines
	TT_MAX_VEH = TT_OTHER,
	TT_POWERLINE,
	TT_MAX
};


/**
 * ATC = accounting type commmon (common means data common for all transport types).
 *
 * Supersedes COST_ types, that CAN NOT be distinguished by type of transport-
 * - the data are concerning to whole company
 */
enum accounting_type_common {
	ATC_CASH = 0,		///< Cash
	ATC_NETWEALTH,		///< Total Cash + Assets
	ATC_ALL_CONVOIS,        ///< Convoy count
	ATC_SCENARIO_COMPLETED, ///< Scenario success (only useful if there is one ... )
	ATC_MAX
};


/**
 * ATV = accounting type vehicles.
 * Supersedes COST_ types, that CAN be distinguished by type of transport.
 * @author jk271
 */
enum accounting_type_vehicles {
	ATV_REVENUE_PASSENGER=0, ///< Revenue from passenger transport
	ATV_REVENUE_MAIL,        ///< Revenue from mail transport
	ATV_REVENUE_GOOD,        ///< Revenue from good transport
	ATV_REVENUE_TRANSPORT,	 ///< Operating profit = passenger + mail + goods = was: COST_INCOME
	ATV_TOLL_RECEIVED,	 ///< Toll paid to you by another player
	ATV_REVENUE,             ///< Operating profit = revenue_transport + toll = passenger + mail+ goods + toll_received

	ATV_RUNNING_COST,               ///< Distance based running costs, was: COST_VEHICLE_RUN
	ATV_VEHICLE_MAINTENANCE,        ///< Monthly vehicle maintenance. Unused.
	ATV_INFRASTRUCTURE_MAINTENANCE, ///< Infrastructure maintenance (roads, railway, ...), was: COST_MAINTENANCE
	ATV_TOLL_PAID,			  ///< Toll paid by you to another player
	ATV_EXPENDITURE,		        ///< Total expenditure = RUNNING_COSTS+VEHICLE_MAINTENANCE+INFRACTRUCTURE_MAINTENANCE+TOLL_PAID
	ATV_OPERATING_PROFIT,		  ///< ATV_REVENUE - ATV_EXPENDITURE, was: COST_OPERATING_PROFIT
	ATV_NEW_VEHICLE,			  ///< New vehicles
	ATV_CONSTRUCTION_COST,		  ///< Construction cost, COST_CONSTRUCTION mapped here
	ATV_PROFIT,			        ///< ATV_OPERATING_PROFIT - (CONSTRUCTION_COST + NEW_VEHICLE), was: COST_PROFIT
	ATV_WAY_TOLL,			  ///< = ATV_TOLL_PAID + ATV_TOLL_RECEIVED, was: COST_WAY_TOLLS
	ATV_NON_FINANCIAL_ASSETS,	  ///< Value of vehicles owned by your company, was: COST_ASSETS
	ATV_PROFIT_MARGIN,		  ///< ATV_OPERATING_PROFIT / ATV_REVENUE, was: COST_MARGIN

	ATV_TRANSPORTED_PASSENGER, ///< Number of transported passengers
	ATV_TRANSPORTED_MAIL,      ///< Number of transported mail
	ATV_TRANSPORTED_GOOD,      ///< Number of transported goods
	ATV_TRANSPORTED,           ///< Total number of transported cargo, was COST_ALL_TRANSPORTED

	ATV_DELIVERED_PASSENGER,   ///< Number of delivered passengers, was: COST_TRANSPORTED_PAS
	ATV_DELIVERED_MAIL,        ///< Number of delivered mail, was: COST_TRANSPORTED_MAIL
	ATV_DELIVERED_GOOD,        ///< Number of delivered goods, was: COST_TRANSPORTED_GOOD
	ATV_DELIVERED,             ///< Total number of delivered cargo

	ATV_MAX
};


class loadsave_t;
class karte_t;
class spieler_t;
class scenario_t;


/**
 * Encapsulate margin calculation  (Operating_Profit / Income)
 * @author Ben Love
 */
inline sint64 calc_margin(sint64 operating_profit, sint64 proceeds)
{
	if (proceeds == 0) {
		return 0;
	}
	return (10000 * operating_profit) / proceeds;
}


/**
 * convert to displayed value
 */
inline sint64 convert_money(sint64 value) { return (value + 50) / 100; }


/**
 * Class to encapsulate all company related statistics.
 */
class finance_t {
	/** transport company */
	spieler_t * player;

	karte_t * world;

	/**
 	 * Amount of money, previously known as "konto"
 	 * @author Hj. Malthaner
 	 */
	sint64 account_balance;

	/**
	 * Shows how many months you have been in red numbers.
	 * @author Hj. Malthaner
	 */
	sint32 account_overdrawn;

	/**
	 * Remember the starting money, used e.g. in scenarios.
	 */
	sint64 starting_money;

	/**
	 * Contains values having relation with whole company but not with particular
	 * type of transport (com - common).
 	 * @author jk271
 	 */
	sint64 com_year[MAX_PLAYER_HISTORY_YEARS][ATC_MAX];

	/**
	 * Monthly finance history, data not distinguishable by transport type.
	 */
	sint64 com_month[MAX_PLAYER_HISTORY_MONTHS][ATC_MAX];

	/**
	 * Finance history having relation with particular type of service
 	 * @author jk271
 	 */
	sint64 veh_year[TT_MAX][MAX_PLAYER_HISTORY_YEARS][ATV_MAX];
	sint64 veh_month[TT_MAX][MAX_PLAYER_HISTORY_MONTHS][ATV_MAX];

	/**
 	 * Monthly maintenance cost
 	 * @author Hj. Malthaner
 	 */
	sint32 maintenance[TT_MAX];

	/**
	 * Monthly vehicle maintenance cost per transport type.
 	 * @author jk271
 	 */
	sint32 vehicle_maintenance[TT_MAX];

public:
	finance_t(spieler_t * _player, karte_t * _world);

	/**
	 * Adds construction cost to finance stats.
	 * @param amount sum of money
	 * @param wt way type, e.g. tram_wt
	 * @param utyp used for distinguishing tranport type of building for accounting purposes, used with buildings only.
	 */
	inline void book_construction_costs(const sint64 amount, const waytype_t wt) {
		transport_type tt = translate_waytype_to_tt(wt);
		veh_year[tt][0][ATV_CONSTRUCTION_COST] += (sint64) amount;
		veh_month[tt][0][ATV_CONSTRUCTION_COST] += (sint64) amount;

		account_balance += amount;
	}

	/**
	 * Adds count to number of convois in statistics.
	 * @author jk271
	 */
	inline void book_convoi_number( const int count ) {
		com_year[0][ATC_ALL_CONVOIS] += count;
		com_month[0][ATC_ALL_CONVOIS] += count;
	}

	/**
	 * Adds maintenance into/from finance stats.
	 * @param change monthly maintenance cost difference
	 * @param wt - waytype for accounting purposes
	 * @param utyp - used for distinguishing of transport type of buildings. Used with buildings only.
	 */
	inline sint32 book_maintenance(sint32 change, waytype_t const wt)
	{
		transport_type tt = translate_waytype_to_tt(wt);
		maintenance[tt] += change;
		maintenance[TT_ALL] += change;
		return maintenance[tt];
	}

	/**
	 * Account purchase of new vehicle: Subtracts money, increases assets.
	 * @param amount money paid for vehicle
	 * @param wt - waytype of vehicle
	 */
	inline void book_new_vehicle(const sint64 amount, const waytype_t wt)
	{
		const transport_type tt = translate_waytype_to_tt(wt);

		veh_year[ tt][0][ATV_NEW_VEHICLE] += (sint64) amount;
		veh_month[tt][0][ATV_NEW_VEHICLE] += (sint64) amount;
		veh_year[ tt][0][ATV_NON_FINANCIAL_ASSETS] -= (sint64) amount;
		veh_month[tt][0][ATV_NON_FINANCIAL_ASSETS] -= (sint64) amount;

		account_balance += amount;
	}

	/**
	 * Accounts income from transport of passenger, mail, or, goods.
	 * @param amount earned money
	 * @param wt waytype of vehicle
	 * @param index 0 = passenger, 1 = mail, 2 = goods
	 */
	inline void book_revenue(const sint64 amount, const waytype_t wt, sint32 index)
	{
		const transport_type tt = translate_waytype_to_tt(wt);

		index = ((0 <= index) && (index <= 2)? index : 2);

		veh_year[tt][0][ATV_REVENUE_PASSENGER+index] += (sint64) amount;
		veh_month[tt][0][ATV_REVENUE_PASSENGER+index] += (sint64) amount;

		account_balance += amount;
	}

	/**
	 * Accounts distance-based running costs
	 * @param amount sum of money
	 * @param wt way type
	 */
	inline void book_running_costs(const sint64 amount, const waytype_t wt)
	{
		const transport_type tt = translate_waytype_to_tt(wt);
		veh_year[tt][0][ATV_RUNNING_COST] += amount;
		veh_month[tt][0][ATV_RUNNING_COST] += amount;
		account_balance += amount;
	}

	/**
	 * Account toll we have paid to any other company.
	 * @param amount sum of money
	 * @param wt way type
	 */
	inline void book_toll_paid(const sint64 amount, const waytype_t wt)
	{
		const transport_type tt =  translate_waytype_to_tt(wt);
		veh_year[tt][0][ATV_TOLL_PAID] += (sint64) amount;
		veh_month[tt][0][ATV_TOLL_PAID] += (sint64) amount;
		account_balance += amount;
	}

	/**
	 * Account toll we have received from another company.
	 * @param amount sum of money
	 * @param wt way type
	 */
	inline void book_toll_received(const sint64 amount, const waytype_t wt)
	{
		const transport_type tt = translate_waytype_to_tt(wt);
		veh_year[tt][0][ATV_TOLL_RECEIVED] += (sint64) amount;
		veh_month[tt][0][ATV_TOLL_RECEIVED] += (sint64) amount;
		account_balance += amount;
	}

	/**
	 * Makes stats of amount of transported passenger, mail and goods
	 * @param amount sum of money
	 * @param wt way type
	 * @param index 0 = passenger, 1 = mail, 2 = goods
	 */
	inline void book_transported(const sint64 amount, const waytype_t wt, int index)
	{
		const transport_type tt = translate_waytype_to_tt(wt);

		// there are: passenger, mail, goods
		if( (index < 0) || (index > 2)){
			index = 2;
		}

		veh_year[ tt][0][ATV_TRANSPORTED_PASSENGER+index] += amount;
		veh_month[tt][0][ATV_TRANSPORTED_PASSENGER+index] += amount;
	}


	/**
	 * Makes stats of amount of delivered passenger, mail and goods
	 * @param amount sum of money
	 * @param wt way type
	 * @param index 0 = passenger, 1 = mail, 2 = goods
	 */
	inline void book_delivered(const sint64 amount, const waytype_t wt, int index)
	{
		const transport_type tt = translate_waytype_to_tt(wt);

		// there are: passenger, mail, goods
		if( (index < 0) || (index > 2)){
			index = 2;
		}

		veh_year[ tt][0][ATV_DELIVERED_PASSENGER+index] += amount;
		veh_month[tt][0][ATV_DELIVERED_PASSENGER+index] += amount;
	}

	/**
	 * Calculates the finance history for player
	 * @author hsiegeln
	 */
	void calc_finance_history();

	/**
	 * @returns amount of money on account (also known as konto)
	 */
	inline sint64 get_account_balance() { return account_balance; }

	/**
	 * Books amount of money to account (also known as konto)
	 */
	void book_account(sint64 amount)
	{
		account_balance += amount;
		com_month[0][ATC_CASH] = account_balance;
		com_year [0][ATC_CASH] = account_balance;
		com_month[0][ATC_NETWEALTH] += amount;
		com_year [0][ATC_NETWEALTH] += amount;
		// BUG profit is not adjusted when calling this method
	}

	/**
	 * Returns the finance history (indistinguishable part) for player.
	 * @param year 0 .. current year, 1 .. last year, etc
	 * @param type one of accounting_type_common
	 * @author jk271
	 */
	sint64 get_history_com_year(int year, int type) const { return com_year[year][type]; }
	sint64 get_history_com_month(int month, int type) const { return com_month[month][type]; }

	/**
	 * Returns the finance history (distinguishable by type of transport) for player.
	 * @param tt one of transport_type
	 * @param year 0 .. current year, 1 .. last year, etc
	 * @param type one of accounting_type_vehicles
	 * @author jk271
	 */
	sint64 get_history_veh_year(transport_type tt, int year, int type) const { return veh_year[tt][year][type]; }
	sint64 get_history_veh_month(transport_type tt, int month, int type) const { return veh_month[tt][month][type]; }

	/**
	 * @return how much month we have been in red numbers (= we had negative account balance)
	 */
	sint32 get_account_overdrawn() const { return account_overdrawn; }

	/**
	 * @returns maintenance
	 * @param tt transport type (Truck, Ship Air, ...)
	 */
	sint32 get_maintenance(transport_type tt=TT_ALL) const { assert(tt<TT_MAX); return maintenance[tt]; }

	/**
	 * @returns maintenance scaled with bits_per_month
	 * @author jk271
	 */
	sint64 get_maintenance_with_bits(transport_type tt=TT_ALL) const;

	sint64 get_netwealth() const { return com_year[0][ATC_NETWEALTH]; }

	sint64 get_scenario_completed() const { return com_month[0][ATC_SCENARIO_COMPLETED]; }

	void set_scenario_completed(sint64 percent) { com_year[0][ATC_SCENARIO_COMPLETED] = com_month[0][ATC_SCENARIO_COMPLETED] = percent; }

	sint64 get_starting_money() const { return starting_money; }

	/**
	 * @returns vehicle maintenance scaled with bits_per_month
	 * @author jk271
	 */
	sint64 get_vehicle_maintenance_with_bits(transport_type tt=TT_ALL) const;

	/**
	 * @returns TRUE if there is at least one convoi, otherwise returns false
	 */
	bool has_convoi() const { return (com_year[0][ATC_ALL_CONVOIS] > 0); }

	/**
	 * returns TRUE if net wealth > 0 (but this of course requires that we keep netwealth up to date!)
	 */
	bool has_money_or_assets() const { return ((get_history_com_year(0, ATC_NETWEALTH) ) > 0 ); }

	/**
	 * increases number of month for which the company is in red numbers
	 */
	void increase_account_overdrawn() { account_overdrawn++; }

	/**
	 * returns true if company bancrupted
	 */
	bool is_bancrupted() const;

	/**
	 * Called at beginning of new month.
	 */
	void new_month();

	/**
	 * rolls the finance history for player (needed when neues_jahr() or neuer_monat()) triggered
	 * @author hsiegeln, jk271
	 */
	void roll_history_year();
	void roll_history_month();

	/**
	 * loads or saves finance statistic
	 */
	void rdwr(loadsave_t *file);

	/// loads statistics of old versions
	void rdwr_compatibility(loadsave_t *file);

	/**
	 * Sets account balance. This method enables to load old game format.
	 * Do NOT use it in any other places!
	 */
	void set_account_balance(sint64 amount) { account_balance = amount; }

	void set_assets(const sint64 (&assets)[TT_MAX]);

	/**
	 * Sets number of months for that the account balance is below zero. This method enables to load old game format.
	 * Do NOT use it in any other places for any other purpose!
	 */
	void set_account_overdrawn(sint32 num) { account_overdrawn = num; }

	void set_starting_money(sint64 amount) {  starting_money = amount; }

	/**
 	 * Translates haus_besch_t to transport_type
	 * Building can be assigned to transport type using utyp
 	 * @author jk271
 	 */
	static transport_type translate_utyp_to_tt(int utyp);

	/**
 	 * Translates waytype_t to transport_type
 	 * @author jk271
 	 */
	static transport_type translate_waytype_to_tt(waytype_t wt);

	void update_assets(sint64 delta, waytype_t wt);

private:
	/// helper method to translate old COST_ constants
	static int translate_index_cost_to_at(int cost_);

	/// helper method to translate old COST_ constants
	static int translate_index_cost_to_atc(int cost_index);

	/**
	 * Translates finance statistics from new format to old one.
	 * Used for saving data in old format
	 * @author jk271
	 */
	void export_to_cost_month(sint64 finance_history_month[][OLD_MAX_PLAYER_COST]);
	void export_to_cost_year( sint64 finance_history_year[][OLD_MAX_PLAYER_COST]);


	/**
	 * Translates finance statistics from old format to new one.
	 * Used for loading data from old format
	 * @author jk271
	 */
	void import_from_cost_month(const sint64 finance_history_month[][OLD_MAX_PLAYER_COST]);
	void import_from_cost_year( const sint64 finance_history_year[][OLD_MAX_PLAYER_COST]);
};

#endif
