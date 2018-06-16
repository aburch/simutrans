/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_scenario_frame_h
#define gui_scenario_frame_h


#include "savegame_frame.h"
#include "../utils/cbuffer_t.h"


class scenario_frame_t : public savegame_frame_t
{
private:
	cbuffer_t path;
	button_t easy_server;

protected:
	/**
	 * Action that's started by the press of a button.
	 * @author Hansjörg Malthaner
	 */
	virtual bool item_action(const char *fullpath);

	/**
	 * Action, started after X-Button pressing
	 * @author V. Meyer
	 */
	virtual bool del_action(const char *f) { return item_action(f); }

	// returns extra file info
	virtual const char *get_info(const char *fname);

	// true, if valid
	virtual bool check_file( const char *filename, const char *suffix );

	virtual void set_windowsize(scr_size size);

public:
	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	virtual const char * get_help_filename() const { return "scenario.txt"; }

	scenario_frame_t();
};

#endif
