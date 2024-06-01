#include "./simachenum.h"
#include "./world/simworld.h"

/**
 * Manages the achievement system. This currently only works for Steam achievements.
 * TODO: It would be cool to have our own achievement system!
 */
class simachievements_t {
private:

public:
	// Check achievements for loading the current pakset
	static void check_pakset_ach();

	// Check achievements for querying a specific object
	static void check_query_ach(const char* object_name);

	// Check achievements that depend on game state
	static void check_state_ach(karte_t* world);

	// Set the given achievement. No check is made, use after the condition is fullfilled!
	static void set_achievement(simachievements_enum ach);

	// TODO: A function to set achievement from script tools (for scenarios, tutorial, etc...)
};