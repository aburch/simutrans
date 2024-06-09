/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "sve_cache.h"
#include <time.h>

#include "../dataobj/loadsave.h"
#include "../utils/simstring.h"
#include "../pathes.h"
#include "../sys/simsys.h"
#include "../simversion.h"
#include "../dataobj/environment.h"

#include <sys/stat.h>


stringhashtable_tpl<sve_info_t *> sve_cache_t::cached_info;


sve_info_t::sve_info_t()
	: pak("")
	, mod_time(0)
	, file_size(0)
	, file_exists(false)
{
}


sve_info_t::sve_info_t(const char *pak_, time_t mod_, sint32 fs)
	: pak("")
	, mod_time(mod_)
	, file_size(fs)
{
	if(pak_) {
		pak = pak_;
		file_exists = true;
	}
}


bool sve_info_t::operator== (const sve_info_t &other) const
{
	return (mod_time==other.mod_time)  &&  (file_size == other.file_size)  &&  (pak.compare(other.pak)==0);
}


void sve_info_t::rdwr(loadsave_t *file)
{
	const char *s = strdup(pak.c_str());
	file->rdwr_str(s);
	if (file->is_loading()) {
		pak = s ? s : "<unknown pak>";
	}

	free(const_cast<char *>(s));
	file->rdwr_longlong(mod_time);
	file->rdwr_long(file_size);
}


void sve_cache_t::load_cache()
{
	// load cached entries
	if (!cached_info.empty()) {
		// already loaded
		return;
	}

	loadsave_t file;
	// We rename the old cache file and remove any incomplete read version.
	// Upon an error the cache will be rebuilt then next time.
	dr_rename( SAVE_PATH_X "_cached.xml", SAVE_PATH_X "_load_cached.xml" );
	if(  file.rd_open(SAVE_PATH_X "_load_cached.xml") != loadsave_t::FILE_STATUS_OK  ) {
		return;
	}

	// ignore comment
	const char *text = NULL;
	file.rdwr_str(text);

	bool ok = true;
	do {
		xml_tag_t t(&file, "save_game_info");
		// first filename
		file.rdwr_str(text);

		if ((ok = !strempty(text))) {
			sve_info_t *svei = new sve_info_t();
			svei->rdwr(&file);
			cached_info.put(text, svei);
			text = NULL; // it is used as key, do not delete it
		}
	} while (ok);

	if (text) {
		free(const_cast<char *>(text));
	}

	file.close();
	dr_rename( SAVE_PATH_X "_load_cached.xml", SAVE_PATH_X "_cached.xml" );
}


void sve_cache_t::write_cache()
{
	static const char *cache_file = SAVE_PATH_X "_cached.xml";

	loadsave_t file;
	if(  file.wr_open(cache_file, loadsave_t::xml, 0, "cache", SAVEGAME_VER_NR) != loadsave_t::FILE_STATUS_OK  ) {
		return;
	}

	const char *text = "Automatically generated file. Do not edit. An invalid file may crash the game. Deleting is allowed though.";
	file.rdwr_str(text);
	for(auto const& i : cached_info) {
		// save only existing files
		if (i.value->file_exists) {
			xml_tag_t t(&file, "save_game_info");
			char const* filename = i.key;
			file.rdwr_str(filename);
			i.value->rdwr(&file);
		}
	}
	// mark end with empty entry
	{
		xml_tag_t t(&file, "save_game_info");
		text = "";
		file.rdwr_str(text);
	}
	file.close();
}


const char *sve_cache_t::get_info(const std::string &fname)
{
	static char date[1024];

	std::string pak_extension;

	// get file information
	struct stat sb;
	if (dr_stat(fname.c_str(), &sb) != 0) {
		// file not found?
		date[0] = 0;
		return date;
	}

	// check hash table
	sve_info_t *svei = cached_info.get(fname.c_str());
	if (svei   &&  svei->file_size == sb.st_size  &&  svei->mod_time == sb.st_mtime) {
		// compare size and mtime
		// if both are equal then most likely the files are the same
		// no need to read the file for pak_extension
		pak_extension = svei->pak.c_str();
		svei->file_exists = true;
	}
	else {
		// read pak_extension from file
		loadsave_t test;
		test.rd_open(fname.c_str());
		// add pak extension
		pak_extension = test.get_pak_extension();

		// now insert in hash_table
		sve_info_t *svei_new = new sve_info_t(pak_extension.c_str(), sb.st_mtime, sb.st_size );
		// copy filename
		char *key = strdup(fname.c_str());
		sve_info_t *svei_old = cached_info.set(key, svei_new);
		delete svei_old;
	}

	// write everything in string
	// add pak extension
	const size_t n = snprintf( date, lengthof(date), "%s - ", pak_extension.c_str());

	// add the time too
	struct tm *tm = localtime(&sb.st_mtime);
	if(tm) {
		strftime(date+n, 18, "%Y-%m-%d %H:%M", tm);
	}
	else {
		tstrncpy(date, "??.??.???? ??:??", lengthof(date));
	}

	date[lengthof(date)-1] = 0;
	return date;
}


std::string sve_cache_t::get_most_recent_compatible_save()
{
	const char *best_filename = "";
	sint64 best_time = 0;

	for (stringhashtable_tpl<sve_info_t *>::iterator it = cached_info.begin(); it != cached_info.end(); ++it) {
		if (it->value->pak+PATH_SEPARATOR == env_t::pak_name && it->value->mod_time > best_time) {
			best_filename = it->key;
			best_time = it->value->mod_time;
		}
	}

	return best_filename;
}
