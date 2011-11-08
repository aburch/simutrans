#ifndef ai_option_h
#define ai_option_h

#include "../simmesg.h"

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_numberinput.h"
#include "../utils/cbuffer_t.h"

class spieler_t;
class ai_t;

class ai_option_t : public gui_frame_t, private action_listener_t
{
private:
	button_t buttons[4];
	gui_label_t label_cs;
	gui_numberinput_t construction_speed;
	ai_t *ai;

public:
	ai_option_t(spieler_t *sp);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "players.txt";}

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
