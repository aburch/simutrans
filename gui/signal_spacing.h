/*
 * Dialogue to set the signal spacing, when CTRL+clicking a signal on toolbar
 * Used by wkz_roadsign_t
 */

#ifndef signal_spacing_h
#define signal_spacing_h

#include "gui_frame.h"
#include "components/action_listener.h"

class gui_numberinput_t;
class button_t;
class gui_label_t;
class wkz_roadsign_t;
class spieler_t;

class signal_spacing_frame_t : public gui_frame_t, private action_listener_t
{
	private:
		wkz_roadsign_t* tool;
		gui_numberinput_t signal_spacing_inp;
		static uint8 signal_spacing;
		gui_label_t signal_label;
		button_t remove_button, replace_button;
		static bool remove, replace;
		spieler_t *sp;
	public:
		signal_spacing_frame_t( spieler_t *, wkz_roadsign_t * );

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	const char * get_hilfe_datei() const {return "signal_spacing.txt";}
};

#endif
