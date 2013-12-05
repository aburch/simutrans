/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_savegame_frame_h
#define gui_savegame_frame_h

#include <string>

#include "gui_frame.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "components/action_listener.h"
#include "components/gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"



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
	};

	// Standard GUI controls in dialogue
	gui_textinput_t  input;         //@< Filename input field
	gui_divider_t    divider1;      //@< Filename input field   (Added 30-Oct-2001 Markus Weber)
	button_t         savebutton;    //@< Save button            (Added 29-Oct-2001 Markus Weber)
	button_t         cancelbutton;  //@< Cancel button          (Added 29-Oct-2001 Markus Weber)
	gui_label_t      fnlabel;       //@< Static file name label (Added 31-Oct-2001 Markus Weber)
	gui_container_t  button_frame;  //@< Gui container for all items
	gui_scrollpane_t scrolly;       //@< Scroll panel for the GUI container

	slist_tpl<dir_entry_t> entries;  //@< Internal list representing the file listing

	uint32           num_sections;   //@< Internal counter representing the number of sections added to the list
	bool             delete_enabled; //@< Show the first column of delete buttons.
	bool             label_enabled;  //@< Show the third column of labels.

	void        add_file     ( const char *path, const char *filename, const char *pak, const bool no_cutting_suffix );
	void        add_path     ( const char *path );
	void        set_filename ( const char *file_name );
	void        cleanup_path ( char *path );
	void        shorten_path ( char *dest, const char *source, const size_t max_size );
	std::string get_filename ( const char *fullpath, const bool with_extension = true );
	std::string get_basename ( const char *fullpath );
	void        list_filled  ( void );

	 // Virtual callback function that will be executed when the user clicks ok,
	virtual bool cancel_action ( const char * /*fullpath*/ ) { return true; } // Callback for cancel button click
	virtual bool del_action    ( const char *   fullpath   );                 // Callback for delete button click
	virtual bool ok_action     ( const char * /*fullpath*/ ) { return true; } // Callback for ok button click

	virtual void set_windowsize     ( scr_size size );
	virtual bool check_file         ( const char *filename, const char *suffix );

	// Pure virtual functions
	virtual const char *get_info    ( const char *fname    ) = 0;
	virtual bool        item_action ( const char *fullpath ) = 0;

public:

	savegame_frame_t(const char *suffix, bool only_directorie, const char *path, const bool delete_enabled );
//	savegame_frame_t(const char *suffix = NULL, bool only_directories = false, const char *path = NULL, const bool delete_enabled = true);
	virtual ~savegame_frame_t();

	bool action_triggered  ( gui_action_creator_t*, value_t ) OVERRIDE;
	bool infowin_event     ( event_t const* ) OVERRIDE;

	virtual void fill_list ( void );
};

#endif
