#include "api.h"

/** @file api_player.cc exports player/company related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../player/simplay.h"


using namespace script_api;

vector_tpl<sint64> const& get_player_stat(spieler_t *sp, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (sp  &&  0<=INDEX  &&  INDEX<MAX_PLAYER_COST) {
		for(uint16 i = 0; i < MAX_PLAYER_HISTORY_MONTHS; i++) {
			v.append(sp->get_finance_history_month_converted(i, INDEX));
		}
	}
	return v;
}

void_t change_player_account(spieler_t *sp, sint64 delta)
{
	if (sp) {
		sp->buche(delta, COST_INCOME);
	}
	return void_t();
}


#define begin_class(c,p) push_class(vm, c);
#define end_class() sq_pop(vm,1);

void export_player(HSQUIRRELVM vm)
{
	/**
	 * Class to access player statistics.
	 * Here, a player refers to one transport company, not to an individual playing simutrans.
	 */
	begin_class("player_x", "extend_get");

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
	register_method_fv(vm, &get_player_stat, "get_construction",      freevariable<sint32>(COST_CONSTRUCTION), true);
	/**
	 * Get monthly statistics of vehicle running costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_vehicle_maint",     freevariable<sint32>(COST_VEHICLE_RUN), true);
	/**
	 * Get monthly statistics of costs for vehicle purchase.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_new_vehicles",      freevariable<sint32>(COST_NEW_VEHICLE), true);
	/**
	 * Get monthly statistics of income.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_income",            freevariable<sint32>(COST_INCOME), true);
	/**
	 * Get monthly statistics of infrastructure maintenance.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_maintenance",       freevariable<sint32>(COST_MAINTENANCE), true);
	/**
	 * Get monthly statistics of assets.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_assets",            freevariable<sint32>(COST_ASSETS), true);
	/**
	 * Get monthly statistics of cash.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_cash",              freevariable<sint32>(COST_CASH), true);
	/**
	 * Get monthly statistics of net wealth.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_net_wealth",        freevariable<sint32>(COST_NETWEALTH), true);
	/**
	 * Get monthly statistics of profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_profit",            freevariable<sint32>(COST_PROFIT), true);
	/**
	 * Get monthly statistics of operating profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_operating_profit",  freevariable<sint32>(COST_OPERATING_PROFIT), true);
	/**
	 * Get monthly statistics of margin.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_margin",            freevariable<sint32>(COST_MARGIN), true);
	/**
	 * Get monthly statistics of all transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported",       freevariable<sint32>(COST_ALL_TRANSPORTED), true);
	/**
	 * Get monthly statistics of income from powerlines.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_powerline",         freevariable<sint32>(COST_POWERLINES), true);
	/**
	 * Get monthly statistics of transported passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_pax",   freevariable<sint32>(COST_TRANSPORTED_PAS), true);
	/**
	 * Get monthly statistics of transported mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_mail",  freevariable<sint32>(COST_TRANSPORTED_MAIL), true);
	/**
	 * Get monthly statistics of transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_goods", freevariable<sint32>(COST_TRANSPORTED_GOOD), true);
	/**
	 * Get monthly statistics of number of convoys.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_convoys",           freevariable<sint32>(COST_ALL_CONVOIS), true);
	/**
	 * Get monthly statistics of income/loss due to way tolls.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_way_tolls",         freevariable<sint32>(COST_WAY_TOLLS), true);

	/**
	 * Change bank account of player by given amount @p delta.
	 * @param delta
	 * @warning cannot be used in network games.
	 */
	register_method(vm, &change_player_account, "book_cash", true);

	end_class();
}
