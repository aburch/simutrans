/*
 * Copyright (c) 1997 - 2001 Hansjoerg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_savegame_frame_h
#define gui_savegame_frame_h

#include <string>

#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "components/action_listener.h"
#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"

/**
 * Base class from wich all GUI dialogs to load/save generics can inherit from
 * @author Hansjoerg Malthaner
 * @author Makohs
 * @note Based on previous original work from the simutrans team and Hansjoerg Malthaner
 * @note When I refer to a "qualified" path I mean it can contain sub-directories or even fully qualified path. i.e. : "save/a.sve" or "c:\simutrans\scenario\file.nut"
 */
class savegame_frame_t : public gui_frame_t, action_listener_t
{
private:

	/**
	 * To avoid double mouse action
	 */
	bool in_action;

	/**
	 * Paths in wich this dialog will search for
	 */
	vector_tpl<std::string> paths;

	/**
	 * Input buffer for the text input component
	 */
	char ibuf[1024];

	/**
	 * Is default path defined?
	 */
	bool searchpath_defined;

	/**
	 * Default search path
	 */
	char searchpath[1024];

	/**
	 * Adds a section entry to the list
	 */
	void add_section(std::string &name);

	/**
	 * Extension of the files this dialog will use, can be NULL
	 * Can include or not the "." at start, will work on both cases
	 */
	const char *suffix;

	/**
	 * Search for directories (used in pak_selector)
	 */
	bool only_directories;

	/**
	 * Enable delete buttons
	 */
	bool delete_enabled;

protected:

	gui_textinput_t input;
	gui_divider_t divider1;                               // 30-Oct-2001  Markus Weber    Added
	button_t savebutton;                                  // 29-Oct-2001  Markus Weber    Added
	button_t cancelbutton;                                // 29-Oct-2001  Markus Weber    Added
	gui_label_t fnlabel;        //filename                // 31-Oct-2001  Markus Weber    Added
	gui_container_t button_frame;
	gui_scrollpane_t scrolly;

	/**
	 * Entries in list can be actual file entries or headers, that have a diferent look
	 */
	enum dirlist_item_t {LI_HEADER,LI_ENTRY};

	struct dir_entry_t
	{
		dir_entry_t(button_t* button_, button_t* del_, gui_label_t* label_, dirlist_item_t type_ = LI_ENTRY, const char *info_=NULL) :
			button(button_),
			del(del_),
			label(label_),
			type(type_),
			info(info_)
		{}

		button_t*      button;
		button_t*      del;
		gui_label_t*   label;
		dirlist_item_t type;

		/**
		* Contains a qualified path (might be relative) to the file, not just the name
		*/
		const char *info;
	};

	/**
	 * Returns extra file info
	 * @note filename is a qualified path
	 */
	virtual const char *get_info(const char *fname) = 0;

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

	/**
	 * Adds a directory to search in
	 */
	void add_path(const char *path);

	/**
	 * Called when the directory processing ends
	 */
	void list_filled();

	/**
	 * Internal list representing the file listing
	 */
	slist_tpl<dir_entry_t> entries;

	/**
	 * Internal counter representing the number of sections added to the list
	 */
	uint32 num_sections;

	/**
	 * Callback function that will be executed when the user clicks one of the entries in list
	 * @author Hansjörg Malthaner
	 */
	virtual void action(const char *fullpath) = 0;

	/**
	 * Will be executed when the user presses the delete 'X' button.
	 * @return If true, then the dialogue will be closed
	 * @note on the pakselector, this action is re-used to load addons
	 * @author Volker Meyer
	 */
	virtual bool del_action(const char *fullpath);

	/**
	 * Sets the filename in the edit field
	 */
	void set_filename(const char *fn);

	/**
	 * Translates '/' into '\', in Windows systems
	 */
	void cleanup_path(char *path);

	/**
	 * extracts file name from a full path
	 */
	std::string get_filename(const char *fullpath, const bool with_extension = true);

	/**
	 * extracts base name from a full path
	 */
	std::string get_basename(const char *fullpath);

	/**
	 * Sets the gui components widths and coordinates
	 * @author Mathew Hounsell
	 * \date   11-Mar-2003
	 */
	virtual void set_fenstergroesse(koord groesse);

public:
	/**
	 * @param suffix Filename suffix, i.e. ".sve", you can omit the intial dot, i.e. "sve", NULL to not enforce any extension.
	 * @param only_directories will just process directory entries, not files.
	 * @param path Default path to search at. If NULL, next call to add_path will define the default path.
	 * @param delete_enabled Determins if we'll add delete buttons to the frame.
	 * @author Hj. Malthaner
	 */
	savegame_frame_t(const char *suffix = NULL, bool only_directories = false, const char *path = NULL, const bool delete_enabled = true);

	virtual ~savegame_frame_t();

	/**
	 * Inherited from action_listener_t
	 */
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Must catch open message to update list, since I am using virtual functions
	 */
	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Start the directory processing
	 */
	void fill_list();
};

#endif
