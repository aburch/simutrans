#include <string>
#include <string.h>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#endif

#include "../simdebug.h"
#include "../simmem.h"
#include "simstring.h"
#include "searchfolder.h"

/*
 *  Author:
 *      Volker Meyer
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
int searchfolder_t::search(const std::string &filepath, const std::string &extension)
{
	std::string path(filepath);
	std::string name;
	std::string lookfor;
	std::string ext;

	for(  vector_tpl<char *>::const_iterator i = files.begin(), end = files.end();  i != end;  ++i  ) {
		guarded_free(*i);
	}
	files.clear();

	if(path[path.size() - 1] == '/') {
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
			path = path.substr(slash + 1);
			ext = std::string(".") + extension;
		}
		else {
			// Look for a file with own extension
 			ext = path.substr(dot, std::string::npos);
			name = path.substr(slash + 1, dot - slash - 1);
			path = path.substr(slash + 1);
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
				char* const c = MALLOCN(char, path.length() + entry_len + 1);
				sprintf(c,"%s%s",path.c_str(),entry.name);
				files.append(c);
			}
		} while(_findnext(hfind, &entry) == 0 );
	}
#else
	lookfor = path + ".";

	DIR* dir = opendir(lookfor.c_str());
	struct  dirent  *entry;

	if(dir != NULL) {
		lookfor = (name == "*") ? ext : name + ext;

		while((entry = readdir(dir)) != NULL) {
			if(entry->d_name[0]!='.' || (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {
				int entry_len = strlen(entry->d_name);
				if (strcasecmp(entry->d_name + entry_len - lookfor.size(), lookfor.c_str()) == 0) {
					char* const c = MALLOCN(char, path.size() + entry_len + 1);
					sprintf(c,"%s%s", path.c_str(), entry->d_name);
					files.append(c);
				}
			}
		}
		closedir(dir);
	}
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
 * since we explicitly alloc the char *'s we must free them here
 */
searchfolder_t::~searchfolder_t()
{
	for (vector_tpl<char*>::const_iterator i = files.begin(), end = files.end(); i != end; ++i) {
		guarded_free(*i);
	}
}
