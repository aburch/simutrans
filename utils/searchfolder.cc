#include <string>
#include <string.h>

#ifndef _MSC_VER
#include <dirent.h>
#else
#include <io.h>
#endif

#include "../simdebug.h"
#include "../simmem.h"
#include "simstring.h"
#include "searchfolder.h"

/*
 *
 *  Description:
 *      If filepath ends with a slash, search for all files in the
 *      directory having the given extension.
 *      If filepath does not end with a slash and it also doesn't contain
 *      a dot after the last slash, then append extension to filepath and
 *      search for it.
 *	Otherwise searches directly for filepath.
 *
 *	No wildcards please!
 *
 *  Return type:
 *      int			number of matching files.
*/

void searchfolder_t::add_entry(const std::string &path, const char *entry, const bool prepend)
{
	const size_t entry_len = strlen(entry);
	char *c = MALLOCN(char, path.length() + entry_len + 1);

	if(prepend) {
		sprintf(c,"%s%s",path.c_str(),entry);
	}
	else{
		sprintf(c,"%s",entry);
	}
	files.append(c);
}


void searchfolder_t::clear_list()
{
	FOR(vector_tpl<char*>, const i, files) {
		guarded_free(i);
	}
	files.clear();
}


int searchfolder_t::search(const std::string &filepath, const std::string &extension, const bool only_directories, const bool prepend_path)
{
	clear_list();
	return search_path(filepath, extension, only_directories, prepend_path);
}


int searchfolder_t::search_path(const std::string &filepath, const std::string &extension, const bool only_directories, const bool prepend_path)
{
	std::string path(filepath);
	std::string name;
	std::string lookfor;
	std::string ext;

	if(extension.empty()) {
		//path=name;
		name = std::string("*");
		ext = std::string("");
	}
	else if(path[path.size() - 1] == '/') {
		// Look for a directory
		name = "*";
		ext = std::string(".") + extension;
	}
	else {
		int slash = path.rfind('/');
		int dot = path.rfind('.');

		if(dot == -1 || dot < slash) {
			// Look for a file with default extension
			name = path.substr(slash + 1, std::string::npos);
			path = path.substr(0, slash + 1);
			ext = std::string(".") + extension;
		}
		else {
			// Look for a file with own extension
			ext = path.substr(dot, std::string::npos);
			name = path.substr(slash + 1, dot - slash - 1);
			path = path.substr(0, slash + 1);
		}
	}
#ifdef _MSC_VER
	lookfor = path + name + ext;
	struct _finddata_t entry;
	intptr_t hfind = _findfirst(lookfor.c_str(), &entry);

	if(hfind != -1) {
		lookfor = ext;
		do {
			size_t entry_len = strlen(entry.name);

			if(  stricmp( entry.name + entry_len - lookfor.length(), lookfor.c_str() ) == 0  ) {
				if(only_directories) {
					if ((entry.attrib & _A_SUBDIR)==0) {
						continue;
					}
				}
				add_entry(path,entry.name,prepend_path);
			}
		} while(_findnext(hfind, &entry) == 0 );
	}
#else
	lookfor = path + ".";

	if (DIR* const dir = opendir(lookfor.c_str())) {
		lookfor = (name == "*") ? ext : name + ext;

		while (dirent const* const entry = readdir(dir)) {
			if(entry->d_name[0]!='.' || (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {
				int entry_len = strlen(entry->d_name);
				if (strcasecmp(entry->d_name + entry_len - lookfor.size(), lookfor.c_str()) == 0) {
					add_entry(path,entry->d_name,prepend_path);
				}
			}
		}
		closedir(dir);
	}
	(void) only_directories;
#endif
	return files.get_count();
}

std::string searchfolder_t::complete(const std::string &filepath, const std::string &extension)
{
	if(filepath[filepath.size() - 1] != '/') {
		int slash = filepath.rfind('/');
		int dot = filepath.rfind('.');

		if(dot == -1 || dot < slash) {
			return filepath + "." + extension;
		}
	}
	return filepath;
}


/*
 * \note since we explicitly alloc the char *'s we must free them here
 */
searchfolder_t::~searchfolder_t()
{
	FOR(vector_tpl<char*>, const i, files) {
		guarded_free(i);
	}
	files.clear();
}
