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
#include "../gui/simwin.h"

class spieler_t;


/**
 * Window with destination information for a stop
 * @author Hj. Malthaner
 */

class halt_detail_t : public gui_frame_t, action_listener_t
{
private:
	halthandle_t halt;
	uint8 destination_counter;	// last destination counter of the halt; if mismatch to current, then redraw destinations
	spieler_t *cached_active_player; // So that, if different from current, change line links
	uint32 cached_line_count;
	uint32 cached_convoy_count;

	cbuffer_t buf;

	gui_container_t cont;
	gui_scrollpane_t scrolly;
	gui_textarea_t txt_info;

	slist_tpl<button_t *>posbuttons;
	slist_tpl<gui_label_t *>linelabels;
	slist_tpl<button_t *>linebuttons;
	slist_tpl<gui_label_t *> convoylabels;
	slist_tpl<button_t *> convoybuttons;
	slist_tpl<char*> label_names;

public:
	halt_detail_t(halthandle_t halt);

	~halt_detail_t();

	void halt_detail_info();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const { return "station_details.txt"; }

	// Set window size and adjust component sizes and/or positions accordingly
	virtual void set_windowsize(scr_size size);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// only defined to update schedule, if changed
	void draw( scr_coord pos, scr_size size );

	// this constructor is only used during loading
	halt_detail_t();

	void rdwr( loadsave_t *file );

	uint32 get_rdwr_id() { return magic_halt_detail; }
};

#endif
