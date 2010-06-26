/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_savegame_frame_h
#define gui_savegame_frame_h

#include <string.h>
#include <sys/stat.h>

#include "../tpl/slist_tpl.h"
#include "components/action_listener.h"
#include "components/gui_table.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "gui_frame.h"
#include "gui_container.h"

#include <string>

using std::string;


class gui_file_table_column_t : public gui_table_column_t
{
protected:
	bool pressed;
	virtual const char *get_text(const gui_table_row_t &row) const { (void) row; return ""; }
public:
	gui_file_table_column_t(coordinate_t size_) : gui_table_column_t(size_) { pressed = false; }
	virtual int compare_rows(const gui_table_row_t &row1, const gui_table_row_t &row2) const { return strcmp(get_text(row1), get_text(row2)); }
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) = 0;
	bool get_pressed() const { return pressed; }
	void set_pressed(bool value) { pressed = value; }
};


class gui_file_table_button_column_t : public gui_file_table_column_t
{
protected:
	button_t btn;
public:
	gui_file_table_button_column_t(coordinate_t size_) : gui_file_table_column_t(size_) {}
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
	void set_text(char *text) { btn.set_text(text); }
	void set_tooltip(char *tooltip) { btn.set_tooltip(tooltip); }
};


class gui_file_table_label_column_t : public gui_file_table_column_t
{
protected:
	gui_label_t lbl;
public:
	gui_file_table_label_column_t(coordinate_t size_) : gui_file_table_column_t(size_) {}
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
};


class gui_file_table_delete_column_t : public gui_file_table_button_column_t
{
public:
	gui_file_table_delete_column_t() : gui_file_table_button_column_t(14) {
		btn.set_text("X");
		btn.set_tooltip("Delete this file.");
	}
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
};


class gui_file_table_action_column_t : public gui_file_table_button_column_t
{
protected:
	virtual const char *get_text(const gui_table_row_t &row) const;
public:
	gui_file_table_action_column_t() : gui_file_table_button_column_t(300) {
		btn.set_no_translate(true);
	}
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
};


class gui_file_table_time_column_t : public gui_file_table_label_column_t
{
protected:
	virtual time_t get_time(const gui_table_row_t &row) const;
public:
	gui_file_table_time_column_t() : gui_file_table_label_column_t(120) {
		set_sort_descendingly(true);
	}
	virtual int compare_rows(const gui_table_row_t &row1, const gui_table_row_t &row2) const { return sgn(get_time(row1) - get_time(row2)); }
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
};


class gui_file_table_row_t : public gui_table_row_t
{
	friend class gui_file_table_button_column_t;
	friend class gui_file_table_delete_column_t;
	friend class gui_file_table_action_column_t;
	friend class gui_file_table_time_column_t;
protected:
	string text;
	string name;
	string error;
	bool pressed;
	bool delete_enabled;
	struct stat info;
public:
	//gui_file_table_row_t();
	gui_file_table_row_t(const char *pathname, const char *buttontext, bool delete_enabled = true);
	const char *get_name() const { return name.c_str(); }
	void set_pressed(bool value) { pressed = value; }
	bool get_pressed() { return pressed; }
	void set_delete_enabled(bool value) { delete_enabled = value; }
	bool get_delete_enabled() { return delete_enabled; }
};


class gui_file_table_t : public	gui_table_t
{
protected:
	virtual void paint_cell(const koord &offset, coordinate_t x, coordinate_t y);	
};


class savegame_frame_t : public gui_frame_t, action_listener_t
{
private:
	char ibuf[1024];

	/**
	 * Filename suffix, i.e. ".sve", must be four characters
	 * @author Hj. Malthaner
	 */
	const char *suffix;

	// path, to be put in front
	const char *fullpath;

	// true, if there is additional information, i.e. loading a game
	bool use_pak_extension;

	// to avoid double mouse action
	bool in_action;

	bool file_table_button_pressed;
	coordinates_t pressed_file_table_button;

	void press_file_table_button(coordinates_t &cell);
	void release_file_table_button();	
protected:
	gui_textinput_t input;
	gui_divider_t divider1;                               // 30-Oct-2001  Markus Weber    Added
	button_t savebutton;                                  // 29-Oct-2001  Markus Weber    Added
	button_t cancelbutton;                               // 29-Oct-2001  Markus Weber    Added
	gui_label_t fnlabel;        //filename                // 31-Oct-2001  Markus Weber    Added
	gui_file_table_t file_table;
	gui_container_t button_frame;
	struct entry
	{
		entry(button_t* button_, button_t* del_, gui_label_t* label_) :
			button(button_),
			del(del_),
			label(label_)
		{}

		button_t*    button;
		button_t*    del;
		gui_label_t* label;
	};
	slist_tpl<entry> entries;
	gui_scrollpane_t scrolly;
	// use file_table instead of button_frame:
	bool use_table;

	virtual void add_file(const char *filename, const bool not_cutting_suffix);

	void add_file(const char *filename, const char *pak, const bool no_cutting_suffix );

	/**
	 * Aktion, die nach Knopfdruck gestartet wird.
	 * @author Hansjörg Malthaner
	 */
	virtual void action(const char *filename) = 0;

	/**
	 * Aktion, die nach X-Knopfdruck gestartet wird.
	 * if true, then the dialogue will be closed
	 * @author Volker Meyer
	 */
	virtual bool del_action(const char *filename) = 0;

	// returns extra file info
	virtual const char *get_info(const char *fname) = 0;

	// true, if valid
	virtual bool check_file( const char *filename, const char *suffix);

	// sets the filename in the edit field
	void set_filename( const char *fn );

	virtual void init(const char *suffix, const char *path);

	/**
	 * called by fill_list():
	 */
	virtual void set_file_table_default_sort_order();
public:
	const char *get_path() const { return fullpath; }
	void fill_list();	// do the search ...

	/**
	 * @param suffix Filename suffix, i.e. ".sve", must be four characters
	 * @author Hj. Malthaner
	 */
	savegame_frame_t(const char *suffix, const char *path );
	savegame_frame_t(const char *suffix, const char *path, bool use_table );

	virtual ~savegame_frame_t();

	/**
	 * Setzt die Fenstergroesse
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	virtual void set_fenstergroesse(koord groesse);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	virtual bool action_triggered( gui_action_creator_t *komp, value_t extra);

	// must catch open message to update list, since I am using virtual functions
	virtual bool infowin_event(const event_t *ev);
};

#endif
