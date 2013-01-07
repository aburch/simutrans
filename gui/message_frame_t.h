/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef message_frame_h
#define message_frame_h

#include "../simwin.h"

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_textinput.h"

#include "message_stats_t.h"
#include "components/action_listener.h"

class karte_t;


/**
 * All messages since the start of the program
 * @author prissi
 */
class message_frame_t : public gui_frame_t, private action_listener_t
{
private:
	char ibuf[256];
	message_stats_t	stats;
	gui_scrollpane_t scrolly;
	gui_tab_panel_t tabs;		// Knightly : tab panel for filtering messages
	gui_textinput_t input;
	button_t option_bt, send_bt;
	static karte_t *welt;
	vector_tpl<sint32> tab_categories;

public:
	message_frame_t(karte_t * welt);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const {return "mailbox.txt";}

	/**
	* resize window in response to a resize event
	* @author Hj. Malthaner
	*/
	void resize(const koord delta);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr(loadsave_t *);

	uint32 get_rdwr_id() { return magic_messageframe; }
};

#endif
