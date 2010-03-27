/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_loadsave_frame_h
#define gui_loadsave_frame_h


#include "savegame_frame.h"

class karte_t;

class gui_file_table_pak_column_t : public gui_file_table_label_column_t
{
public:
	gui_file_table_pak_column_t() : gui_file_table_label_column_t() {
		set_width(150);
	}
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, gui_table_row_t &row);
};

class gui_file_table_std_column_t : public gui_file_table_label_column_t
{
public:
	gui_file_table_std_column_t() : gui_file_table_label_column_t() {
		set_width(70);
	}
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, gui_table_row_t &row);
};

class gui_file_table_exp_column_t : public gui_file_table_label_column_t
{
public:
	gui_file_table_exp_column_t() : gui_file_table_label_column_t() {
		set_width(20);
	}
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, gui_table_row_t &row);
};


class loadsave_frame_t : public savegame_frame_t
{
private:
	karte_t *welt;
	gui_file_table_delete_column_t delete_column;
	gui_file_table_action_column_t action_column;
	gui_file_table_date_column_t date_column;
	gui_file_table_pak_column_t pak_column;
	gui_file_table_std_column_t std_column;
	gui_file_table_exp_column_t exp_column;
	bool do_load;

protected:
	virtual void init(const char *suffix, const char *path);

	/**
	 * Aktion, die nach Knopfdruck gestartet wird.
	 * @author Hansjörg Malthaner
	 */
	virtual void action(const char *filename);

	/**
	 * Aktion, die nach X-Knopfdruck gestartet wird.
	 * @author V. Meyer
	 */
	virtual bool del_action(const char *filename);

	// returns extra file info
	virtual const char *get_info(const char *fname) { return ""; };
	virtual void add_file(const char *filename, const bool not_cutting_suffix);

public:
	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	virtual const char * get_hilfe_datei() const;

	loadsave_frame_t(karte_t *welt, bool do_load);
};

#endif
