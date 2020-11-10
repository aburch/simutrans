/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_PAKSELECTOR_H
#define GUI_PAKSELECTOR_H


#include "savegame_frame.h"


class pakselector_t : public savegame_frame_t
{
private:
	// unused button_t load_addons;
	//bool at_least_one_add;
	gui_file_table_action_column_t action_column;
	gui_file_table_delete_column_t addon_column;

protected:
	virtual bool item_action(const char *fullpath) OVERRIDE;
	virtual bool del_action(const char *fullpath) OVERRIDE;
	virtual const char *get_info(const char *fname) OVERRIDE;

	// true, if valid
	virtual bool check_file( const char *filename, const char *suffix ) OVERRIDE;

	virtual void init(const char *suffix, const char *path) OVERRIDE;
	virtual void add_file(const char *fullpath, const char *filename, const bool not_cutting_suffix) OVERRIDE;

public:
	void fill_list() OVERRIDE;	// do the search ...
	virtual bool has_title() const OVERRIDE { return false; }
	bool has_pak() const { return use_table ? file_table.get_size().h > 0 : !entries.empty(); }

	// If there is only one option, this will set the pak name and return true.
	// Otherwise it will return false.  (Note, it's const but it modifies global data.)
	bool check_only_one_option() const;
	const char * get_help_filename() const OVERRIDE { return ""; }
	// since we only want to see the frames ...
	void draw(scr_coord pos, scr_size gr) OVERRIDE;
	void set_windowsize(scr_size size) OVERRIDE;
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
	pakselector_t();
};

#endif
