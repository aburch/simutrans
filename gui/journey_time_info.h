/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_JOURNEY_TIME_INFO_T_H
#define GUI_JOURNEY_TIME_INFO_T_H


#include "components/gui_aligned_container.h"
#include "components/gui_scrollpane.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "gui_frame.h"
#include "../linehandle_t.h"
#include "../dataobj/schedule_entry.h"

class schedule_t;
class player_t;

class gui_journey_time_stat_t : public gui_aligned_container_t {
  player_t* player;
  
public:
  gui_journey_time_stat_t(schedule_t*, player_t*);
  
  void update(linehandle_t, vector_tpl<uint32*>&, bool&);
};


class gui_journey_time_info_t : public gui_frame_t, private action_listener_t
{
private:
  gui_journey_time_stat_t stat;
	gui_scrollpane_t scrolly;
  button_t bt_copy_names, bt_copy_stations, bt_copy_csv;
  gui_label_t insufficient_cnv_label;
  
  player_t* player;
  schedule_t* schedule;
  linehandle_t line;
  cbuffer_t* title_buf;
  vector_tpl<uint32*> journey_times; // in divisor time unit
  uint32 journey_time_sum;
  
public:
	gui_journey_time_info_t(linehandle_t, player_t*);
  ~gui_journey_time_info_t();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void update();

	//void rdwr( loadsave_t *file );
};

#endif
