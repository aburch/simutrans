#include "simachievements.h"

#include "./dataobj/environment.h"
#include "./simdebug.h"
#include "./utils/simstring.h"

#ifdef STEAM_BUILT
#include "../steam/steam.h"
#endif

void simachievements_t::check_pakset_ach() {
	if (!STRICMP(env_t::pak_name.c_str(), "pak192.comic/")) {
		set_achievement(LOAD_PAK192_COMIC);
	}
}

void simachievements_t::check_query_ach(const char* object_name) {
	if (!STRICMP(env_t::pak_name.c_str(), "pak128/") && strstr(object_name, "rmax_dictator_statue")) {
		set_achievement(QUERY_DICTACTOR);
	}
}

void simachievements_t::set_achievement(simachievements_enum ach) {
	dbg->message("simachievements_t::set_achievement()", "Unlocking achievement %d", ach);
#ifdef STEAM_BUILT
	steam_t::get_instance()->get_achievements()->set_achievement(ach);
#endif
}