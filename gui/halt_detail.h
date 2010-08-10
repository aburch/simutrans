/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_halt_detail_h
#define gui_halt_detail_h

#include "components/gui_textarea.h"

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"

#include "../halthandle_t.h"
#include "../utils/cbuffer_t.h"

class spieler_t;


/**
 * Dies stellt ein Fenster mit den Zielinformationen
 * fuer eine Haltestelle dar.
 *
 * @author Hj. Malthaner
 */

class halt_detail_t : public gui_frame_t, action_listener_t
{
private:
	halthandle_t halt;
	uint8 destination_counter;	// last destination counter of the halt; if mismatch to current, then redraw destinations
	spieler_t *cached_active_player; // So that, if different from current, change line links
	uint32 cached_line_count;

	gui_container_t cont;
	gui_scrollpane_t scrolly;
	gui_textarea_t txt_info;

	cbuffer_t cb_info_buffer;

	slist_tpl<button_t *>posbuttons;
	slist_tpl<gui_label_t *>linelabels;
	slist_tpl<button_t *>linebuttons;
	slist_tpl<gui_label_t *> convoylabels;
	slist_tpl<button_t *> convoybuttons;

public:
	halt_detail_t(halthandle_t halt);

	~halt_detail_t();

	void halt_detail_info(cbuffer_t & buf);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const { return "station_details.txt"; }

	// callback for posbuttons
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	// only defined to update schedule, if changed
	void zeichnen( koord pos, koord gr );
};

#endif
