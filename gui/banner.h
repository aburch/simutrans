/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_BANNER_H
#define GUI_BANNER_H


#include "components/gui_button.h"
#include "components/gui_image.h"
#include "gui_frame.h"


/*
 * Class to generates the welcome screen with the scrolling
 * text to celebrate contributors.
 */
class banner_t : public gui_frame_t, action_listener_t
{
private:
	sint32 last_ms;
	int line;

	button_t
		new_map,
		load_map,
		load_scenario,
		join_map,
		quit;

	gui_image_t logo;


public:
	banner_t();

	bool has_sticky() const OVERRIDE { return false; }

	virtual bool has_title() const OVERRIDE { return false; }

	/**
	* Window Title
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return ""; }

	/**
	* get color information for the window title
	* -borders and -body background
	* @author Hj. Malthaner
	*/
	PLAYER_COLOR_VAL get_titlecolor() const OVERRIDE {return WIN_TITLE; }

	bool is_hit(int, int) OVERRIDE { return true; }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	* @author Hj. Malthaner
	*/
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
