#ifndef gui_optionen_h
#define gui_optionen_h


#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_divider.h"
#include "components/action_listener.h"


/**
 * Settings in the game
 * @author Hj. Malthaner
 */
class optionen_gui_t : public gui_frame_t, action_listener_t
{
private:
	button_t option_buttons[6];

	button_t bt_load;
	button_t bt_load_scenario;
	button_t bt_save;

	button_t bt_new;
	button_t bt_quit;

	gui_divider_t seperator;

	karte_t *welt;

public:
    optionen_gui_t(karte_t *welt);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "options.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
