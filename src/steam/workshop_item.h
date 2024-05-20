
#include "isteamugc.h"
#include <string>
#include <vector>

class workshop_item_t {
private:
	PublishedFileId_t id;
    std::string title;
	std::vector<std::string> paths;
	std::vector<std::string> files;
	std::string category_tag;
	std::vector<std::string> pakset_tags;
	void fill_paths();

public:
	workshop_item_t(PublishedFileId_t id, std::vector<std::string> files);
	workshop_item_t(PublishedFileId_t id, std::string title);

	/**
	 * Sets the category tag. It must be "pakset", "scenario", "map", or "addon"
	 * @returns false if it was unable to set category
	 */
	bool set_category_tag(std::string tag);
	bool add_pakset_tag(std::string tag);
    bool install(std::string origin);
    void uninstall();

	PublishedFileId_t get_id() { return id; };
	std::vector<std::string> get_files() { return files; };
};