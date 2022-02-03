/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HELP_FRAME_H
#define GUI_HELP_FRAME_H


#include <string>

#include "gui_frame.h"
#include "components/gui_flowtext.h"
#include "components/action_listener.h"

class cbuffer_t;

class help_frame_t : public gui_frame_t, action_listener_t
{
private:
	enum {
		missing,
		native,
		english
	};

	gui_flowtext_t
		generaltext,
		helptext;

	std::string title;

	// show the help to one topic
	void set_helpfile(const char *filename, bool resize_frame );

	help_frame_t(char const* filename);
	void add_helpfile( cbuffer_t &section, const char *titlename, const char *filename, bool only_native, int indent_level );
	std::string extract_title( const char *htmllines );
	static FILE *has_helpfile( char const* const filename, int &mode );

public:
	help_frame_t();
	static void open_help_on( const char *helpfilename );
	void set_text(const char * text, bool resize = true );

	/**
	 * resize window in response to a resize event
	 */
	void resize(const scr_coord delta) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
