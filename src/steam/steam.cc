/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "steam.h"

#include <stdio.h>

#include <fstream>

#include "../simutrans/dataobj/environment.h"
#include "../simutrans/simdebug.h"
#include "isteamugc.h"
#include "isteamutils.h"
#include "steam_api.h"

#define MAX_WORKSHOP_ITEMS 1000	// API fetch limit
#define MAX_TAG_LENGTH 100

steam_t* steam_t::steam = nullptr;

steam_t::steam_t() {
	if (!(is_api_initialized = SteamAPI_Init())) {
		dbg->warning("steam_t::steam_t()", "Steam API Initialization FAILED, Steam functionality won't be available!");
	} else {
		steam_achievements = new steam_achievements_t();
	}
}

steam_t* steam_t::get_instance() {
	if (steam == nullptr) {
		steam = new steam_t();
	}
	return steam;
}

/**
 * 0. Get previously installed items (saved in wokshop.txt)
 * 1. Get subscribed items: https://partner.steamgames.com/doc/api/ISteamUGC#GetSubscribedItems
 * 2. Compare to ge new items and items to uninstall
 * 3. Uninstall old items that are no longer subscribed
 * 4. Get details of the new susbcribed items to fetch tags:
 * 5. Get the number of tags for each new item: https://partner.steamgames.com/doc/api/ISteamUGC#GetQueryUGCNumKeyValueTags
 * 6. Get the values of the tags for each new item: https://partner.steamgames.com/doc/api/ISteamUGC#GetQueryUGCKeyValueTag
 * 7. Install the new items in an appropiate location based on their tags
 */
void steam_t::install_workshop_items() {
	if (!is_api_initialized)
		return;

	std::vector<workshop_item_t> previously_installed_items = steam_t::read_installed_items();

	int num_subscribed_items = SteamUGC()->GetNumSubscribedItems();

	PublishedFileId_t subscribed_items[MAX_WORKSHOP_ITEMS];
	SteamUGC()->GetSubscribedItems(subscribed_items, num_subscribed_items);

	existing_items = steam_t::uninstall_old_items(previously_installed_items, subscribed_items, num_subscribed_items);

	new_items_ids = steam_t::get_new_items(existing_items, subscribed_items, num_subscribed_items);

	if (new_items_ids.size()) {
		UGCQueryHandle_t ugc_query = SteamUGC()->CreateQueryUGCDetailsRequest(new_items_ids.data(), new_items_ids.size());
		if (ugc_query != k_UGCQueryHandleInvalid) {
			SteamUGC()->SetReturnMetadata(ugc_query, true);
			SteamAPICall_t hSteamAPICall = SteamUGC()->SendQueryUGCRequest(ugc_query);
			get_items_details_result.Set(hSteamAPICall, steam, &steam_t::on_get_items_details);
			dbg->message("steam_t::install_workshop_items()", "Fetching details of new items...");
			is_installing = true;
			// Calls will resolve asynchronously, but we need to wait until paksets are installed!
			while (is_installing) {
				SteamAPI_RunCallbacks();
			}
		}
	} else if (existing_items.size() != previously_installed_items.size()) {
		write_existing_items();
	}
}

void steam_t::on_get_items_details(SteamUGCQueryCompleted_t* callback, bool failure) {
	std::vector<workshop_item_t> installed_items;
	if (!failure && callback->m_eResult == 1) {
		int num_results = callback->m_unNumResultsReturned;
		int num_tags_vector[MAX_WORKSHOP_ITEMS];

		for (int result_index = 0; result_index < num_results; result_index++) {
			num_tags_vector[result_index] = SteamUGC()->GetQueryUGCNumTags(callback->m_handle, result_index);
		}
		for (int result_index = 0; result_index < num_results; result_index++) {
			SteamUGCDetails_t pDetails = SteamUGCDetails_t();
			SteamUGC()->GetQueryUGCResult(callback->m_handle, result_index, &pDetails);
			workshop_item_t item = workshop_item_t(new_items_ids[result_index], pDetails.m_rgchTitle);

			for (int tag_index = 0; tag_index < num_tags_vector[result_index]; tag_index++) {
				char tag[MAX_TAG_LENGTH];
				SteamUGC()->GetQueryUGCTagDisplayName(callback->m_handle, result_index, tag_index, tag, MAX_TAG_LENGTH);
				if (!item.set_category_tag(tag) && !item.add_pakset_tag(tag)) {
					dbg->error("steam_t::on_get_items_details()", "Tag name '%s' is not a valid tag!", tag);
				}
			}

			char folder[1024];
			SteamUGC()->GetItemInstallInfo(item.get_id(), NULL, folder, sizeof(folder), NULL);
			if (item.install(folder)) {
				installed_items.push_back(item);
			}
		}
	}
	steam_t::write_installed_items(installed_items);
	is_installing = false;
	SteamUGC()->ReleaseQueryUGCRequest(callback->m_handle);
}

/**
 * Get the list of installed items from a command-separated list in a file
 * First value should be the workshop item id, then files installed
 */
std::vector<workshop_item_t> steam_t::read_installed_items() {
	std::string file_path = std::string(env_t::base_dir) + "workshop.txt";
	std::vector<workshop_item_t> installed_items;
	std::ifstream file(file_path);

	if (file.is_open()) {
		std::string line;
		std::string delimiter = ",";
		while (getline(file, line)) {
			// dbg->message("steam_t::read_installed_items()", "Reading line: '%s'", line.c_str());
			std::string id = line.substr(0, line.find(delimiter));
			line.erase(0, line.find(delimiter) + delimiter.length());
			std::vector<std::string> files;
			while (true) {
				std::string path = line.substr(0, line.find(delimiter));
				files.push_back(path);
				// dbg->message("steam_t::read_installed_items()", "Reading line: '%s'", line.c_str());
				if (line.find(delimiter) == line.npos) {
					break;
				} else {
					line.erase(0, line.find(delimiter) + delimiter.length());
				}
			}
			installed_items.push_back({std::stoull(id), files});
		}
		file.close();
	}
	return installed_items;
}

void steam_t::write_installed_items(std::vector<workshop_item_t> installed_items) {
	for (auto item : installed_items) {
		existing_items.push_back(item);
	}
	write_existing_items();
}

void steam_t::write_existing_items() {
	std::string file_path = std::string(env_t::base_dir) + "workshop.txt";
	std::remove(file_path.c_str());
	std::ofstream file(file_path);
	if (file.is_open()) {
		for (auto item : existing_items) {
			std::string line = std::to_string(item.get_id());
			for (auto item_file : item.get_files()) {
				line.append("," + item_file);
			}
			file << line + "\n";
		}
		file.close();
	} else {
		dbg->error("steam_t::write_existing_items()", "Could not open %s file for writing", file_path.c_str());
	}
}

std::vector<PublishedFileId_t> steam_t::get_new_items(std::vector<workshop_item_t> installed_items, PublishedFileId_t subscribed_items[],
													  int num_subscribed_items) {
	std::vector<PublishedFileId_t> new_items;

	for (int i = 0; i < num_subscribed_items; i++) {
		bool found_item = false;
		for (auto item : installed_items) {
			if (item.get_id() == subscribed_items[i]) {
				found_item = true;
				break;
			}
		}
		if (!found_item) {
			new_items.push_back(subscribed_items[i]);
			dbg->message("steam_t::get_new_items()", "Found new workshop item: %lu", subscribed_items[i]);
		}
	}

	return new_items;
}

std::vector<workshop_item_t> steam_t::uninstall_old_items(std::vector<workshop_item_t> installed_items, PublishedFileId_t subscribed_items[],
														  int num_subscribed_items) {
	std::vector<workshop_item_t> old_items;
	std::vector<workshop_item_t> current_items;

	for (auto item : installed_items) {
		bool found_item = false;
		for (int i = 0; i < num_subscribed_items; i++) {
			if (item.get_id() == subscribed_items[i]) {
				found_item = true;
				existing_items.push_back(item);
				break;
			}
		}
		if (!found_item) {
			old_items.push_back(item);
		} else {
			current_items.push_back(item);
		}
	}

	for (auto item : old_items) {
		item.uninstall();
	}

	return current_items;
}

void steam_t::update_ui(uint32 year, uint32 total_convoys) {
	if (!is_api_initialized)
		return;

	std::string pakset = env_t::pak_name;
	pakset.pop_back();	// Remove path separator
	SteamFriends()->SetRichPresence("pakset", pakset.c_str());
	SteamFriends()->SetRichPresence("year", std::to_string(year).c_str());
	SteamFriends()->SetRichPresence("convoys", std::to_string(total_convoys).c_str());
	SteamFriends()->SetRichPresence("steam_display", "#Playing");
	SteamAPI_RunCallbacks();
}

void steam_t::shutdown() {
	SteamAPI_Shutdown();
	if (steam_achievements)
		delete steam_achievements;
}