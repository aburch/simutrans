#include "isteamuserstats.h"
#include "steam_api.h"

enum EAchievements {
	PLAY_PAK192_COMIC = 0,
};

struct achievement_t {
	int m_eAchievementID;
	const char *m_pchAchievementID;
	char m_rgchName[128];
	char m_rgchDescription[256];
	bool m_bAchieved;
	int m_iIconImage;
};

#define _ACH_ID(id, name) \
	{ id, #id, name, "", 0, 0 }

class steam_achievements_t {
private:
	uint64 app_id;
	achievement_t *achievements;
	int num_achievements;
	bool stats_initialized;

public:
	steam_achievements_t();

	bool request_stats();
	bool set_achievement(const char *ID);
	achievement_t *get_achievements() { return achievements; };

	STEAM_CALLBACK(steam_achievements_t, on_user_stats_received, UserStatsReceived_t, m_CallbackUserStatsReceived);
	STEAM_CALLBACK(steam_achievements_t, on_user_stats_stored, UserStatsStored_t, m_CallbackUserStatsStored);
	STEAM_CALLBACK(steam_achievements_t, on_achievement_stored, UserAchievementStored_t, m_CallbackAchievementStored);
};