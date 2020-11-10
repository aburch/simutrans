/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_TIMES_HISTORY_H
#define GUI_TIMES_HISTORY_H


#include "components/gui_textarea.h"

#include "gui_frame.h"
#include "times_history_entry.h"
#include "times_history_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"

#include "../linehandle_t.h"
#include "../utils/cbuffer_t.h"

class player_t;

// @author: suitougreentea
class times_history_t : public gui_frame_t, action_listener_t
{
private:
	const player_t *owner;
	// if it is a history about convoi, line is null
	linehandle_t line;
	// if it is a history about line, convoi is null
	convoihandle_t convoi;
	const char* cached_name;
	uint32 update_time;
	schedule_t *last_schedule = NULL;
	bool last_convoy_reversed;

	cbuffer_t title_buf;

	gui_container_t cont;
	gui_scrollpane_t scrolly;
	slist_tpl <times_history_container_t *> history_conts;

public:
	times_history_t(linehandle_t line, convoihandle_t convoi);

	~times_history_t();

	void register_containers();
	void update_components();

	// const char * get_help_filename() const { return ".txt"; }

	// Set window size and adjust component sizes and/or positions accordingly
	virtual void set_windowsize(scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// only defined to update, if changed
	void draw( scr_coord pos, scr_size size ) OVERRIDE;

	// this constructor is only used during loading
	times_history_t();
};

#endif
