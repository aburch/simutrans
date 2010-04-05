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
#include "../pathes.h"
#include "../utils/simstring.h"

#include "components/gui_button.h"

class gui_loadsave_table_row_t : public gui_file_table_row_t
{
public:
	loadsave_t file;

	gui_loadsave_table_row_t() : gui_file_table_row_t() {};
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
	sprintf( pathname, SAVE_PATH_X "%s", filename );
	char buttontext[1024];
	strcpy( buttontext, filename );
	if ( !not_cutting_suffix ) {
		buttontext[strlen(buttontext)-4] = '\0';
	}
	file_table.add_row( new gui_loadsave_table_row_t( pathname, buttontext ));
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

void gui_file_table_pak_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, gui_table_row_t &row) {
 	gui_loadsave_table_row_t &file_row = (gui_loadsave_table_row_t&)row;
	lbl.set_text(file_row.file.get_pak_extension());
	gui_file_table_label_column_t::paint_cell(offset, x, y, row);
}

void gui_file_table_std_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, gui_table_row_t &row) {
 	gui_loadsave_table_row_t &file_row = (gui_loadsave_table_row_t&)row;
	// file version 
	uint32 v2 = file_row.file.get_version();
	uint32 v1 = v2 / 1000;
	uint32 v0 = v1 / 1000;
	v1 %= 1000;
	v2 %= 1000;
	char date[64];
	sprintf(date, "v %d.%d.%d", v0, v1, v2);
	lbl.set_text(date);
	gui_file_table_label_column_t::paint_cell(offset, x, y, row);
}

void gui_file_table_exp_column_t::paint_cell(const koord &offset, coordinate_t x, coordinate_t y, gui_table_row_t &row) {
 	gui_loadsave_table_row_t &file_row = (gui_loadsave_table_row_t&)row;
	uint32 v3 = file_row.file.get_experimental_version();
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
