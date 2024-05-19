/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <cstring>
#include <string>
#include <vector>

#include "achievements.h"
#include "isteamugc.h"
#include "workshop_item.h"

/**
 *  Singleton class to prevent the Steam API from being initialized more than once
 */
class steam_t {
private:
	steam_t();
	steam_t(steam_t& other) = delete;
	steam_t& operator=(steam_t& other) = delete;

	static steam_t* steam;

	steam_achievements_t* g_SteamAchievements = NULL;

	bool is_api_initialized = false;
	bool is_installing = false;

	std::vector<PublishedFileId_t> new_items_ids;
	std::vector<workshop_item_t> existing_items;

	CCallResult<steam_t, SteamUGCQueryCompleted_t> get_items_details_result;

	void on_get_items_details(SteamUGCQueryCompleted_t* callback, bool failure);

	std::vector<workshop_item_t> read_installed_items();
	void write_installed_items(std::vector<workshop_item_t> installed_items);
	void write_existing_items();

	std::vector<PublishedFileId_t> get_new_items(std::vector<workshop_item_t> installed_items, PublishedFileId_t subscribed_items[], int num_subscribed_items);

	std::vector<workshop_item_t> uninstall_old_items(std::vector<workshop_item_t> installed_items, PublishedFileId_t subscribed_items[],
													 int num_subscribed_items);
	void update_achievements();

public:
	static steam_t* get_instance();

	void install_workshop_items();

	void update_ui(uint32 year, uint32 total_convoys);

	void shutdown();

};
