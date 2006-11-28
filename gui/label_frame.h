/*
 * savegame_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef gui_savegame_frame_h
#define gui_savegame_frame_h


#include "../tpl/slist_tpl.h"
#include "components/action_listener.h"
#include "gui_frame.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"


class karte_t;
class spieler_t;

class label_frame_t : public gui_frame_t, action_listener_t
{
	slist_tpl <button_t *> buttons;
	static label_frame_t *instance;

	char txlabel[64];

	char ibuf[64];
	gui_textinput_t input;
	gui_divider_t divider1;
	button_t savebutton;
	button_t cancelbutton;
	button_t removebutton;
	gui_label_t fnlabel;

	spieler_t *sp;
	karte_t *welt;
	koord pos;

protected:
	void remove_label();
	void load_label(char *name);
	void create_label(const char *name);
	void goto_label(const char *name);

public:
	/**
	 * Konstruktor.
	 * @author V. Meyer
	 */
	label_frame_t(karte_t *welt, spieler_t *sp, koord pos);

	~label_frame_t();

	 /**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Markus Weber
	 */
	void zeichnen(koord pos, koord gr);

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
