/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LOADSAVE_FRAME_H
#define GUI_LOADSAVE_FRAME_H


#include <time.h>

#include "savegame_frame.h"
#include "../tpl/stringhashtable_tpl.h"
#include <string>

class loadsave_t;

class sve_info_t {
public:
	std::string pak;
	sint64 mod_time;
	sint32 file_size;
	uint32 version;
	uint32 extended_version;
	uint32 extended_revision;
	bool file_exists;

	sve_info_t() :
		pak(""),
		mod_time(0),
		file_size(0),
		version(0),
		extended_version(0),
		extended_revision(0),
		file_exists(false)
	{}

	sve_info_t(const char *pak_, time_t mod_, sint32 fs, uint32 version, uint32 extended_version);
	bool operator== (const sve_info_t &) const;
	void rdwr(loadsave_t *file);
};

class loadsave_frame_t : public savegame_frame_t
{
	//friend class gui_loadsave_table_row_t;
private:
	gui_file_table_delete_column_t delete_column;
	gui_file_table_action_column_t action_column;
	gui_file_table_time_column_t date_column;
	bool do_load;

	button_t easy_server; // only active on loading savegames

	static stringhashtable_tpl<sve_info_t *, N_BAGS_LARGE> cached_info;

protected:
	/**
	 * Action that's started with a button click
	 */
	bool item_action (const char *filename) OVERRIDE;
	bool ok_action   (const char *fullpath) OVERRIDE;

	// returns extra file info
	const char *get_info(const char *fname) OVERRIDE;

	// FIXME: need incorporating r7664
	//// sort with respect to info, which is date
	////bool compare_items ( const dir_entry_t & entry, const char *info, const char *) OVERRIDE;

	// sort with respect to info, which is date
	virtual bool compare_items ( const dir_entry_t & entry, const char *info, const char *);

public:
	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	*/
	const char *get_help_filename() const OVERRIDE;

	loadsave_frame_t(bool do_load);

	/**
	 * save hashtable to xml file
	 */
	~loadsave_frame_t();
};

#endif
