/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"

#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
// This gets us the max for a signed 32-bit int.  Hopefully.
#define MAXINT INT_MAX
#else
#include <io.h>
#include <direct.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include "loadsave_frame.h"

#include "../simworld.h"
#include "../simversion.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../pathes.h"
#include "../utils/simstring.h"

#include "components/gui_button.h"

class gui_loadsave_table_row_t : public gui_file_table_row_t
{
public:
	loadsave_t file;

	//gui_loadsave_table_row_t() : gui_file_table_row_t() {};
	gui_loadsave_table_row_t(const char *pathname, const char *buttontext);
};

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
		welt->speichern( filename, umgebung_t::savegame_version_str, umgebung_t::savegame_ex_version_str, false );
		welt->set_dirty();
		welt->reset_timer();
	}
}


bool loadsave_frame_t::del_action(const char *filename)
{
	remove(filename);
	return false;
}


loadsave_frame_t::loadsave_frame_t(karte_t *welt, bool do_load) : savegame_frame_t(".sve", NULL, false, true)
{
	this->welt = welt;
	this->do_load = do_load;
	init(".sve", NULL);
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
				if (text  &&  strlen(text)>0) {
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
const char * loadsave_frame_t::get_hilfe_datei() const
{
	return do_load ? "load.txt" : "save.txt";
}


void loadsave_frame_t::init(const char * /*suffix*/, const char * /*path*/ )
{
	file_table.set_owns_columns(false);
	file_table.add_column(&delete_column);
	file_table.add_column(&action_column);
	file_table.add_column(&date_column);
	file_table.add_column(&pak_column);
	file_table.add_column(&std_column);
	file_table.add_column(&exp_column);
	set_min_windowsize(get_fenstergroesse());
	set_resizemode(diagonal_resize);
	//set_fenstergroesse(koord(640+36, get_fenstergroesse().y));
}


void loadsave_frame_t::add_file(const char *filename, const bool not_cutting_suffix)
{
	char pathname[1024];
	sprintf( pathname, "%s%s", get_path(), filename );
	char buttontext[1024];
	strcpy( buttontext, filename );
	if ( !not_cutting_suffix ) {
		buttontext[strlen(buttontext)-4] = '\0';
	}
	file_table.add_row( new gui_loadsave_table_row_t( pathname, buttontext ));
}


void loadsave_frame_t::set_file_table_default_sort_order()
{
	savegame_frame_t::set_file_table_default_sort_order();
	file_table.set_row_sort_column_prio(3, 0);
	file_table.set_row_sort_column_prio(2, 1);
}
 

gui_loadsave_table_row_t::gui_loadsave_table_row_t(const char *pathname, const char *buttontext) : gui_file_table_row_t(pathname, buttontext)
{
	if (error.empty()) {
		try {
			file.rd_open(pathname);
			file.close();
		}
		catch (char *e) {
			error = e;
		}
		catch (...) {
			error = "failed reading header";
		}
	}
}

gui_file_table_pak_column_t::gui_file_table_pak_column_t() : gui_file_table_label_column_t(150) 
{
	strcpy(pak, umgebung_t::objfilename.c_str());
	pak[strlen(pak) - 1] = 0;
}

/**
 * Get a rate for the similarity of strings.
 *
 * The similar the strings, the lesser the result.
 * Identical strings result to 0.
 * The result rates the number of identical characters.
 */
sint32 strsim(const char a[], const char b[]) 
{	
	int i = 0;
	while (a[i] && b[i] && tolower(a[i]) == tolower(b[i])) i++;
	if (tolower(a[i]) == tolower(b[i]))
		return 0;
	return MAXINT / (i+1);
}


int gui_file_table_pak_column_t::compare_rows(const gui_table_row_t &row1, const gui_table_row_t &row2) const
{
	char s1[1024];
	strcpy(s1, get_text(row1));
	sint32 f1 = strsim(s1, pak);
	char s2[1024];
	strcpy(s2, get_text(row2));
	sint32 f2 = strsim(s2, pak);
	int result = sgn(f1 - f2);
	if (!result)
		result = strcmp(s1, s2);
	dbg->debug("gui_file_table_pak_column_t::compare_rows()", "\"%s\" %s \"%s\"", s1, result < 0 ? "<" : result == 0 ? "==" : ">", s2);
	return result;
}


const char *gui_file_table_pak_column_t::get_text(const gui_table_row_t &row) const
{
 	gui_loadsave_table_row_t &file_row = (gui_loadsave_table_row_t&)row;
	const char *pak = file_row.file.get_pak_extension();
	return strlen(pak) > 3 && (!STRNICMP(pak, "zip", 3) || !STRNICMP(pak, "xml", 3)) ? pak + 3 : pak;
}


void gui_file_table_pak_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) {
	lbl.set_text(get_text(row));
	gui_file_table_label_column_t::paint_cell(offset, x, y, row);
}


sint32 gui_file_table_std_column_t::get_int(const gui_table_row_t &row) const
{
	// file version 
 	gui_loadsave_table_row_t &file_row = (gui_loadsave_table_row_t&)row;
	return (sint32) file_row.file.get_version();
}


void gui_file_table_std_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) {
	uint32 v2 = (uint32) get_int(row);
	uint32 v1 = v2 / 1000;
	uint32 v0 = v1 / 1000;
	v1 %= 1000;
	v2 %= 1000;
	char date[64];
	sprintf(date, "v %d.%d.%d", v0, v1, v2);
	lbl.set_text(date);
	gui_file_table_label_column_t::paint_cell(offset, x, y, row);
}


sint32 gui_file_table_exp_column_t::get_int(const gui_table_row_t &row) const
{
	// file version 
 	gui_loadsave_table_row_t &file_row = (gui_loadsave_table_row_t&)row;
	return (sint32) file_row.file.get_experimental_version();
}


void gui_file_table_exp_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) {
	uint32 v3 = get_int(row);
	char date[64];
	if (v3)
	{
		sprintf(date, "e %d", v3);
	}
	else
	{
		date[0] = 0;
	}
	lbl.set_text(date);
	gui_file_table_label_column_t::paint_cell(offset, x, y, row);
}

const char *loadsave_frame_t::get_info(const char *fname)
{
	static char date[1024];
	date[0] = 0;
	const char *pak_extension = NULL;
	// get file information
	char path[1024];
	sprintf( path, SAVE_PATH_X "%s", fname );
	struct stat  sb;
	if(stat(path, &sb)!=0) {
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
		test.rd_open(path);
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
	file.wr_open(cache_file, loadsave_t::xml, "cache", SAVEGAME_VER_NR, EXPERIMENTAL_VER_NR);
	const char *text="Automatically generated file. Do not edit. An invalid file may crash the game. Deleting is allowed though.";
	file.rdwr_str(text);
	stringhashtable_iterator_tpl<sve_info_t *> iterator(cached_info);
	while(  iterator.next()  ) {
		// save only existing files
		if (iterator.get_current_value()->file_exists) {
			xml_tag_t t(&file, "save_game_info");
			const char *filename = iterator.get_current_key();
			file.rdwr_str(filename);
			iterator.access_current_value()->rdwr(&file);
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
