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
#else
#include <io.h>
#include <direct.h>
#endif
#include <sys/stat.h>
#include <time.h>

#include "loadsave_frame.h"

#include "../simworld.h"
#include "../dataobj/loadsave.h"
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
		welt->speichern(filename,false);
		welt->set_dirty();
		welt->reset_timer();
	}
}

bool loadsave_frame_t::del_action(const char *filename)
{
	remove(filename);
	return false;
}


loadsave_frame_t::loadsave_frame_t(karte_t *welt, bool do_load) : savegame_frame_t(".sve", NULL, true)
{
	this->welt = welt;
	this->do_load = do_load;
	init(".sve", NULL);
	if(do_load) {
		set_name("Laden");
	}
	else {
		set_filename(welt->get_einstellungen()->get_filename());
		set_name("Speichern");
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


void loadsave_frame_t::init(const char *suffix, const char *path )
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


gui_file_table_pak_column_t::gui_file_table_pak_column_t() : gui_file_table_label_column_t() 
{
	set_width(150);
	strcpy(pak, umgebung_t::objfilename);
	pak[strlen(pak) - 1] = 0;
	strlwr(pak);
}

/**
 * Get a rate for the similarity of strings.
 *
 * The similar the strings, the lesser the result.
 * Identical strings result to 0.
 * The result rates the number of identical characters.
 */
float strsim(const char a[], const char b[]) 
{	
	int i = 0;
	while (a[i] && b[i] && a[i] == b[i]) i++;
	if (a[i] == b[i])
		return 0;
	return 1.0f / (float)(i+1);
}


int gui_file_table_pak_column_t::compare_rows(const gui_table_row_t &row1, const gui_table_row_t &row2) const
{
	char s1[1024];
	strcpy(s1, get_text(row1));
	strlwr(s1);
	float f1 = strsim(s1, pak);
	char s2[1024];
	strcpy(s2, get_text(row2));
	strlwr(s2);
	float f2 = strsim(s2, pak);
	int result = sgn(f1 - f2);
	if (!result)
		result = strcmp(s1, s2);
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
