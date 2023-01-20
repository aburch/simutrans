/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_world.cc exports world-map functions. */

#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "get_next.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../world/simworld.h"
#include "../../simversion.h"
#include "../../player/simplay.h"
#include "../../obj/gebaeude.h"
#include "../../obj/label.h"
#include "../../descriptor/ground_desc.h"
#include "../../simware.h"
#include "../../builder/goods_manager.h"
#include "../../simhalt.h"
#include "../../simfab.h"


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


uint32 world_generate_goods(karte_t *welt, koord from, koord to, const goods_desc_t *desc, uint32 count)
{
	if (count == 0 || count >= 1<<23) {
		return 0;
	}

	planquadrat_t *fromplan = welt->access(from);
	planquadrat_t *toplan = welt->access(to);

	if (!fromplan || !toplan) {
		return haltestelle_t::NO_ROUTE;
	}
	else if (fromplan->get_haltlist_count() == 0) {
		// not reachable
		return haltestelle_t::NO_ROUTE;
	}

	ware_t good(desc);
	good.set_zielpos(to);
	good.menge = count;
	good.to_factory = fabrik_t::get_fab(to) != NULL;

	ware_t return_good(desc);

	const halthandle_t *haltlist = fromplan->get_haltlist();
	const uint8 hl_count = fromplan->get_haltlist_count();
	const bool no_route_over_overcrowded = welt->get_settings().is_no_routing_over_overcrowding();

	assert(hl_count > 0);
	const int route_result = haltestelle_t::search_route(haltlist, hl_count, no_route_over_overcrowded, good, &return_good);

	switch (route_result) {
	case haltestelle_t::ROUTE_WALK:
		if (good.get_desc() == goods_manager_t::passengers) {
			haltlist[0]->add_pax_walked(count);
		}
		break;

	case haltestelle_t::ROUTE_OVERCROWDED:
		if (good.get_desc() == goods_manager_t::passengers) {
			haltlist[0]->add_pax_unhappy(count);
		}
		break;

	case haltestelle_t::NO_ROUTE:
		if (good.get_desc() == goods_manager_t::passengers) {
			haltlist[0]->add_pax_no_route(count);
		}
		break;

	case haltestelle_t::ROUTE_OK: {
		assert(route_result == haltestelle_t::ROUTE_OK);
		const halthandle_t start_halt = return_good.get_ziel();
		start_halt->starte_mit_route(good);
		if (good.get_desc() == goods_manager_t::passengers) {
			start_halt->add_pax_happy(good.menge);
		}
		}
		break;
	}

	return route_result;
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
	declare_fake_param(label_list_t, "label_list_x");
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

SQInteger world_get_label_list(HSQUIRRELVM vm)
{
	return push_instance(vm, "label_list_x");
}

uint32 world_get_label_count(label_list_t)
{
	return welt->get_label_list().get_count();
}

SQInteger world_label_list_next(HSQUIRRELVM vm)
{
	return generic_get_next(vm, welt->get_label_list().get_count());
}

label_t* world_label_list_get(label_list_t, uint32 index)
{
	auto list = welt->get_label_list();
	if (index < list.get_count()) {
		koord k = list[index];
		if (grund_t *gr = welt->lookup_kartenboden(k)) {
			return gr->find<label_t>();
		}
	}
	return NULL;
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

		/**
		 * Generates goods (passengers, mail or freight) that want to travel from @p from to @p to.
		 * Updates halt statistics (happy, unhappy, no route) for passengers if possible.
		 *
		 * @param from start position for good
		 * @param to destination position for good
		 * @param desc Good descriptor
		 * @param count Number of goods to generate
		 * @retval 0 No route to destination or start position not valid
		 * @retval 1 Passengers/mail/freight successfully generated
		 * @retval 2 Destination is within station catchment area
		 * @retval 8 Route is overcrowded (if no_routing_over_overcrowded is enabled)
		 * @ingroup scen_only
		 */
		STATIC register_method(vm, &world_generate_goods, "generate_goods", true);
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
	 * Returns iterator through the list of labels on the map.
	 * @returns iterator class.
	 * @typemask label_list_x()
	 */
	STATIC register_function(vm, world_get_label_list, "get_label_list", 1, ".");

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
	 * Implements iterator to iterate through the list of all labels on the map.
	 *
	 * Usage:
	 * @code
	 * local list = world.get_label_list()
	 * // list is now of type label_list_x
	 * foreach(label in list) {
	 *     ... // label is an instance of the label_x class
	 * }
	 * @endcode
	 */
	create_class(vm, "label_list_x");
	/**
	 * Meta-method to be used in foreach loops to loop over all labels on the map. Do not call it directly.
	 */
	register_function(vm, world_label_list_next,  "_nexti",  2, ". o|i");
	/**
	 * Meta-method to be used in foreach loops to loop over all labels on the map. Do not call it directly.
	 */
	register_method(vm, world_label_list_get,   "_get", true);
	/**
	 * Returns number of labels in the list.
	 * @typemask integer()
	 */
	register_method(vm, world_get_label_count, "get_count", true);

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
