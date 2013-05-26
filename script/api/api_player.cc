#include "api.h"

/** @file api_player.cc exports player/company related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../player/simplay.h"
#include "../../player/finance.h"


using namespace script_api;

vector_tpl<sint64> const& get_player_stat(spieler_t *sp, sint32 INDEX, sint32 TTYPE, bool monthly)
{
	static vector_tpl<sint64> v;
	v.clear();
	bool atv = false;
	if (TTYPE<0  ||  TTYPE>=TT_MAX) {
		if (INDEX<0  ||  INDEX>ATC_MAX) {
			return v;
		}
	}
	else {
		if (INDEX<0  ||  INDEX>ATV_MAX) {
			atv = true;
			return v;
		}
	}
	if (sp) {
		finance_t *finance = sp->get_finance();
		uint16 maxi = monthly ? MAX_PLAYER_HISTORY_MONTHS : MAX_PLAYER_HISTORY_YEARS;
		for(uint16 i = 0; i < maxi; i++) {
			sint64 m = atv ? ( monthly ? finance->get_history_veh_month((transport_type)TTYPE, i, INDEX) : finance->get_history_veh_year((transport_type)TTYPE, i, INDEX) )
			               : ( monthly ? finance->get_history_com_month(i, INDEX) : finance->get_history_com_year(i, INDEX) );
			if (atv) {
				if (INDEX != ATV_TRANSPORTED_PASSENGER  &&  INDEX != ATV_TRANSPORTED_MAIL  &&  INDEX != ATV_TRANSPORTED_GOOD  &&  INDEX !=ATV_TRANSPORTED) {
					m = convert_money(m);
				}
			}
			else {
				if (INDEX != ATC_ALL_CONVOIS) {
					m = convert_money(m);
				}
			}
			v.append(m);
		}
	}
	return v;
}

void_t change_player_account(spieler_t *sp, sint64 delta)
{
	if (sp) {
		sp->get_finance()->book_account(delta);
	}
	return void_t();
}


bool player_active(spieler_t *sp)
{
	return sp != NULL;
}


void export_player(HSQUIRRELVM vm)
{
	/**
	 * Class to access player statistics.
	 * Here, a player refers to one transport company, not to an individual playing simutrans.
	 */
	begin_class(vm, "player_x", "extend_get");

	/**
	 * Constructor.
	 * @param nr player number, 0 = standard player, 1 = public player
	 * @typemask (integer)
	 */
	// actually defined simutrans/script/scenario_base.nut
	// register_function(..., "constructor", ...);

	/**
	 * Return headquarter level.
	 * @returns level, level is zero if no headquarter was built
	 */
	register_method(vm, &spieler_t::get_headquarter_level, "get_headquarter_level");
	/**
	 * Return headquarter position.
	 * @returns coordinate, (-1,-1) if no headquarter was built
	 */
	register_method(vm, &spieler_t::get_headquarter_pos,   "get_headquarter_pos");
	/**
	 * Return name of company.
	 * @returns name
	 */
	register_method(vm, &spieler_t::get_name,              "get_name");
	/**
	 * Get monthly statistics of construction costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_construction",      freevariable3<sint32,sint32,bool>(ATV_CONSTRUCTION_COST, TT_ALL, true), true);
	/**
	 * Get monthly statistics of vehicle running costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_vehicle_maint",     freevariable3<sint32,sint32,bool>(ATV_RUNNING_COST, TT_ALL, true), true);
	/**
	 * Get monthly statistics of costs for vehicle purchase.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_new_vehicles",      freevariable3<sint32,sint32,bool>(ATV_NEW_VEHICLE, TT_ALL, true), true);
	/**
	 * Get monthly statistics of income.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_income",            freevariable3<sint32,sint32,bool>(ATV_REVENUE_TRANSPORT, TT_ALL, true), true);
	/**
	 * Get monthly statistics of infrastructure maintenance.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_maintenance",       freevariable3<sint32,sint32,bool>(ATV_INFRASTRUCTURE_MAINTENANCE, TT_ALL, true), true);
	/**
	 * Get monthly statistics of assets.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_assets",            freevariable3<sint32,sint32,bool>(ATV_NON_FINANCIAL_ASSETS, TT_ALL, true), true);
	/**
	 * Get monthly statistics of cash.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_cash",              freevariable3<sint32,sint32,bool>(ATC_CASH, -1, true), true);
	/**
	 * Get monthly statistics of net wealth.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_net_wealth",        freevariable3<sint32,sint32,bool>(ATC_NETWEALTH, -1, true), true);
	/**
	 * Get monthly statistics of profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_profit",            freevariable3<sint32,sint32,bool>(ATV_PROFIT, TT_ALL, true), true);
	/**
	 * Get monthly statistics of operating profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_operating_profit",  freevariable3<sint32,sint32,bool>(ATV_OPERATING_PROFIT, TT_ALL, true), true);
	/**
	 * Get monthly statistics of margin.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_margin",            freevariable3<sint32,sint32,bool>(ATV_PROFIT_MARGIN, -1, true), true);
	/**
	 * Get monthly statistics of all transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported",       freevariable3<sint32,sint32,bool>(ATV_TRANSPORTED, TT_ALL, true), true);
	/**
	 * Get monthly statistics of income from powerlines.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_powerline",         freevariable3<sint32,sint32,bool>(ATV_REVENUE, TT_POWERLINE, true), true);
	/**
	 * Get monthly statistics of transported passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_pax",   freevariable3<sint32,sint32,bool>(ATV_TRANSPORTED_PASSENGER, TT_ALL, true), true);
	/**
	 * Get monthly statistics of transported mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_mail",  freevariable3<sint32,sint32,bool>(ATV_TRANSPORTED_MAIL, TT_ALL, true), true);
	/**
	 * Get monthly statistics of transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_goods", freevariable3<sint32,sint32,bool>(ATV_TRANSPORTED_GOOD, TT_ALL, true), true);
	/**
	 * Get monthly statistics of number of convoys.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_convoys",           freevariable3<sint32,sint32,bool>(ATC_ALL_CONVOIS, -1, true), true);
	/**
	 * Get monthly statistics of income/loss due to way tolls.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_way_tolls",         freevariable3<sint32,sint32,bool>(ATV_WAY_TOLL, TT_ALL, true), true);

	/**
	 * Change bank account of player by given amount @p delta.
	 * @param delta
	 * @warning cannot be used in network games.
	 */
	register_method(vm, &change_player_account, "book_cash", true);

	/**
	 * Returns whether the player (still) exists in the game.
	 */
	register_method(vm, &player_active, "is_active", true);

	end_class(vm);
}
