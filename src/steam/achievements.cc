#include "achievements.h"
#include "../simutrans/simachenum.h"
#include "../simutrans/simdebug.h"
#include "isteamuser.h"
#include "isteamutils.h"

// Generating the initial array from the enum. I love the X Macro, so powerful!
#define X(id) { id, #id, "", "", 0, 0 },
achievement_t  achievements_list[] = {
	ACHIEVEMENTS
};
#undef X

steam_achievements_t::steam_achievements_t()
	: app_id(0),
	  stats_initialized(false),
	  m_CallbackUserStatsReceived(this, &steam_achievements_t::on_user_stats_received),
	  m_CallbackUserStatsStored(this, &steam_achievements_t::on_user_stats_stored),
	  m_CallbackAchievementStored(this, &steam_achievements_t::on_achievement_stored) {
	app_id = SteamUtils()->GetAppID();
	achievements = achievements_list;
	request_stats();
}

bool steam_achievements_t::request_stats() {
	if (NULL == SteamUserStats() || NULL == SteamUser()) {
		return false;
	}
	if (!SteamUser()->BLoggedOn()) {
		return false;
	}
	return SteamUserStats()->RequestCurrentStats();
}

void steam_achievements_t::on_user_stats_received(UserStatsReceived_t *callback) {
	if (app_id == callback->m_nGameID) {
		if (k_EResultOK == callback->m_eResult) {
			dbg->message("steam_achievements_t::on_user_stats_received", "Received stats and achievements from Steam\n");
			stats_initialized = true;

			// load achievements
			for (int iAch = 0; iAch < NUM_ACHIEVEMENTS; ++iAch) {
				achievement_t &ach = achievements[iAch];

				SteamUserStats()->GetAchievement(ach.m_pchAchievementID, &ach.m_bAchieved);
				snprintf(ach.m_rgchName, sizeof(ach.m_rgchName), "%s", SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, "name"));
				snprintf(ach.m_rgchDescription, sizeof(ach.m_rgchDescription), "%s",
						 SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, "desc"));
			}
			// retry achievements sent before the callback completed
			for (int ach_id : achievements_queue) {
				set_achievement(ach_id);
			}
			achievements_queue.clear();
		} else {
			dbg->error("steam_achievements_t::on_user_stats_received", "RequestStats - failed, %d\n", callback->m_eResult);
		}
	}
}

void steam_achievements_t::on_user_stats_stored(UserStatsStored_t *callback) {
	if (app_id == callback->m_nGameID) {
		if (k_EResultOK == callback->m_eResult) {
			dbg->message("steam_achievements_t::on_user_stats_stored", "Stored stats for Steam\n");
		} else {
			dbg->error("steam_achievements_t::on_user_stats_received", "StatsStored - failed, %d\n", callback->m_eResult);
		}
	}
}

void steam_achievements_t::on_achievement_stored(UserAchievementStored_t *callback) {
	if (app_id == callback->m_nGameID) {
		dbg->message("steam_achievements_t::on_achievement_stored", "Stored Achievement for Steam\n");
	}
}

bool steam_achievements_t::set_achievement(int ach_enum) {
	if (stats_initialized && !achievements[ach_enum].m_bAchieved) {
		dbg->message("steam_achievements_t::set_achievement", "Earned achievement %s\n", achievements[ach_enum].m_pchAchievementID);
		SteamUserStats()->SetAchievement(achievements[ach_enum].m_pchAchievementID);
		return SteamUserStats()->StoreStats();
	} else {
		// Too early! Add them to the queue to retry on_user_stats_received
		achievements_queue.push_back(ach_enum);
	}
	return false;
}