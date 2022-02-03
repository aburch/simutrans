/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LOADSOUNDFONT_FRAME_H
#define GUI_LOADSOUNDFONT_FRAME_H


#include "simwin.h"
#include "savegame_frame.h"

#include "components/action_listener.h"
#include "components/gui_button.h"

#include "../tpl/stringhashtable_tpl.h"
#include <string>


class loadsoundfont_frame_t : public savegame_frame_t
{
protected:
	static std::string old_soundfontname;

	/**
	 * Action that's started with a button click
	 */
	bool item_action (const char *filename) OVERRIDE;
	bool ok_action   (const char *fullpath) OVERRIDE;
	bool cancel_action(const char *) OVERRIDE;

	// returns extra file info
	const char *get_info(const char *fname) OVERRIDE;

	// sort with respect to info, which is date
	bool compare_items ( const dir_entry_t & entry, const char *info, const char *) OVERRIDE;

	bool check_file( const char *filename, const char *suffix ) OVERRIDE;

	void fill_list() OVERRIDE;

public:
	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	*/
	const char *get_help_filename() const OVERRIDE { return "load_soundfont.txt"; }

	loadsoundfont_frame_t();

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	uint32 get_rdwr_id( void ) OVERRIDE { return magic_soundfont; }
	void rdwr( loadsave_t *file ) OVERRIDE;

	bool action_triggered(gui_action_creator_t *, value_t v) OVERRIDE;
};

#endif
