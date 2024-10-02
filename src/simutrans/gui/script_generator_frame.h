/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCRIPT_GENERATOR_INFO_H
#define GUI_SCRIPT_GENERATOR_INFO_H

#include "savegame_frame.h"
#include "../utils/cbuffer.h"
#include "../tool/simtool.h"


class script_generator_frame_t : public savegame_frame_t
{
private:
	tool_generate_script_t* tool;
	cbuffer_t command;
	
protected:
	/**
	 * Action that's started by the press of a button.
	 */
	bool item_action(const char *fullpath) OVERRIDE;
	
	bool ok_action(const char *fullpath) OVERRIDE;

	/**
	 * Returns extra file info: title of tool from description.tab
	 */
	const char *get_info(const char *path) OVERRIDE;

	// true, if valid
	bool check_file( const char *filename, const char *suffix ) OVERRIDE;

public:
	script_generator_frame_t(tool_generate_script_t*, const char *path, cbuffer_t &cmd);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE { return "script_generator.txt"; }
};

#endif
