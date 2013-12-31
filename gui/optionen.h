/*
 * Dialog for game options/Main menu
 * Niels Roest, Hj. Malthaner, 2000
 */

#ifndef gui_optionen_h
#define gui_optionen_h

/**
 * Settings in the game
 * @author Hj. Malthaner
 */

class gui_frame_t;
class action_listener_t;
class gui_divider_t;
class button_t;
class gui_action_creator_t;

class optionen_gui_t : public gui_frame_t, action_listener_t
{
	private:
		gui_divider_t divider;
		button_t      option_buttons[11];

	public:
		optionen_gui_t();

		 /**
		 * Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 * @author Hj. Malthaner
		 */
		const char * get_hilfe_datei() const {return "options.txt";}

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
