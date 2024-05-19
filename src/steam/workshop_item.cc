#include "workshop_item.h"

#include <filesystem>
#include <set>

#include "../simutrans/dataobj/environment.h"
#include "../simutrans/simdebug.h"
#include "../simutrans/sys/simsys.h"
#include "../simutrans/utils/simstring.h"
namespace fs = std::filesystem;

// Current categories available on Steam Workshop
static const std::set<std::string> valid_paksets({"pak", "Pak64", "Pak128", "pak128.japan", "pak192.comic"});
static const std::set<std::string> valid_categories({"pakset", "map", "addon", "Scenario"});

workshop_item_t::workshop_item_t(PublishedFileId_t id, std::vector<std::string> files) {
	this->id = id;
	this->files = files;
};

workshop_item_t::workshop_item_t(PublishedFileId_t id, std::string title) {
	this->id = id;
	str_to_lowercase(title);
	this->title = title;
};

bool workshop_item_t::set_category_tag(std::string tag) {
	auto search = valid_categories.find(tag);
	if (search != valid_categories.end()) {
		category_tag = tag;
		return true;
	}
	return false;
}

bool workshop_item_t::add_pakset_tag(std::string tag) {
	auto search = valid_paksets.find(tag);
	if (search != valid_paksets.end()) {
		str_to_lowercase(tag);
		if (tag == "pak") {
			pakset_tags.push_back("pak64");
		} else {
			pakset_tags.push_back(tag);
		}
		return true;
	}

	return false;
}

void workshop_item_t::fill_paths() {
	if (category_tag == "map") {
		paths.push_back("maps");
	} else if (category_tag == "pakset") {
		paths.push_back("paksets" + std::string(PATH_SEPARATOR) + title);
	} else if (category_tag == "addon") {
		for (auto pakset_name : pakset_tags) {
			paths.push_back("addons" + std::string(PATH_SEPARATOR) + pakset_name);
		}
	} else if (category_tag == "Scenario") {
		for (auto pakset_name : pakset_tags) {
			paths.push_back("addons" + std::string(PATH_SEPARATOR) + pakset_name + std::string(PATH_SEPARATOR) + "scenario" + std::string(PATH_SEPARATOR) +
							this->title);
		}
	}
}

bool workshop_item_t::install(std::string origin) {
	this->fill_paths();
	if (paths.size()) {
		for (auto path : paths) {
			fs::create_directories(env_t::user_dir + path);
			const auto copyOptions = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive;
			std::filesystem::copy(origin, env_t::user_dir + path, copyOptions);
			dbg->message("workshop_item_t::install()", "Installed workshop item %lu from %s to %s", this->id, origin.c_str(), (env_t::user_dir + path).c_str());
		}
		// For other categories we save the full list of files, we will need them when uninstalling to avoid deleting unrelated items
		if (this->category_tag == "map" || this->category_tag == "addon") {
			std::vector<std::string> files;
			for (auto path : paths) {
				for (auto const& dir_entry : std::filesystem::directory_iterator{origin}) {
					files.push_back(env_t::user_dir + path + std::string(PATH_SEPARATOR) + dir_entry.path().filename().string());
				}
			}
			this->files = files;
		} else {
			this->files = paths;
		}
		return true;
	}
	dbg->error("workshop_item_t::install()", "Could NOT install workshop item %lu: no valid paths!", this->id);
	return false;
}

void workshop_item_t::uninstall() {
	if(files.size()){
		for(auto file : files ) {
			fs::remove_all(file);
		}
	}
	dbg->message("workshop_item_t::uninstall()", "Uninstalled workshop item %lu", this->id);
}