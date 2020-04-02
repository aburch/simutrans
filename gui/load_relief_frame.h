/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LOAD_RELIEF_FRAME_H
#define GUI_LOAD_RELIEF_FRAME_H


#include "savegame_frame.h"


class settings_t;

class load_relief_frame_t : public savegame_frame_t
{
private:
	settings_t* sets;

	button_t new_format;

protected:
	virtual bool item_action(const char *fullpath);
	virtual const char *get_info(const char *fullpath);
	virtual bool check_file(const char *fullpath, const char *suffix);

public:
	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_help_filename() const { return "load_relief.txt"; }

	load_relief_frame_t(settings_t*);
};

#endif
