/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#	include <sys/stat.h>
#	include <sys/types.h>
#	include <dirent.h>
#else
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <io.h>
#endif

#include "../simdebug.h"
#include "../simmem.h"
#include "../simtypes.h"
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
 * Otherwise searches directly for filepath.
 *
 * No wildcards please!
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
	for(char* const i : files) {
		free(i);
	}
	files.clear();
}


int searchfolder_t::search(const std::string &filepath, const std::string &extension, const search_flags_t search_flags, const int max_depth )
{
	clear_list();
	return prepare_search(filepath, extension, search_flags, max_depth);
}


int searchfolder_t::prepare_search(const std::string &filepath, const std::string &extension, const search_flags_t search_flags, const int max_depth )
{
	std::string path(filepath);
	std::string name;
	std::string ext;

#ifdef _WIN32
	// since we assume hardcoded path are using / we need to correct this for windows
	for(  uint i=0;  i<path.size();  i++  ) {
		if(  path[i]=='\\'  ) {
			path[i] = '/';
		}
	}
#endif

	if(extension.empty()) {
		// Look for all files in a directory
		name = std::string("*");
		ext = std::string("");
	}
	else if(path[path.size() - 1] == '/') {
		// Look for files with a specific extension in a directory
		name = "*";
		ext = std::string(".") + extension;
	}
	else {
		int slash = path.rfind('/');
		int dot = path.rfind('.');

		if(dot == -1 || dot < slash) {
			// Look for a specific file with default extension
			ext = std::string(".") + extension;
			name = path.substr(slash + 1, std::string::npos);
			path = path.substr(0, slash + 1);
		}
		else {
			// e.g. path = foo/bar.baz, ignore @p extension
			ext = path.substr(dot, std::string::npos);
			name = path.substr(slash + 1, dot - slash - 1);
			path = path.substr(0, slash + 1);
		}
	}
	search_path(path, name, ext, search_flags, max_depth);

	return files.get_count();
}

void searchfolder_t::search_path(const std::string path, const std::string name, const std::string ext, const search_flags_t search_flags, const int max_depth )
{
	std::string lookfor;
	const bool only_directories = (search_flags & SF_ONLYDIRS) != 0;
	const bool prepend_path     = (search_flags & SF_PREPEND_PATH) != 0;

#ifdef _WIN32
	lookfor = path + name + ext;

	WCHAR path_inW[MAX_PATH];
	if(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, lookfor.c_str(), -1, path_inW, MAX_PATH) == 0) {
		// Conversion failed so results will be nonsense anyway.
		return;
	}

	struct _wfinddata_t entry;
	intptr_t const hfind = _wfindfirst(path_inW, &entry);
	if(hfind == -1) {
		// Search failed.
		return;
	}

	lookfor = name + ext;
	do {
		// Convert entry name.
		int const entry_name_size = WideCharToMultiByte( CP_UTF8, 0, entry.name, -1, NULL, 0, NULL, NULL );
		char *const entry_name = new char[entry_name_size];
		WideCharToMultiByte( CP_UTF8, 0, entry.name, -1, entry_name, entry_name_size, NULL, NULL );
		if( entry_name[0]!='.' || (entry_name[1]!='.' && entry_name[1]!=0) ) {
			if ((entry.attrib & _A_SUBDIR) && (search_flags&SF_NOADDONS) && !STRICMP(entry_name, "addons")) {
				continue;
			}

			if(  filename_matches_pattern(entry_name, lookfor.c_str()) ) {
				if(only_directories && ((entry.attrib & _A_SUBDIR)==0)) {
					delete[] entry_name;
					continue;
				}
				add_entry(path,entry_name,prepend_path);
			}
			if( ( (entry.attrib & _A_SUBDIR) ) && ( max_depth > 0 ) ) {
				search_path(path + entry_name + '/', name, ext, search_flags, max_depth - 1);
			}
		}
		delete[] entry_name;
	} while(_wfindnext(hfind, &entry) == 0 );
	_findclose(hfind);
#else
	assert(path.empty() || path[path.length()-1] == '/');
	lookfor = path + ".";

	if (DIR* const dir = opendir(lookfor.c_str())) {
		int dir_fd = dirfd(dir);
		if( dir_fd == -1 ) {
			goto close_dir;
		}
		lookfor = name + ext;

		while (dirent const* const entry = readdir(dir)) {
			if(entry->d_name[0]!='.' || (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {

				bool is_dir = false;
				if( entry->d_type == DT_DIR ) {
					is_dir = true;
				}
				else if( entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK ) {
					struct stat st;
					if( fstatat(dir_fd, entry->d_name, &st, 0) == -1 ) {
						continue;
					}
					is_dir = S_ISDIR(st.st_mode);
				}

				if( is_dir && (search_flags&SF_NOADDONS) && !STRICMP(entry->d_name, "addons") ) {
					continue;
				}

				if (only_directories) {
					if( is_dir ) {
						add_entry(path, entry->d_name, prepend_path);
					}
				} else if (filename_matches_pattern(entry->d_name, lookfor.c_str())) {
					add_entry(path, entry->d_name, prepend_path);
				}
				if( is_dir && max_depth > 0 ) {
					search_path(path + entry->d_name + '/', name, ext, search_flags, max_depth - 1);
				}

			}
		}
close_dir:
		closedir(dir);
	}
#endif
}

#ifdef _WIN32
std::string searchfolder_t::complete(const std::string &filepath_raw, const std::string &extension)
{
	std::string filepath(filepath_raw);
	// since we assume hardcoded path are using / we need to correct this for windows
	for(  uint i=0;  i<filepath.size();  i++  ) {
		if(  filepath[i]=='\\'  ) {
			filepath[i] = '/';
		}
	}
#else
std::string searchfolder_t::complete(const std::string &filepath, const std::string &extension)
{
#endif
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
	for(char* const i : files) {
		free(i);
	}
	files.clear();
}


bool searchfolder_t::filename_matches_pattern(const char *fn, const char *pattern)
{
	const size_t pattern_len = strlen(pattern);
	const size_t fn_len = strlen(fn);

	if (pattern_len == 0) {
		return fn_len == 0;
	}

	const bool match_suffix = pattern[0] == '*';

	if (match_suffix) {
		if (fn_len < pattern_len-1) {
			return false;
		}

		const size_t match_off = fn_len - (pattern_len-1);
		return STRICMP(fn+match_off, pattern+1) == 0;
	}
	else {
		return STRICMP(fn, pattern) == 0;
	}
}
