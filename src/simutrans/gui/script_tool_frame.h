/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCRIPT_TOOL_FRAME_H
#define GUI_SCRIPT_TOOL_FRAME_H


#include "savegame_frame.h"
#include "../tpl/vector_tpl.h"

struct scripted_tool_info_t;


class script_tool_frame_t : public savegame_frame_t
{
private:
	// pointers to info structures
	vector_tpl<const scripted_tool_info_t*> infos;

protected:
	/**
	 * Action that's started by the press of a button.
	 */
	bool item_action(const char *fullpath) OVERRIDE;

	/**
	 * Returns extra file info: title of tool from description.tab
	 */
	const char *get_info(const char *path) OVERRIDE;

	// true, if valid
	bool check_file( const char *filename, const char *suffix ) OVERRIDE;

public:
	script_tool_frame_t();

	~script_tool_frame_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char * get_help_filename() const OVERRIDE { return "script_tool.txt"; }
};

#endif
