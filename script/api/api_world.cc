/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_world.cc exports world-map functions. */

#include "api_simple.h"
#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simworld.h"
#include "../../simversion.h"
#include "../../player/simplay.h"
#include "../../obj/gebaeude.h"
#include "../../descriptor/ground_desc.h"

using namespace script_api;


mytime_ticks_t world_get_time(karte_t*)
{
	return mytime_ticks_t(welt->get_current_month(), welt->get_ticks(), welt->scale_with_month_length(1<<18), welt->get_next_month_ticks());
}


vector_tpl<sint64> const& get_world_stat(karte_t* welt, bool monthly, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (0<=INDEX  &&  INDEX<karte_t::MAX_WORLD_COST) {
		uint16 maxi = monthly ? MAX_WORLD_HISTORY_MONTHS : MAX_WORLD_HISTORY_YEARS;
		for(uint16 i = 0; i < maxi; i++) {
			if (monthly) {
				v.append( welt->get_finance_history_month(i, INDEX) );
			}
			else {
				v.append( welt->get_finance_history_year(i, INDEX) );
			}
		}
	}
	return v;
}


bool world_remove_player(karte_t *welt, player_t *player)
{
	if (player == NULL) {
		return false;
	}
	// first test
	bool ok = welt->change_player_tool(karte_t::delete_player, player->get_player_nr(), 0, true /*unlocked*/, false /*exec*/);
	if (!ok) {
		return false;
	}
	// now call - will not have immediate effect in network games
	welt->call_change_player_tool(karte_t::delete_player, player->get_player_nr(), 0, true /*scripted*/);
	return true;
}


// returns index of attraction base tile with index > start
uint32 get_next_attraction_base(uint32 start)
{
	const weighted_vector_tpl<gebaeude_t*>& attractions = welt->get_attractions();
	for(uint32 i = start+1; i < attractions.get_count(); i++) {
		gebaeude_t *gb = attractions[i];
		if (gb != NULL  &&  gb->get_first_tile() == gb) {
			return i;
		}
	}
	return attractions.get_count();
}


SQInteger world_attraction_list_next(HSQUIRRELVM vm)
{
	return generic_get_next_f(vm, welt->get_attractions().get_count(), get_next_attraction_base);
}

namespace script_api {
	declare_fake_param(attraction_list_t, "attraction_list_x");
}

gebaeude_t* world_attraction_list_get(attraction_list_t, uint32 index)
{
	const weighted_vector_tpl<gebaeude_t*>& attractions = welt->get_attractions();
	return (index < attractions.get_count())  ?  attractions[index]  :  NULL;
}


SQInteger world_get_attraction_list(HSQUIRRELVM vm)
{
	return push_instance(vm, "attraction_list_x");
}

SQInteger world_get_attraction_count(HSQUIRRELVM vm)
{
	return param<uint32>::push(vm, welt->get_attractions().get_count());
}

SQInteger world_get_convoy_list(HSQUIRRELVM vm)
{
	push_instance(vm, "convoy_list_x");
	set_slot<bool>(vm, "use_world", true);
	return 1;
}

SQInteger world_get_size(HSQUIRRELVM vm)
{
	koord k = welt->get_size();
	if (coordinate_transform_t::get_rotation() & 1) {
		return push_instance(vm, "coord", k.y, k.x);
	}
	else {
		return push_instance(vm, "coord", k.x, k.y);
	}
}

const char* get_pakset_name()
{
	return ground_desc_t::outside->get_copyright();
}

const char* get_version_number()
{
	return VERSION_NUMBER;
}

void export_world(HSQUIRRELVM vm, bool scenario)
{
	/**
	 * Table with methods to access the world, the universe, and everything.
	 */
	begin_class(vm, "world", "extend_get");

	/**
	 * Checks whether given coordinate is valid.
	 * @param k coordinate
	 * @returns true if coordinate is valid
	 */
	STATIC register_method< bool(karte_t::*)(koord) const>(vm, &karte_t::is_within_limits,  "is_coord_valid");

	/**
	 * Searches city next to the given coordinate.
	 * @param k coordinate
	 * @returns city if there is any
	 */
	STATIC register_method(vm, &karte_t::find_nearest_city, "find_nearest_city");

	/**
	 * Current season.
	 * @returns season (0=winter, 1=spring, 2=summer, 3=autumn)
	 */
	STATIC register_method(vm, &karte_t::get_season, "get_season");

	if (scenario) {
		/**
		* Removes player company: removes all assets. Use with care.
		*
		* If pl is the first player (nr == 0) it is restarted immediately.
		* Public player (nr == 1) cannot be removed.
		*
		* In network games, there will be a delay between the call to this function and the removal of the player.
		*
		* @param pl player to be removed
		* @ingroup scen_only
		* @returns whether operation was successful
		*/
		STATIC register_method(vm, &world_remove_player, "remove_player", true);
	}
	/**
	 * Returns player number @p pl. If player does not exist, returns null.
	 * @param pl player number
	 */
	STATIC register_method(vm, &karte_t::get_player, "get_player", true);

	/**
	 * @returns current in-game time.
	 */
	STATIC register_local_method(vm, world_get_time, "get_time");
	/**
	 * Get monthly statistics of total number of citizens.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_citizens",          freevariable<bool,sint32>(true, karte_t::WORLD_CITIZENS), true );
	/**
	 * Get monthly statistics of total city growth.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_growth",            freevariable<bool,sint32>(true, karte_t::WORLD_GROWTH), true );
	/**
	 * Get monthly statistics of total number of towns.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_towns",             freevariable<bool,sint32>(true, karte_t::WORLD_TOWNS), true );
	/**
	 * Get monthly statistics of total number of factories.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_factories",         freevariable<bool,sint32>(true, karte_t::WORLD_FACTORIES), true );
	/**
	 * Get monthly statistics of total number of convoys.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_convoys",           freevariable<bool,sint32>(true, karte_t::WORLD_CONVOIS), true );
	/**
	 * Get monthly statistics of total number of citycars.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_citycars",          freevariable<bool,sint32>(true, karte_t::WORLD_CITYCARS), true );
	/**
	 * Get monthly statistics of ratio transported to generated passengers.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_ratio_pax",         freevariable<bool,sint32>(true, karte_t::WORLD_PAS_RATIO), true );
	/**
	 * Get monthly statistics of total number of generated passengers.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_pax city_x::get_transported_pax
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_generated_pax",     freevariable<bool,sint32>(true, karte_t::WORLD_PAS_GENERATED), true );
	/**
	 * Get monthly statistics of ratio transported to generated mail.
	 * @returns array, index [0] corresponds to current month
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_ratio_mail",        freevariable<bool,sint32>(true, karte_t::WORLD_MAIL_RATIO), true );
	/**
	 * Get monthly statistics of total number of generated mail.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_generated_mail",    freevariable<bool,sint32>(true, karte_t::WORLD_MAIL_GENERATED), true );
	/**
	 * Get monthly statistics of ratio of factories that got supplied.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_ratio_goods",       freevariable<bool,sint32>(true, karte_t::WORLD_GOODS_RATIO), true );
	/**
	 * Get monthly statistics of total number of transported goods.
	 * @returns array, index [0] corresponds to current month
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_transported_goods", freevariable<bool,sint32>(true, karte_t::WORLD_TRANSPORTED_GOODS), true );

	/**
	 * Get per year statistics of total number of citizens.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_citizens",          freevariable<bool,sint32>(false, karte_t::WORLD_CITIZENS), true );
	/**
	 * Get per year statistics of total city growth.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_growth",            freevariable<bool,sint32>(false, karte_t::WORLD_GROWTH), true );
	/**
	 * Get per year statistics of total number of towns.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_towns",             freevariable<bool,sint32>(false, karte_t::WORLD_TOWNS), true );
	/**
	 * Get per year statistics of total number of factories.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_factories",         freevariable<bool,sint32>(false, karte_t::WORLD_FACTORIES), true );
	/**
	 * Get per year statistics of total number of convoys.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_convoys",           freevariable<bool,sint32>(false, karte_t::WORLD_CONVOIS), true );
	/**
	 * Get per year statistics of total number of citycars.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_citycars",          freevariable<bool,sint32>(false, karte_t::WORLD_CITYCARS), true );
	/**
	 * Get per year statistics of ratio transported to generated passengers.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_ratio_pax",         freevariable<bool,sint32>(false, karte_t::WORLD_PAS_RATIO), true );
	/**
	 * Get per year statistics of total number of generated passengers.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_pax city_x::get_transported_pax
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_generated_pax",     freevariable<bool,sint32>(false, karte_t::WORLD_PAS_GENERATED), true );
	/**
	 * Get per year statistics of ratio transported to generated mail.
	 * @returns array, index [0] corresponds to current year
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_ratio_mail",        freevariable<bool,sint32>(false, karte_t::WORLD_MAIL_RATIO), true );
	/**
	 * Get per year statistics of total number of generated mail.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_generated_mail",    freevariable<bool,sint32>(false, karte_t::WORLD_MAIL_GENERATED), true );
	/**
	 * Get per year statistics of ratio of factories that got supplied.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_ratio_goods",       freevariable<bool,sint32>(false, karte_t::WORLD_GOODS_RATIO), true );
	/**
	 * Get per year statistics of total number of transported goods.
	 * @returns array, index [0] corresponds to current year
	 * @see city_x::get_generated_mail city_x::get_transported_mail
	 */
	STATIC register_method_fv(vm, &get_world_stat, "get_year_transported_goods", freevariable<bool,sint32>(false, karte_t::WORLD_TRANSPORTED_GOODS), true );

	/**
	 * @returns true if timeline play is active
	 */
	STATIC register_method(vm, &karte_t::use_timeline, "use_timeline");

	/**
	 * Returns iterator through the list of attractions on the map.
	 * @returns iterator class.
	 * @typemask attraction_list_x()
	 */
	STATIC register_function(vm, world_get_attraction_list, "get_attraction_list", 1, ".");
	/**
	 * Returns list of convoys on the map.
	 * @returns convoy list
	 * @typemask convoy_list_x()
	 */
	STATIC register_function(vm, world_get_convoy_list, "get_convoy_list", 1, ".");
	/**
	 * Returns size of the map.
	 * @typemask coord()
	 */
	STATIC register_function(vm, world_get_size, "get_size", 1, ".");

	end_class(vm);

	/**
	 * Implements iterator to iterate through the list of all attractions on the map.
	 *
	 * Usage:
	 * @code
	 * local list = world.get_attraction_list()
	 * // list is now of type attraction_list_x
	 * foreach(att in list) {
	 *     ... // att is an instance of the building_x class
	 * }
	 * @endcode
	 */
	create_class(vm, "attraction_list_x");
	/**
	 * Meta-method to be used in foreach loops to loop over all attractions on the map. Do not call it directly.
	 */
	register_function(vm, world_attraction_list_next,  "_nexti",  2, ". o|i");
	/**
	 * Meta-method to be used in foreach loops to loop over all attractions on the map. Do not call it directly.
	 */
	register_method(vm,   world_attraction_list_get,   "_get", true);
	/**
	 * Returns number of attractions in the list.
	 * @typemask integer()
	 */
	register_function(vm, world_get_attraction_count, "get_count",  1, "x");

	end_class(vm);

	/**
	 * Returns pakset name as set in ground.outside.pak
	 */
	register_method(vm, get_pakset_name, "get_pakset_name");

	/**
	 * Returns simutrans version number
	 */
	register_method(vm, get_version_number, "get_version_number");
}
