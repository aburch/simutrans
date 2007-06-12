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
    button_t bt_lang;
    button_t bt_color;
    button_t bt_display;
    button_t bt_sound;
    button_t bt_player;
    button_t bt_load;
    button_t bt_save;
    button_t bt_new;
    button_t bt_quit;

	gui_divider_t seperator;
	gui_label_t txt;

	karte_t *welt;

public:
    optionen_gui_t(karte_t *welt);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    const char * gib_hilfe_datei() const {return "options.txt";}

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif
