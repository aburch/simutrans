#include "isteamuserstats.h"
#include "steam_api.h"
#include <vector>

struct achievement_t {
	int m_eAchievementID;
	const char *m_pchAchievementID;
	char m_rgchName[128];
	char m_rgchDescription[256];
	bool m_bAchieved;
	int m_iIconImage;
};

class steam_achievements_t {
private:
	uint64 app_id;
	achievement_t *achievements;
	bool stats_initialized;
	std::vector<int> achievements_queue;

public:
	steam_achievements_t();

	bool request_stats();
	// Note: not declaring type as "simachievements_enum" instead of "int" to avoid circular dependency issues
	bool set_achievement(int ach_enum);
	achievement_t *get_achievements() { return achievements; };

	STEAM_CALLBACK(steam_achievements_t, on_user_stats_received, UserStatsReceived_t, m_CallbackUserStatsReceived);
	STEAM_CALLBACK(steam_achievements_t, on_user_stats_stored, UserStatsStored_t, m_CallbackUserStatsStored);
	STEAM_CALLBACK(steam_achievements_t, on_achievement_stored, UserAchievementStored_t, m_CallbackAchievementStored);
};