#include "achievements.h"

#include "../simutrans/simdebug.h"
#include "isteamuser.h"
#include "isteamutils.h"

int NUM_ACHIEVEMENTS = 1;

achievement_t achievements_list[] = {
	_ACH_ID(PLAY_PAK192_COMIC, "Play Pak192.Comic"),
};

steam_achievements_t::steam_achievements_t()
	: app_id(0),
	  stats_initialized(false),
	  m_CallbackUserStatsReceived(this, &steam_achievements_t::on_user_stats_received),
	  m_CallbackUserStatsStored(this, &steam_achievements_t::on_user_stats_stored),
	  m_CallbackAchievementStored(this, &steam_achievements_t::on_achievement_stored) {
	app_id = SteamUtils()->GetAppID();
	achievements = achievements_list;
	num_achievements = NUM_ACHIEVEMENTS;
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
			for (int iAch = 0; iAch < num_achievements; ++iAch) {
				achievement_t &ach = achievements[iAch];

				SteamUserStats()->GetAchievement(ach.m_pchAchievementID, &ach.m_bAchieved);
				snprintf(ach.m_rgchName, sizeof(ach.m_rgchName), "%s", SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, "name"));
				snprintf(ach.m_rgchDescription, sizeof(ach.m_rgchDescription), "%s",
						 SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID, "desc"));
			}
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

bool steam_achievements_t::set_achievement(const char *ID) {
	if (stats_initialized) {
		dbg->message("steam_achievements_t::set_achievement", "Earned achievement %s\n", ID);
		SteamUserStats()->SetAchievement(ID);
		return SteamUserStats()->StoreStats();
	}
	return false;
}