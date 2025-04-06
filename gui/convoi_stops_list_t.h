/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCHEDULE_GUI_H
#define GUI_SCHEDULE_GUI_H


#include "gui_frame.h"

#include "components/action_listener.h"

#include "components/gui_scrollpane.h"

#include "../convoihandle_t.h"
#include "../tpl/vector_tpl.h"


class schedule_t;
class player_t;
class cbuffer_t;
class convoi_stops_list_t;
class convoi_stops_list_item_t;


/**
 * List of displayed schedule entries.
 */
class convoi_stops_list_t : public gui_aligned_container_t, action_listener_t, public gui_action_creator_t
{
	convoihandle_t cnv;

	vector_tpl<convoi_stops_list_item_t*> entries;
	schedule_t *gui_schedule; // displaied schedule
public:
	player_t*  player;

	convoi_stops_list_t(convoihandle_t cnv_ = convoihandle_t());

	// shows/deletes highlighting of tiles
	void update_schedule();
	void draw(scr_coord offset) OVERRIDE;
	bool action_triggered(gui_action_creator_t *comp, value_t p) OVERRIDE;
};

#endif
