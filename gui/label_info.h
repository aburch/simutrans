/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_label_info_h
#define gui_label_info_h

#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_label.h"
#include "components/gui_world_view_t.h"
#include "components/action_listener.h"

class label_t;

/**
 * Dies stellt ein Fenster mit den Zielinformationen
 * fuer eine Haltestelle dar.
 *
 * @author Hj. Malthaner
 */

class label_info_t : public gui_frame_t, private action_listener_t
{
private:
	static karte_t *welt;
	spieler_t *sp;
	label_t *label;

	gui_label_t player_name;
	gui_textinput_t input;
	world_view_t view;
	char edit_name[64];

public:
	label_info_t(karte_t *welt, label_t* l);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	// rotated map need new info ...
	void map_rotate90( sint16 );
};

#endif
