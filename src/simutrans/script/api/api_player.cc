/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_player.cc exports player/company related functions. */

#include "../api_class.h"
#include "../api_function.h"
#include "../../player/simplay.h"
#include "../../player/finance.h"

// for creation of lines
#include "../../simline.h"
#include "../../tool/simmenu.h"
#include "../../world/simworld.h"


using namespace script_api;

vector_tpl<sint64> const& get_player_stat(player_t *player, sint32 INDEX, sint32 TTYPE, bool monthly)
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
			return v;
		}
		atv = true;
	}
	if (player) {
		finance_t *finance = player->get_finance();
		// calculate current month
		finance->calc_finance_history();
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


// export of finance_t only here
namespace script_api {
	declare_specialized_param(finance_t*, param<player_t*>::typemask(), param<player_t*>::squirrel_type());

	finance_t* param<finance_t*>::get(HSQUIRRELVM vm, SQInteger index)
	{
		player_t *player = param<player_t*>::get(vm, index);
		return player ? player->get_finance() : NULL;
	}
};
// also export transport_type - to get correct parameters
namespace script_api {
	declare_specialized_param(transport_type, "i", "integer");

	transport_type param<transport_type>::get(HSQUIRRELVM vm, SQInteger index)
	{
		return (transport_type) min(param<uint16>::get(vm, index), TT_MAX-1);
	}
};


SQInteger player_export_line_list(HSQUIRRELVM vm)
{
	if (player_t* player = param<player_t*>::get(vm, 1)) {
		push_instance(vm, "line_list_x");
		set_slot(vm, "player_id", player->get_player_nr());
		return 1;
	}
	return SQ_ERROR;
}

call_tool_init player_create_line(player_t *player, waytype_t wt)
{
	simline_t::linetype lt = simline_t::waytype_to_linetype(wt);
	if (lt == simline_t::MAX_LINE_TYPE) {
		return "Invalid waytype provided";
	}
	// build param string (see schedule_list_gui_t::action_triggered)
	cbuffer_t buf;
	buf.printf( "c,0,%i,0,0|%i|", lt, lt);
	return call_tool_init(TOOL_CHANGE_LINE | SIMPLE_TOOL, buf, 0, player);
}

call_tool_init player_book_account(player_t *player, sint32 delta)
{
	// build param string (see tool_change_player_t)
	cbuffer_t buf;
	buf.printf( "$,%i,%i", player->get_player_nr(), delta);
	return call_tool_init(TOOL_CHANGE_PLAYER | SIMPLE_TOOL, buf, 0, welt->get_public_player());
}

SQInteger player_get_my_player(HSQUIRRELVM vm)
{
	return script_api::param<player_t*>::push(vm, get_my_player(vm) );
}

call_tool_init player_set_name(player_t *player, const char* name)
{
	return script_api::command_rename(player, 'p', player->get_player_nr(), name);
}

void export_player(HSQUIRRELVM vm, bool scenario)
{
	/**
	 * Class to access player statistics.
	 * Here, a player refers to one transport company, not to an individual playing simutrans.
	 */
	begin_class(vm, "player_x", "extend_get,ingame_object");

	/**
	 * Constructor.
	 * @param nr player number, 0 = standard player, 1 = public player
	 * @typemask (integer)
	 */
	// actually defined in simutrans/script/script_base.nut
	// register_function(..., "constructor", ...);

	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<player_t*>(vm); //register_function("is_valid")

	if (!scenario) {
		/**
		 * @returns player associated with the AI.
		 * @ingroup ai_only
		 * @typemask player_x()
		 * @note Only available in AI mode.
		 */
		STATIC register_function(vm, &player_get_my_player, "self", 0, "", true);
	}
	/**
	 * Return headquarters level.
	 * @returns level, level is zero if no headquarters was built
	 */
	register_method(vm, &player_t::get_headquarter_level, "get_headquarter_level");
	/**
	 * Return headquarters position.
	 * @returns coordinate, (-1,-1) if no headquarters was built
	 */
	register_method(vm, &player_t::get_headquarter_pos,   "get_headquarter_pos");
	/**
	 * Return name of company.
	 * @returns name
	 */
	register_method(vm, &player_t::get_name,              "get_name");
	/**
	 * Sets name of company.
	 * @param name the new name
	 * @ingroup rename_func
	 */
	register_method(vm, &player_set_name, "set_name", true);
	/**
	 * Get monthly statistics of construction costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_construction",      freevariable<sint32,sint32,bool>(ATV_CONSTRUCTION_COST, TT_ALL, true), true);
	/**
	 * Get monthly statistics of vehicle running costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_vehicle_maint",     freevariable<sint32,sint32,bool>(ATV_RUNNING_COST, TT_ALL, true), true);
	/**
	 * Get monthly statistics of costs for vehicle purchase.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_new_vehicles",      freevariable<sint32,sint32,bool>(ATV_NEW_VEHICLE, TT_ALL, true), true);
	/**
	 * Get monthly statistics of income.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_income",            freevariable<sint32,sint32,bool>(ATV_REVENUE_TRANSPORT, TT_ALL, true), true);
	/**
	 * Get monthly statistics of infrastructure maintenance.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_maintenance",       freevariable<sint32,sint32,bool>(ATV_INFRASTRUCTURE_MAINTENANCE, TT_ALL, true), true);
	/**
	 * Get monthly statistics of assets.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_assets",            freevariable<sint32,sint32,bool>(ATV_NON_FINANCIAL_ASSETS, TT_ALL, true), true);
	/**
	 * Get monthly statistics of cash.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_cash",              freevariable<sint32,sint32,bool>(ATC_CASH, -1, true), true);
	/**
	 * Get monthly statistics of net wealth.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_net_wealth",        freevariable<sint32,sint32,bool>(ATC_NETWEALTH, -1, true), true);
	/**
	 * Get monthly statistics of profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_profit",            freevariable<sint32,sint32,bool>(ATV_PROFIT, TT_ALL, true), true);
	/**
	 * Get monthly statistics of operating profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_operating_profit",  freevariable<sint32,sint32,bool>(ATV_OPERATING_PROFIT, TT_ALL, true), true);
	/**
	 * Get monthly statistics of margin.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_margin",            freevariable<sint32,sint32,bool>(ATV_PROFIT_MARGIN, -1, true), true);
	/**
	 * Get monthly statistics of all transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported",       freevariable<sint32,sint32,bool>(ATV_TRANSPORTED, TT_ALL, true), true);
	/**
	 * Get monthly statistics of income from powerlines.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_powerline",         freevariable<sint32,sint32,bool>(ATV_REVENUE, TT_POWERLINE, true), true);
	/**
	 * Get monthly statistics of transported passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_pax",   freevariable<sint32,sint32,bool>(ATV_TRANSPORTED_PASSENGER, TT_ALL, true), true);
	/**
	 * Get monthly statistics of transported mail.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_mail",  freevariable<sint32,sint32,bool>(ATV_TRANSPORTED_MAIL, TT_ALL, true), true);
	/**
	 * Get monthly statistics of transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_transported_goods", freevariable<sint32,sint32,bool>(ATV_TRANSPORTED_GOOD, TT_ALL, true), true);
	/**
	 * Get monthly statistics of number of convoys.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_convoys",           freevariable<sint32,sint32,bool>(ATC_ALL_CONVOIS, -1, true), true);
	/**
	 * Get monthly statistics of income/loss due to way tolls.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_player_stat, "get_way_tolls",         freevariable<sint32,sint32,bool>(ATV_WAY_TOLL, TT_ALL, true), true);

	if (scenario) {
		/**
		 * Change bank account of player by given amount @p delta.
		 * @param delta
		 * @ingroup scen_only
		 */
		register_method(vm, player_book_account, "book_cash", true);
	}
	/**
	 * Returns the current account balance.
	 */
	register_method(vm, &player_t::get_account_balance_as_double, "get_current_cash");
	/**
	 * Returns the current net worth [in 1/100 cr].
	 */
	register_method(vm, &finance_t::get_netwealth, "get_current_net_wealth");
	/**
	 * Returns the current maintenance [in 1/100 cr].
	 */
	register_method_fv(vm, &finance_t::get_maintenance_with_bits, "get_current_maintenance", freevariable<uint8>(TT_ALL));
	/**
	 * Returns whether the player (still) exists in the game.
	 *
	 * @deprecated Only available for api versions less than 120.1, see @ref get_api_version.
	 * @typemask bool()
	 */
	// register_method(vm,, "is_active", true); implemented in scenario_compat.nut
	/**
	 * Exports list of lines of this player.
	 * @typemask line_list_x()
	 */
	register_function(vm, &player_export_line_list, "get_line_list", 1, param<player_t*>::typemask());

	/**
	 * Creates a new line for the player of the given way type.
	 * @param wt way type
	 * @ingroup game_cmd
	 */
	register_method(vm, &player_create_line, "create_line", true);
	/**
	 * Returns player type: 1 = human, 2,3 = old c++ AI, 4 = scripted AI
	 */
	register_method(vm, &player_t::get_ai_id, "get_type");

	end_class(vm);
}
