#include "./simachenum.h"

/**
 * Manages the achievement system. This currently only works for Steam achievements.
 * TODO: It would be cool to have our own achievement system!
 */
class simachievements_t {
private:
	// Set the given achievement.
	static void set_achievement(simachievements_enum ach);

public:
	// Check achievements for loading the current pakset
	static void check_pakset_ach();

	// Check achievements for querying a specific object
	static void check_query_ach(const char* object_name);

	// TODO: A function to set achievement from script tools (for scenarios, tutorial, etc...)
};