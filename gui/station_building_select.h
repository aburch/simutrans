/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Maintains the list of custon labels assigned to different palces on the map
 */

#ifndef station_building_select_h
#define station_building_select_h

#include "components/action_listener.h"
#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"


class haus_besch_t;
class karte_t;
class wkz_station_t;

class station_building_select_t : public gui_frame_t, action_listener_t
{
	static karte_t *welt;
	const haus_besch_t *besch;

	char buf[64];
	button_t actionbutton[4];
	gui_textarea_t txt;
	gui_image_t img[16];

	static char default_str[260];
	static wkz_station_t wkz;

public:
	station_building_select_t(karte_t *welt, const haus_besch_t *besch);

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen für die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
//	const char * get_hilfe_datei() const { return ".txt"; }

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
