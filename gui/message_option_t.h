#ifndef message_option_h
#define message_option_h

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_textarea.h"
#include "components/gui_image.h"

class karte_t;

#define MAX_MESSAGE_TYP (10)

class message_option_t : public gui_frame_t, private action_listener_t
{
private:
    gui_textarea_t text_label;
    button_t buttons[3*MAX_MESSAGE_TYP];
    gui_image_t legend;
    sint32 ticker_msg, window_msg, auto_msg, ignore_msg;
	static karte_t *welt;

public:
	message_option_t(karte_t *welt);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen für die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    const char * get_hilfe_datei() const {return "mailbox.txt";}

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
