#include "simachievements.h"

#include "./dataobj/environment.h"
#include "./simdebug.h"
#include "./simfab.h"
#include "./utils/simstring.h"

#ifdef STEAM_BUILT
#include "../steam/steam.h"
#endif

void simachievements_t::check_pakset_ach() {
	std::string pak_name = env_t::pak_name;
	pak_name.erase(pak_name.length() - 1);
	if ( STRICMP(pak_name.c_str(), "pak192.comic") == 0 ) {
		set_achievement(ACH_LOAD_PAK192_COMIC);
	}
}

void simachievements_t::check_query_ach(const char* object_name) {
	std::string pak_name = env_t::pak_name;
	pak_name.erase(pak_name.length() - 1);
	if ( (STRICMP(pak_name.c_str(), "pak128") == 0) && strstr(object_name, "rmax_dictator_statue") ) {
		set_achievement(ACH_QUERY_DICTACTOR);
	}
}

/**
 * Given a factory and a name, checks if the factory has the given name
 * If so, checks if the provided product meets the production target requirement (given as a percentage of maximum production)
 * @returns true if the yearly production is greater than the target production
 */
bool check_yearly_production(fabrik_t* fab, const char* name, uint32 product_index, double target_percent = 0.70) {
	if ( strcmp(fab->get_name(), name) != 0 || product_index >= fab->get_output().get_count() ) {
		return false;
	}
	sint64 yearly_production = 0;
	sint32 target_production = fab->get_current_production() * 12 * target_percent;
	for (int i = 0; i < MAX_MONTH; i++) {
		yearly_production += convert_goods(fab->get_output()[product_index].get_stats()[i * MAX_FAB_GOODS_STAT + FAB_GOODS_PRODUCED]);
	}
	// dbg->message("check_state_ach()", "TARGET PROD: %s", std::to_string(target_production).c_str());
	// dbg->message("check_state_ach()", "PROD: %s", std::to_string(yearly_production).c_str());
	return yearly_production >= target_production;
}

void simachievements_t::check_state_ach(karte_t* world) {
	if ( env_t::networkmode )
		return;	 // We don't allow these kind of achievements in multiplayer games
	std::string pak_name = env_t::pak_name;
	pak_name.erase(pak_name.length() - 1);
	for ( fabrik_t* fab : world->get_fab_list() ) {
		if ( STRICMP(pak_name.c_str(), "pak192.comic") == 0 ) {
			if ( check_yearly_production(fab, "squid", 0) ) {
				set_achievement(ACH_PROD_INK);
			}
		}
	}
}

void simachievements_t::set_achievement(simachievements_enum ach) {
	dbg->message("simachievements_t::set_achievement()", "Unlocking achievement %d", ach);
#ifdef STEAM_BUILT
	steam_t::get_instance()->get_achievements()->set_achievement(ach);
#endif
}