/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_savegame_frame_h
#define gui_savegame_frame_h

#include <string.h>
#include <cstring>
#include <sys/stat.h>

#include "gui_frame.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "components/action_listener.h"
#include "components/gui_table.h"
#include "components/gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"

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
	virtual void paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row) = 0;
	bool get_pressed() const { return pressed; }
	void set_pressed(bool value) { pressed = value; }
};


class gui_file_table_button_column_t : public gui_file_table_column_t
{
protected:
	button_t btn;
public:
	gui_file_table_button_column_t(coordinate_t size_) : gui_file_table_column_t(size_) {}
	virtual void paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
	void set_text(const char *text) { btn.set_text(text); }
	void set_tooltip(const char *tooltip) { btn.set_tooltip(tooltip); }
};


class gui_file_table_label_column_t : public gui_file_table_column_t
{
protected:
	gui_label_t lbl;
public:
	gui_file_table_label_column_t(coordinate_t size_) : gui_file_table_column_t(size_) {}
	virtual void paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
};


class gui_file_table_delete_column_t : public gui_file_table_button_column_t
{
public:
	gui_file_table_delete_column_t() : gui_file_table_button_column_t(14) {
		btn.set_text("X");
		btn.set_tooltip("Delete this file.");
	}
	virtual void paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
};


class gui_file_table_action_column_t : public gui_file_table_button_column_t
{
protected:
	virtual const char *get_text(const gui_table_row_t &row) const;
public:
	gui_file_table_action_column_t() : gui_file_table_button_column_t(300) {
		btn.set_no_translate(true);
	}
	virtual void paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
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
	virtual void paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y, const gui_table_row_t &row);
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
	virtual void paint_cell(const scr_coord& offset, coordinate_t x, coordinate_t y);	
};



/**
 * Base class from which all GUI dialogs to load/save generics can inherit from
 * @author Hansjoerg Malthaner
 * @author Markohs
 * @author Max Kielland
 * @note Based on previous original work from the Simutrans team and Hansjoerg Malthaner
 * @note When I refer to a "qualified" path I mean it can contain sub-directories or even fully qualified path. i.e. : "save/a.sve" or "c:\simutrans\scenario\file.nut"
 */
class savegame_frame_t : public gui_frame_t, action_listener_t
{
private:

	vector_tpl<std::string> paths;     //@< Paths in which this dialog will search for

	const char *suffix;                //@< Extension of the files this dialog will use, can be NULL Can include or not the "." at start, will work on both cases
	char        ibuf[1024];            //@< Input buffer for the text input component
	char        searchpath[1024];      //@< Default search path
	bool        in_action;             //@< To avoid double mouse action
	bool        only_directories;      //@< Search for directories (used in pak_selector)
	bool        searchpath_defined;    //@< Is default path defined?

	void add_section(std::string &name);

protected:

	/**
	 * Entries in list can be actual file entries or
	 * headers, that have a different look.
	 */
	enum dirlist_item_t {
		LI_HEADER, //@< This is a header list item.
		LI_ENTRY   //@< This is a data list item.
	};


	/**
	 * A list item.
	 * Max Kielland: Shouldn't this be an ADT and then have
	 * each derivate to define their own item class? This would also
	 * take care of differences, sorting and freeing resources.
	 */
	struct dir_entry_t
	{
		dir_entry_t(button_t* button_, button_t* del_, gui_label_t* label_, dirlist_item_t type_ = LI_ENTRY, const char *info_=NULL) :
			del(del_),
			button(button_),
			label(label_),
			type(type_),
			info(info_)
		{}

		button_t       *del;    //@< Delete button placed in the first column.
		button_t       *button; //@< Action button placed in the second column.
		gui_label_t    *label;  //@< Label placed in the third column.
		dirlist_item_t  type;   //@< Item type, data or header.
		const char     *info;   //@< A qualified path (might be relative) to the file, not just the name
		static int compare (const dir_entry_t & l, const dir_entry_t &r) {
			if (( NULL == l.info ) && ( NULL != r.info )) {
				return -1;
			}
			if (( NULL == r.info ) && ( NULL != l.info )) {
				return 1;
			}
			if (( NULL != l.info ) && ( NULL != r.info )) {
				return strcmp ( l.info, r.info );
			}
			return 0;
		}
	};

	// Standard GUI controls in dialogue
	gui_textinput_t  input;         //@< Filename input field
	gui_divider_t    divider1;      //@< Filename input field   (Added 30-Oct-2001 Markus Weber)
	button_t         savebutton;    //@< Save button            (Added 29-Oct-2001 Markus Weber)
	button_t         cancelbutton;  //@< Cancel button          (Added 29-Oct-2001 Markus Weber)
	gui_label_t      fnlabel;       //@< Static file name label (Added 31-Oct-2001 Markus Weber)
	gui_file_table_t file_table;
	gui_container_t  button_frame;  //@< Gui container for all items
	gui_scrollpane_t scrolly;       //@< Scroll panel for the GUI container

	slist_tpl<dir_entry_t> entries;  //@< Internal list representing the file listing

	uint32           num_sections;   //@< Internal counter representing the number of sections added to the list
	bool             delete_enabled; //@< Show the first column of delete buttons.
	bool             label_enabled;  //@< Show the third column of labels.

	bool file_table_button_pressed;
	coordinates_t pressed_file_table_button;

	void press_file_table_button(coordinates_t &cell);
	void release_file_table_button();	
	// use file_table instead of button_frame:
	bool use_table;

	/**
	 * Returns extra file info
	 * @note filename is a qualified path
	 */
	virtual const char *get_info(const char *fname) = 0;
	virtual bool        item_action ( const char *fullpath ) = 0;

	/**
	 * Called on each entry, to be re-implemented on each sub-class
	 * @return if true, the file will be added to the list
	 * @note filename is a qualified path
	 */
	virtual bool check_file(const char *filename, const char *suffix);

	/**
	 * Called on each entry that passed the check
	 */
	void add_file(const char *path, const char *filename, const char *pak, const bool no_cutting_suffix);
	virtual void add_file(const char *fullpath, const char *filename, const bool not_cutting_suffix);

	/**
	 * Adds a directory to search in
	 */
	void add_path(const char *path);

	/**
	 * Called when the directory processing ends
	 */
	void list_filled();

	/**
	 * Callback function that will be executed when the user clicks one of the entries in list
	 * @author Hansjörg Malthaner
	 */

	 // Virtual callback function that will be executed when the user clicks ok,
	virtual bool cancel_action ( const char * /*fullpath*/ ) { return true; } // Callback for cancel button click
	virtual bool del_action    ( const char *   fullpath   );                 // Callback for delete button click
	virtual bool ok_action     ( const char * /*fullpath*/ ) { return true; } // Callback for ok button click

	/**
	 * Sets the filename in the edit field
	 */
	void set_filename(const char *fn);

	/**
	 * Translates '/' into '\', in Windows systems, will capitalize the drive letter too.
	 */
	void cleanup_path(char *path);

	/**
	 * Outputs a shortened path removing characters in the middle of the input path, replacing them with "..."
	 * @param dest output will be written here
	 * @param orig input string
	 * @param max_size the string will be truncated to this length
	 */
	void shorten_path(char *dest, const char *orig, const size_t max_size);

	/**
	 * extracts file name from a full path
	 */
	std::string get_filename(const char *fullpath, const bool with_extension = true) const;

	virtual void init(const char *suffix, const char *path);

	/**
	 * called by fill_list():
	 */
	virtual void set_file_table_default_sort_order();

public:

	/**
	 * extracts base name from a full path
	 */
	std::string get_basename(const char *fullpath);

	/**
	 * Sets the gui components widths and coordinates
	 * @author Mathew Hounsell
	 * \date   11-Mar-2003
	 */
	void set_windowsize(scr_size size) OVERRIDE;

	/**
	 * @param suffix Filename suffix, i.e. ".sve", you can omit the intial dot, i.e. "sve", NULL to not enforce any extension.
	 * @param only_directories will just process directory entries, not files.
	 * @param path Default path to search at. If NULL, next call to add_path will define the default path.
	 * @param delete_enabled Determins if we'll add delete buttons to the frame.
	 * @author Hj. Malthaner
	 */
	savegame_frame_t(const char *suffix = NULL, bool only_directories = false, const char *path = NULL, const bool delete_enabled = true, bool use_table = false );

//	savegame_frame_t(const char *suffix, bool only_directorie, const char *path, const bool delete_enabled );
//	savegame_frame_t(const char *suffix = NULL, bool only_directories = false, const char *path = NULL, const bool delete_enabled = true);
	virtual ~savegame_frame_t();

	bool action_triggered  ( gui_action_creator_t*, value_t ) OVERRIDE;
	bool infowin_event     ( event_t const* ) OVERRIDE;

	virtual void fill_list ( void );
};

#endif
