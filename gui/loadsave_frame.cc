/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"

#include <sys/stat.h>
#include <time.h>

#include "loadsave_frame.h"

#include "../simworld.h"
#include "../simversion.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../pathes.h"
#include "../utils/simstring.h"


stringhashtable_tpl<sve_info_t *> loadsave_frame_t::cached_info;


sve_info_t::sve_info_t(const char *pak_, time_t mod_, long fs)
: pak(""), mod_time(mod_), file_size(fs)
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
		pak = s;
	}
	free(const_cast<char *>(s));
	file->rdwr_longlong(mod_time);
	file->rdwr_long(file_size);
}


/**
 * Aktion, die nach Knopfdruck gestartet wird.
 * @author Hansjörg Malthaner
 */
void loadsave_frame_t::action(const char *filename)
{
	if(do_load) {
		welt->laden(filename);
	}
	else {
		welt->speichern( filename, loadsave_t::save_mode, umgebung_t::savegame_version_str, false );
		welt->set_dirty();
		welt->reset_timer();
	}
}


loadsave_frame_t::loadsave_frame_t(karte_t *welt, bool do_load) : savegame_frame_t(".sve",false,"save/")
{
	this->welt = welt;
	this->do_load = do_load;

	if(do_load) {
		set_name(translator::translate("Laden"));
	}
	else {
		set_filename(welt->get_settings().get_filename());
		set_name(translator::translate("Speichern"));
	}

	// load cached entries
	if (cached_info.empty()) {
		loadsave_t file;
		const char *cache_file = SAVE_PATH_X "_cached.xml";
		if (file.rd_open(cache_file)) {
			// ignore comment
			const char *text=NULL;
			file.rdwr_str(text);

			bool ok = true;
			while (ok) {
				xml_tag_t t(&file, "save_game_info");
				// first filename
				file.rdwr_str(text);
				if (!strempty(text)) {
					sve_info_t *svei = new sve_info_t();
					svei->rdwr(&file);
					cached_info.put(text, svei);
					text = NULL; // it is used as key, do not delete it
				}
				else {
					ok = false;
				}
			}
			if (text) {
				free(const_cast<char *>(text));
			}
			file.close();
		}
	}
}


/**
 * Manche Fenster haben einen Hilfetext assoziiert.
 * @return den Dateinamen für die Hilfe, oder NULL
 * @author Hj. Malthaner
 */
const char *loadsave_frame_t::get_hilfe_datei() const
{
	return do_load ? "load.txt" : "save.txt";
}


const char *loadsave_frame_t::get_info(const char *fname)
{
	static char date[1024];
	date[0] = 0;
	const char *pak_extension = NULL;
	// get file information
	struct stat  sb;
	if(stat(fname, &sb)!=0) {
		// file not found?
		return date;
	}
	// check hash table
	sve_info_t *svei = cached_info.get(fname);
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
		test.rd_open(fname);
		// add pak extension
		pak_extension = test.get_pak_extension();

		// now insert in hash_table
		sve_info_t *svei_new = new sve_info_t(pak_extension, sb.st_mtime, sb.st_size );
		// copy filename
		char *key = strdup(fname);
		sve_info_t *svei_old = cached_info.set(key, svei_new);
		if (svei_old) {
			delete svei_old;
		}
	}

	// write everything in string
	// add pak extension
	size_t n = sprintf( date, "%s - ", pak_extension);

	// add the time too
	struct tm *tm = localtime(&sb.st_mtime);
	if(tm) {
		strftime(date+n, 18, "%Y-%m-%d %H:%M", tm);
	}
	else {
		tstrncpy(date, "??.??.???? ??:??", lengthof(date));
	}
	return date;
}


loadsave_frame_t::~loadsave_frame_t()
{
	// save hashtable
	loadsave_t file;
	const char *cache_file = SAVE_PATH_X "_cached.xml";
	if(  file.wr_open(cache_file, loadsave_t::xml, "cache", SAVEGAME_VER_NR)  ) {
		const char *text="Automatically generated file. Do not edit. An invalid file may crash the game. Deleting is allowed though.";
		file.rdwr_str(text);
		FOR(stringhashtable_tpl<sve_info_t*>, const& i, cached_info) {
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
}
